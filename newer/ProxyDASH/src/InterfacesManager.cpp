/*
 * InterfacesManager.cpp
 *
 *  Created on: 15/apr/2015
 *      Author: angelo
 */

#include "InterfacesManager.h"

static long long timevaldiff_usec(struct timeval *start, struct timeval *end) {
	return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

void InterfacesManager::checkInterfaces(std::list<std::string> &if2exclude) {
	std::vector< interface_info > new_vector;
	interface_info *tmp_glob_var = NULL;
	unsigned int tmp_map_vec_size;

	struct ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	struct ifaddrs *ifa = ifaddr;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr != NULL) {
			int family = ifa->ifa_addr->sa_family;
			if (family == AF_INET) {
				//printf("family AF_INET\n");
				char ip_addr[NI_MAXHOST];
				int s = getnameinfo(ifa->ifa_addr,
						((family == AF_INET) ? sizeof(struct sockaddr_in) :
								sizeof(struct sockaddr_in6)),
								ip_addr, sizeof(ip_addr), NULL, 0, NI_NUMERICHOST);
				if (s != 0) {
					printf("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(1);
				} else {
					//printf("%-7s: %s\n", ifa->ifa_name, ip_addr);
					std::list<std::string>::iterator it_blacklist;
					bool outputIF = true;

					for (it_blacklist = if2exclude.begin(); it_blacklist != if2exclude.end(); it_blacklist++) {
						std::string act_noIF = *it_blacklist;
						if (act_noIF.compare(ifa->ifa_name) == 0) {
							outputIF = false;
							break;
						}
					}

					if (outputIF){
						interface_info new_if;

						memset((char*)&new_if, 0, sizeof(new_if));

						new_if.addr_info = inet_addr(ip_addr);
						strncpy(new_if.name, ifa->ifa_name, sizeof(new_if.name));

						new_vector.push_back(new_if);
					}
				}
			}
		}
	}

	// controllo se ci sono stati cambiamenti
	bool change = false;
	if (interfaces_map_vector_size != new_vector.size()) {
		change = true;
	}
	else if (interfaces_map_vector_size > 0){
		bool trovato = false;

		for (unsigned int i = 0; i < interfaces_map_vector_size; i++) {

			for (int j = 0; j < (int)new_vector.size(); j++) {
				if ( 	(interfaces_map[i].addr_info == new_vector[j].addr_info) &&
						(strcmp(interfaces_map[i].name, new_vector[j].name) == 0) ){
					trovato = true;
					break;
				}
			}

			if (trovato) {
				break;
			}
		}

		if (!trovato) {
			change = true;
		}
	}

	if (change) {
		// Il numero delle interfacce (o l'indirizzo ip) è cambiato... rifaccio tutta la mappa
		// faccio sempre tutto d'accapo (tanto non succederà così spesso...)
		tmp_map_vec_size = new_vector.size();
		tmp_glob_var = (interface_info *) mmap(NULL, tmp_map_vec_size * sizeof(struct interface_info),	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

		if (tmp_glob_var == MAP_FAILED) {
			perror("MAP_FAILED in interface manager");
			exit(EXIT_FAILURE);
		}

		memset (tmp_glob_var, 0, tmp_map_vec_size * sizeof(struct interface_info));

		for (int i = 0; i < (int)new_vector.size(); i++) {
			tmp_glob_var[i].addr_info = new_vector[i].addr_info;
			strncpy(tmp_glob_var[i].name, new_vector[i].name, sizeof(tmp_glob_var[i].name));
			tmp_glob_var[i].statUpdate_sem = NULL;

			for (int j = 0; j < BLOCK_TOTAL_DIMENSION; j++) {
				gettimeofday(&tmp_glob_var[i].stats[j].timestamp, NULL);
			}
		}

		if (interfaces_map != NULL) {
			// devo salvarmi nella nuova mappa i dati delle interfaccie che non sono cambiate
			for (unsigned int i = 0; i < tmp_map_vec_size; i++) {
				for (unsigned int j = 0; j < interfaces_map_vector_size; j++) {

					if ( 	(tmp_glob_var[i].addr_info == interfaces_map[j].addr_info) &&
							(strcmp(tmp_glob_var[i].name, interfaces_map[j].name) == 0) ){

						for (int k = 0; k < BLOCK_TOTAL_DIMENSION; k++) {
							tmp_glob_var[i].stats[k].size = interfaces_map[j].stats[k].size;
							tmp_glob_var[i].stats[k].time = interfaces_map[j].stats[k].time;
							tmp_glob_var[i].stats[k].timestamp = interfaces_map[j].stats[k].timestamp;
						}

						tmp_glob_var[i].statUpdate_sem = interfaces_map[j].statUpdate_sem;
					}
				}
			}

			// dealloco la vecchia mappa
			munmap(interfaces_map, interfaces_map_vector_size * sizeof(struct interface_info));
		}

		//create the new semaphores
		for (unsigned int i = 0; i < tmp_map_vec_size; i++) {
			if (tmp_glob_var[i].statUpdate_sem == NULL) {
				char sem_name[64];

				snprintf(sem_name, sizeof(sem_name), "proxy_updatestat_semaphore_%s", tmp_glob_var[i].name);

				tmp_glob_var[i].statUpdate_sem = sem_open(sem_name, O_CREAT|O_EXCL, 0777, 1);
				sem_unlink(sem_name);
			}
		}


		// rendo la nuova mappa globale
		interfaces_map = tmp_glob_var;
		interfaces_map_vector_size = tmp_map_vec_size;

		// print out the interfaces
		debug_low("Found new interfaces: ");
		for (unsigned int i = 0; i < interfaces_map_vector_size; i++) {
			struct in_addr ia;
			ia.s_addr = interfaces_map[i].addr_info;

			debug_low("%s[%s] ", interfaces_map[i].name, inet_ntoa(ia));
		}
		debug_low("\n");
	}
	// else { printf("No interfaces changes\n"); }
}

void InterfacesManager::freeMemory(void) {
	if (interfaces_map != NULL) {
		munmap(interfaces_map, interfaces_map_vector_size * sizeof(struct interface_info));
		interfaces_map = NULL;
		interfaces_map_vector_size = 0;
	}
}

void InterfacesManager::printInterfaces(void) {
	if (interfaces_map != NULL) {
		for (u_int32_t j = 0; j < interfaces_map_vector_size; j++) {
			struct in_addr addr;
			addr.s_addr = interfaces_map[j].addr_info;
			printf("Interface %s address: %s\n", interfaces_map[j].name, inet_ntoa(addr));
		}
	}
}

void InterfacesManager::updateInterfaceStats (struct sockaddr_in *if_used, int pktSize, long long time_usec) {
	if (if_used == NULL) return;

	if (interfaces_map != NULL) {

		//get semaphore
		sem_wait(sem_estimator);

		debug_medium("[%d] ----- Updating interface %s STATS: %ud byte in %lld usec\n",
				getpid(), inet_ntoa(if_used->sin_addr), pktSize, time_usec);

		for (u_int32_t j = 0; j < interfaces_map_vector_size; j++) {

			if (interfaces_map[j].addr_info == if_used->sin_addr.s_addr){

				for (int i = 0; i < BLOCK_TOTAL_DIMENSION; i++) {
					if (i < (BLOCK_TOTAL_DIMENSION - 1)) {
						interfaces_map[j].stats[i].size = interfaces_map[j].stats[i+1].size;
						interfaces_map[j].stats[i].time = interfaces_map[j].stats[i+1].time;
						interfaces_map[j].stats[i].timestamp = interfaces_map[j].stats[i+1].timestamp;
					}
					else {
						interfaces_map[j].stats[i].size = pktSize;
						interfaces_map[j].stats[i].time = time_usec;
						gettimeofday(&interfaces_map[j].stats[i].timestamp, NULL);
					}
				}

				break;
			}
		}

		for (u_int32_t j = 0; j < interfaces_map_vector_size; j++) {
			debug_high("IDX: %d - ", j);
			for (int i = 0; i < BLOCK_TOTAL_DIMENSION; i++) {
				//printf ("%d ", glob_var[idx_if][vec_type][i]);
				if ((interfaces_map[j].stats[i].size > 0) && (interfaces_map[j].stats[i].time > 0)) {
					debug_high ("%lf ", (((double) interfaces_map[j].stats[i].size)  /
							((double) interfaces_map[j].stats[i].time)) * (1000000.0 / 1024.0));
				}
				else {
					debug_high ("0 ");
				}
			}
			debug_high ("\n");
		}

		//release semaphore
		sem_post(sem_estimator);
	}
}

void InterfacesManager::setUpdateFlag(bool updateFlag) {
	flag_update = updateFlag;
}

void InterfacesManager::setRandomChoice(bool r) {
	random_chioce = r;
}

void InterfacesManager::setTimerUpdate(int timer) {
	timer_update = timer;
}

void InterfacesManager::chooseIF(struct sockaddr_in &if_to_use, std::list<struct sockaddr_in> &if_to_update) {

	if_to_use.sin_family=AF_INET;
	if_to_use.sin_port=htons(0);

	// random choice
	if (random_chioce) {
		if_to_use.sin_addr.s_addr=interfaces_map [rand() % interfaces_map_vector_size].addr_info;
		debug_medium("Choosing the interface randomly. Choose: %s\n", inet_ntoa(if_to_use.sin_addr));
	}
	else {
		int block_size = BLOCK_SIZE;

		std::vector< interface_stat_t > if_thr_vector;

		if_thr_vector.resize (interfaces_map_vector_size);

		for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {

			//if_thr_vector[if_idx].block_vector.resize(PROTOCOL_N_VAL/BLOCK_SIZE);
			if_thr_vector[if_idx].block_vector.resize(BLOCK_NUMBER);
			if_thr_vector[if_idx].filled_block = 0;
			if_thr_vector[if_idx].addr_info.s_addr = interfaces_map[if_idx].addr_info;
			strncpy(if_thr_vector[if_idx].if_name, interfaces_map[if_idx].name, sizeof (if_thr_vector[if_idx].if_name));

			for (int block_idx = 0; block_idx < (int)if_thr_vector[if_idx].block_vector.size(); block_idx++) {
				//double sumByte = 0;
				//double sumTime = 0;
				double sumThr = 0;
				int count_thr = 0;

				for (int i = 0; i < block_size; i++) {
					int j_idx = (block_idx * block_size) + i;

					if (interfaces_map[if_idx].stats[j_idx].size > 0) {
					//if (glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)] > 0) {
						//sumByte += glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)];
						//sumTime += glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)];
						sumThr += (((double) interfaces_map[if_idx].stats[j_idx].size) / ((double) interfaces_map[if_idx].stats[j_idx].time)) * (1000000.0 / 1024.0);
						//sumThr += (((double) glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)]) / ((double) glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)])) * (1000000.0 / 1024.0);
						count_thr++;

					}
				}

				if_thr_vector[if_idx].block_vector[block_idx].mean = 0;
				if (count_thr > 0) {
					//if_thr_vector[if_idx].block_vector[block_idx].mean =
					//		(sumByte / sumTime) * (1000000.0 / 1024.0);		// convert to kB/s
					if_thr_vector[if_idx].block_vector[block_idx].mean = sumThr / ((double) count_thr);

					if_thr_vector[if_idx].filled_block++;
				}

				double sumVariance = 0;
				int num_block_ok = 0;
				for (int i = 0; i < block_size; i++) {

					int j_idx = (block_idx * block_size) + i;

					if (interfaces_map[if_idx].stats[j_idx].time > 0) {
					//if (glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)] > 0) {
						double thr_act = (((double) interfaces_map[if_idx].stats[j_idx].size) /
								((double) interfaces_map[if_idx].stats[j_idx].time)) * (1000000.0 / 1024.0);
						//double thr_act = (((double) glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)]) /
						//		((double) glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)])) * (1000000.0 / 1024.0);

						sumVariance += pow (thr_act - if_thr_vector[if_idx].block_vector[block_idx].mean, 2);

						num_block_ok++;
					}
				}

				if (num_block_ok > 1) {
					if_thr_vector[if_idx].block_vector[block_idx].variance = sumVariance / ((double) (num_block_ok - 1));
					if_thr_vector[if_idx].block_vector[block_idx].standard_dev = sqrt (if_thr_vector[if_idx].block_vector[block_idx].variance);
				}
				else {
					if_thr_vector[if_idx].block_vector[block_idx].variance = 0;
					if_thr_vector[if_idx].block_vector[block_idx].standard_dev = 0;
				}
			}

		}

		// calculate P
		for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
			double sd_partial_sum = 0;
			double m_partial_sum = 0;
			double weight_sum = 0;

			for (int block_idx = 0; block_idx < (int)if_thr_vector[if_idx].block_vector.size(); block_idx++) {

				if (if_thr_vector[if_idx].block_vector[block_idx].mean > 0) {
					double act_weight = ((double) (block_idx + 1)) / ((double) if_thr_vector[if_idx].block_vector.size());

					sd_partial_sum += act_weight * if_thr_vector[if_idx].block_vector[block_idx].standard_dev;
					m_partial_sum += act_weight * if_thr_vector[if_idx].block_vector[block_idx].mean;
					weight_sum += act_weight;
				}
			}

			//if_thr_vector[if_idx].p_standardDev = sd_partial_sum / weight_sum;
			if_thr_vector[if_idx].p_standardDev = if_thr_vector[if_idx].block_vector[if_thr_vector[if_idx].block_vector.size() - 1].standard_dev;
			if_thr_vector[if_idx].p_mean = m_partial_sum / weight_sum;

			if_thr_vector[if_idx].expected_thr = if_thr_vector[if_idx].p_mean - if_thr_vector[if_idx].p_standardDev;
			if (if_thr_vector[if_idx].expected_thr < 0) {
				if_thr_vector[if_idx].expected_thr = 0;
			}
		}

		// calculate the best interface
		bool enaught_info = true;
		/*for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
			//if (if_thr_vector[if_idx].filled_block < (int)if_thr_vector[if_idx].block_vector.size()) {
			if (if_thr_vector[if_idx].filled_block < 1) {
				enaught_info = false;
				break;
			}
		}*/

		//idx_ris = rand() % if_vector.size();
		if_to_use.sin_addr.s_addr=interfaces_map [rand() % interfaces_map_vector_size].addr_info;
		if (enaught_info) {
			double best_dr = -1;
			for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
				if (if_thr_vector[if_idx].expected_thr > best_dr) {
					best_dr = if_thr_vector[if_idx].expected_thr;
					//idx_ris = if_idx;
					if_to_use.sin_addr.s_addr=interfaces_map [if_idx].addr_info;
				}
			}
		}

		debug_medium("Best interface is %s\n", inet_ntoa(if_to_use.sin_addr));
		for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
			debug_medium("IF: %s %s - ", if_thr_vector[if_idx].if_name, inet_ntoa(if_thr_vector[if_idx].addr_info));
			for (int i = 0; i < (int)if_thr_vector[if_idx].block_vector.size(); i++) {
				//printf ("%d ", glob_var[idx_if][vec_type][i]);
				debug_medium ("%.2lf[%.2lf] ", if_thr_vector[if_idx].block_vector[i].mean,  if_thr_vector[if_idx].block_vector[i].standard_dev );
			}
			/*debug_medium ("- P_std: %lf.2 - P_mean: %lf.2 - Filled: %d\n",
					if_thr_vector[if_idx].p_standardDev,
					if_thr_vector[if_idx].p_mean,
					if_thr_vector[if_idx].filled_block);*/
			debug_medium ("- P [%.2lf; %.2lf] - ExpectedMinThr: %.2lf\n",
					if_thr_vector[if_idx].p_mean,
					if_thr_vector[if_idx].p_standardDev,
					if_thr_vector[if_idx].expected_thr);
		}

		/*debug_medium("\nBlock throughput\n");
		for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
			debug_medium("IDX: %d - ", if_idx);
			for (int i = 0; i < (int)if_thr_vector[if_idx].block_vector.size(); i++) {
				//printf ("%d ", glob_var[idx_if][vec_type][i]);
				debug_medium ("%lf[%lf] ", if_thr_vector[if_idx].block_vector[i].mean,  if_thr_vector[if_idx].block_vector[i].standard_dev );
			}
			debug_medium ("- P_std: %lf - P_mean: %lf - Filled: %d\n",
					if_thr_vector[if_idx].p_standardDev,
					if_thr_vector[if_idx].p_mean,
					if_thr_vector[if_idx].filled_block);
		}*/

	}

	// check il some of the other devices should be updated
	if_to_update.clear();
	
	if (flag_update) {

		struct timeval timeNOW;
		gettimeofday(&timeNOW, NULL);

		for (int if_idx = 0; if_idx < (int)interfaces_map_vector_size; if_idx++) {
			long long timeDIFF = (timevaldiff_usec(&interfaces_map[if_idx].stats[BLOCK_TOTAL_DIMENSION - 1].timestamp, &timeNOW)) / 1000000.0;

			//struct in_addr tt;
			//tt.s_addr = interfaces_map[if_idx].addr_info;
			//debug_medium("IF: %s - Time to last update: %lld sec\n", inet_ntoa(tt), timeDIFF);

			if (	(timeDIFF >= timer_update) ||
					( 	(interfaces_map[if_idx].stats[BLOCK_TOTAL_DIMENSION - 1].timestamp.tv_sec <= 0) &&
							(interfaces_map[if_idx].stats[BLOCK_TOTAL_DIMENSION - 1].timestamp.tv_usec <= 0) ) ){

				// controllo che non sia quello scelto per inviare il pacchetto su...
				if (if_to_use.sin_addr.s_addr != interfaces_map [if_idx].addr_info) {
					struct sockaddr_in toADD;
					toADD.sin_family=AF_INET;
					toADD.sin_port=htons(0);
					toADD.sin_addr.s_addr = interfaces_map [if_idx].addr_info;

					//debug_medium("IF %s need to be updated\n", inet_ntoa(tt));

					if_to_update.push_back(toADD);
				}
			}
		}
	}
}

bool InterfacesManager::isAlreadyInTest(struct sockaddr_in *addr_in) {
	bool ris = false;

	for (int if_idx = 0; if_idx < (int)interfaces_map_vector_size; if_idx++) {
		if (interfaces_map [if_idx].addr_info == addr_in->sin_addr.s_addr) {
			int semVal;

			if (sem_getvalue(interfaces_map [if_idx].statUpdate_sem, &semVal) < 0) {
				perror("Error getting semaphore value");
			}
			else {
				ris = semVal <= 0;
			}

			break;
		}
	}
	return ris;
}

