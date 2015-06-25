//============================================================================
// Name        : ProxyHttp.cpp
// Author      : Angelo Trotta
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <list>

#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

using namespace std;

#define START_FRAGMENT	5

#define INTERFACE_UPDATE_INTERVAL_SEC	3

#define MAX_DEBUG_LEVEL		3
#define MEDIUM_DEBUG_LEVEL	2
#define MIN_DEBUG_LEVEL		1
#define NO_DEBUG_LEVEL		0

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL MEDIUM_DEBUG_LEVEL
#endif

#define BIG_BUFF_LEN 16384

#define PROTOCOL_N_VAL		20
#define BLOCK_SIZE			5
#define BLOCK_NUMBER		4

#define SIZE_VECTOR			0
#define	TIME_VECTOR			1
//#define	THROUGHPUT_VECTOR	2
#define NUMBER_VECTORS		2

//#define matrix_idx(a, b, c) ((a * NUMBER_VECTORS) + (b * PROTOCOL_N_VAL) + c)
#define matrix_idx(a, b, c) ((a * NUMBER_VECTORS * PROTOCOL_N_VAL) + (b * PROTOCOL_N_VAL) + c)

#if DEBUG_LEVEL==NO_DEBUG_LEVEL

#define debug_low(...)
#define debug_medium(...)
#define debug_high(...)

#elif  DEBUG_LEVEL==MIN_DEBUG_LEVEL

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...)
#define debug_high(...)

#elif  DEBUG_LEVEL==MEDIUM_DEBUG_LEVEL

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...) printf (__VA_ARGS__);
#define debug_high(...)

#else

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...) printf (__VA_ARGS__);
#define debug_high(...) printf (__VA_ARGS__);

#endif

static int *glob_var;
static size_t map_size;

static int *buffer_vec;
static size_t buffer_vec_size;

static struct timeval *t0;

typedef struct mean_var {
	double mean;
	double variance;
	double standard_dev;
} mean_var_t;

typedef struct interface_stat {
	vector <mean_var_t> block_vector;
	int filled_block;
	double p_standardDev;
	double p_mean;
	double expected_thr;
} interface_stat_t;

typedef struct fragment {
	char video_name[32];
	char small_video_name[16];
	int frag_seconds;
	int bps;
	int frag_number;
	int frag_bytesize;
	struct in_addr choosed_interface;
	time_t start_request_time;
	time_t end_request_time;
	struct timeval  start_request_timeval;
	struct timeval  end_request_timeval;
	bool reply_ok;
	bool isMS4;
} fragment_t;

char buffer[BIG_BUFF_LEN];

int chooseIFandBIND(int socket, vector<struct sockaddr_in> &if_list, fragment_t *frag, int &if_choosed);
void fillFragmentField(char *path, fragment_t *frag);
void makeFragStat(fragment_t *frag, char *experiment_name);
void updateBestIfAlgo(vector<struct sockaddr_in> &if_vector, fragment_t *frag, int idx_if);
void updateBestIfAlgo_data(int idx_if, int size_pkt, long int time_pkt);
void chooseIF(int socket, vector<struct sockaddr_in> &if_vector, fragment_t *frag, int &if_choosed);
int chooseBestIfAlgo(vector<struct sockaddr_in> &if_vector, fragment_t *frag);
void updateIfVector(vector<struct sockaddr_in> &if_vector, char *client_if);

void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

long int timevaldiff_usec(struct timeval *start, struct timeval *end) {
	return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

int main(int argc,char* argv[]) {

	pid_t pid;
	struct sockaddr_in cli_addr, serv_addr;
	unsigned int clilen;
	int sockfd, newsockfd;
	vector<struct sockaddr_in> interface_vector;
	char *exp_name, *client_interface;
	time_t lastInterfaceUpdate, now;

	sem_t *sem_estimator = sem_open("vec_estimator_semaphore", O_CREAT|O_EXCL, 0777, 1);
	sem_unlink("vec_estimator_semaphore");

	sem_t *sem = sem_open("proxy_semaphore", O_CREAT|O_EXCL, 0777, 1);
	sem_unlink("proxy_semaphore");


	//printf("3- Sono %d e sono vivo\n", 0);fflush(stdout);
	int valSem;
	sem_getvalue(sem, &valSem);
	printf("3.0- Sono %d e sono vivo- status %d\n", 0, valSem);fflush(stdout);
	sem_wait(sem);
	//printf("3.1- Sono %d e sono vivo\n", 0);fflush(stdout);
	sem_post(sem);
	//printf("4- Sono %d e sono vivo\n", 0);

	if(argc<4){
		fprintf(stderr, "./proxy <port_no> <client_interface> <experiment_name>\n");
		return EXIT_FAILURE;
	}

	// register to signal to prevent zombie child
	struct sigaction sa;
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) == -1) {
	  perror("sigaction error");
	  exit(EXIT_FAILURE);
	}

	exp_name = argv[3];
	client_interface = argv[2];
	glob_var = NULL;

	buffer_vec_size = 512 * sizeof(int);
	buffer_vec = (int *) mmap(NULL, buffer_vec_size,	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (buffer_vec == MAP_FAILED) {
		perror("MAP_FAILED");
		exit(EXIT_FAILURE);
	}
	memset (buffer_vec, 0, buffer_vec_size * sizeof(int));

	t0 = (struct timeval *) mmap(NULL, sizeof(struct timeval),	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (t0 == MAP_FAILED) {
		perror("MAP_FAILED");
		exit(EXIT_FAILURE);
	}
	memset (t0, 0, sizeof(struct timeval));

	updateIfVector(interface_vector, client_interface);
	time(&lastInterfaceUpdate);

	bzero((char*)&serv_addr,sizeof(serv_addr));
	bzero((char*)&cli_addr, sizeof(cli_addr));

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr=INADDR_ANY;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		perror("Problem in initializing socket");
		return EXIT_FAILURE;
	}

	if (bind(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error on binding");
		return EXIT_FAILURE;
	}

	if (listen(sockfd, 50) < 0) {
		perror("Error on listen");
		return EXIT_FAILURE;
	}

	while(true) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			return EXIT_FAILURE;
		}

		debug_medium("\nAccepted new connection from client\n");

		// must re-check interfaces?
		time(&now);
		if (difftime(now, lastInterfaceUpdate) > INTERFACE_UPDATE_INTERVAL_SEC) {
			updateIfVector(interface_vector, client_interface);
			time(&lastInterfaceUpdate);
		}

		// managing the GET by the child process
		pid = fork();
		if (pid < 0) {
			perror("ERROR on fork");

			close(newsockfd);
			close(sockfd);

			return EXIT_FAILURE;
		}
		else if (pid > 0) {
			// parent process
			close(newsockfd);
		}
		else {
			// child process
			//char buffer[8192];
			//char buffer[BIG_BUFF_LEN];
			char t1[256], t2[256], t3[256];
			int nrcv;
			int sockfd_child = -1;
			//char *buffer;
			bool isGET;

			int mypid = getpid();
			srand(mypid);

			close(sockfd);

			do {

				isGET = false;

				bzero(buffer, sizeof(buffer));
				nrcv = recv(newsockfd, buffer, sizeof(buffer), 0);

				if (nrcv < 0) {
					perror("Error recv from client socket");
				}
				else if (nrcv == 0) {
					printf("Connection closed by client\n");
				}
				else {
					if (strncmp(buffer, "GET", 3) == 0) {
						struct sockaddr_in host_addr;
						struct hostent* host;
						unsigned int i;
						char* temp=NULL;
						int port = 80;
						int flag=0;
						int interface_idx_choosed = -1;

						fragment_t newFrag;
						newFrag.isMS4 = false;

						//isGET = true;

						//printf("\n\nPARSING NEW REQUEST:\n");
						debug_high("\nPARSING NEW REQUEST:\n%s\n", buffer);

						//parsing the host and the port
						sscanf(buffer, "GET %s %s\r\n%*s", t2, t3);
						if( ((strncmp(t3, "HTTP/1.1", 8) == 0) || (strncmp(t3, "HTTP/1.0", 8) == 0 )) &&
								(strncmp(t2, "http://", 7) == 0) )
						{
							bool sendBackClient = true;

							//search the port
							strcpy(t1,t2);

							flag=0;

							for(i = 7; i < strlen(t2); i++) {
								if(t2[i]==':')
								{
									flag=1;
									break;
								}
							}

							temp = strtok(t2, "//");
							//temp = &t2[6];
							if (flag==0) {
								temp=strtok(NULL,"/");
							}
							else {
								temp=strtok(NULL,":");
								temp++;
							}

							sprintf(t2, "%s", temp);
							debug_high("host = %s", t2);
							host = gethostbyname(t2);

							if(flag==1) {
								temp=strtok(NULL,"/");
								port=atoi(temp);
							}

							strcat(t1,"^]");
							temp=strtok(t1,"//");
							temp=strtok(NULL,"/");
							if(temp!=NULL)
								temp=strtok(NULL,"^]");
							debug_high("\npath = %s\nport = %d\n",temp,port);

							if ((temp) && (strncmp(temp, "ftp/datasets/DASHDataset2014/", 29) == 0)) {
								char *path = &temp[29];
								int pathLen;
								//FILE *fileStat;

								pathLen = strlen(path);

								//printf("File path: %s \n", path);

								// controllo solo i file m4s
								if (strncmp(&path[pathLen - 4], ".m4s", 4) == 0) {

									newFrag.isMS4 = true;
									time(&newFrag.start_request_time);
									gettimeofday(&newFrag.start_request_timeval, NULL);

									// parsing the path
									fillFragmentField(path , &newFrag);
								}
							}

							//if ((newFrag.isMS4) && (interface_vector)) {
								// check if I have to update the interface making a fork()

							//}

							int idx_backup = -1;
							chooseIF(sockfd_child, interface_vector, &newFrag, interface_idx_choosed);
							if ((newFrag.isMS4) && (interface_vector.size() > 1)) {
								do {
									idx_backup = rand() % interface_vector.size();
								} while (idx_backup == interface_idx_choosed);

								if (((rand() % 100) < 30) || (true)) {
									int pid_child = fork();
									if (pid_child < 0) {
										perror("ERROR on fork");
									}
									else if (pid_child > 0) {
										// parent process

									}
									else {
										interface_idx_choosed = idx_backup;
										sendBackClient = false;

										newFrag.choosed_interface.s_addr = interface_vector[interface_idx_choosed].sin_addr.s_addr;

										close(newsockfd);
									}
								}
							}

							//creating the connection to the server web
							bzero((char*)&host_addr,sizeof(host_addr));
							host_addr.sin_port=htons(port);
							host_addr.sin_family=AF_INET;
							bcopy((char*)host->h_addr, (char*)&host_addr.sin_addr.s_addr, host->h_length);

							sockfd_child = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

							printf("Prima della bind\n"); fflush(stdout);

							int risBind = bind (sockfd_child, (struct sockaddr *)&interface_vector[interface_idx_choosed], sizeof(interface_vector[interface_idx_choosed]));

							//if (newFrag.isMS4 && chooseIFandBIND(sockfd_child, interface_vector, &newFrag, interface_idx_choosed) < 0) {
							if (newFrag.isMS4 && (risBind < 0)) {
							//if (false) {
								perror("Error in binding on a local interface");
							}
							else {

								printf("dopo la bind\n"); fflush(stdout);

								if(connect(sockfd_child, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) < 0) {
									perror("Error in connecting to remote server");
								}
								else {
									int n_send, reqStr_len;

									printf("mi sono connesso\n"); fflush(stdout);

									debug_medium("Connected to %s  IP - %s\n", t2, inet_ntoa(host_addr.sin_addr));


									reqStr_len = strlen(buffer);

									/*bzero((char*)buffer, sizeof(buffer));
									if( temp != NULL ) {
										sprintf(buffer,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp,t3,t2);
										//sprintf(buffer,"GET /%s %s\r\nHost: %s\r\n\r\n",temp,t3,t2);
									}
									else {
										sprintf(buffer,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",t3,t2);
										//sprintf(buffer,"GET / %s\r\nHost: %s\r\n\r\n",t3,t2);
									}*/

									sprintf(&buffer[reqStr_len],"Connection: close\r\n\r\n");

									//printf("Sending the GET: \n%s\n", buffer);
									debug_medium("Sending the GET\n");

									n_send = send(sockfd_child, buffer, strlen(buffer), 0);
									//n_send = send(sockfd_child, buffer, nrcv, 0);

									if (n_send < 0) {
										perror("Error writing to socket");
									}
									else{
										int n_recv;
										int n_tot_recv = 0;
										int max_pkt = 0;
										int stat_recv = 0;

										debug_medium("Receiving from client and sending to the server\n");

										newFrag.reply_ok = true;

										do {
											struct timeval time_st, time_en;

											bzero(buffer, sizeof(buffer));

											if (n_tot_recv == stat_recv) {
												gettimeofday(&time_st, NULL);
											}

											n_recv = recv(sockfd_child, buffer, sizeof(buffer), 0);

											if (max_pkt < n_recv) max_pkt = n_recv;
											n_tot_recv += n_recv;

											debug_high("\r%d of %d  (MAX - %d)", n_recv, n_tot_recv, max_pkt);

											//printf("Received %d bytes from server\n", n_recv);

											if (n_recv > 0) {
												if (sendBackClient) {
													int n_sent = send(newsockfd, buffer, n_recv, 0);
													if(n_sent < 0) {
														perror("Error sending to client the server reply");
														newFrag.reply_ok = false;
													}
													else if(n_sent == 0) {
														debug_low("Connection closed by the server while sending messages!?!?!\n");
														newFrag.reply_ok = false;
													}
												}

												if ((n_tot_recv - stat_recv) >= 150000 ) {
													gettimeofday(&time_en, NULL);

													sem_wait(sem_estimator);
													updateBestIfAlgo_data(interface_idx_choosed, (n_tot_recv - stat_recv),
															timevaldiff_usec(&time_st, &time_en));
													sem_post(sem_estimator);

													stat_recv = n_tot_recv;
												}
											}
											else if (n_recv == 0) {
												debug_high(" - connection closed by the server");
											}
											else {
												perror("Error receiving from client while forwarding");
												newFrag.reply_ok = false;
											}


											//fflush(stdout);

										} while (n_recv > 0);

										newFrag.frag_bytesize = n_tot_recv;

										time(&newFrag.end_request_time);
										gettimeofday(&newFrag.end_request_timeval, NULL);

										debug_high("\n");

										debug_medium("recv&sent %d Byte, %lf kB/sec, %d max_buffer\n",
												newFrag.frag_bytesize,
												(((double)newFrag.frag_bytesize) / ((double) timevaldiff_usec(&newFrag.start_request_timeval, &newFrag.end_request_timeval))) * (1000000.0 / 1024.0),
												max_pkt);
									}

									//close(sockfd_child);
								}
							}

							if (newFrag.isMS4) {
								long int ttt = (newFrag.end_request_timeval.tv_sec * 1000000 + newFrag.end_request_timeval.tv_usec)
																			-(newFrag.start_request_timeval.tv_sec * 1000000 + newFrag.start_request_timeval.tv_usec);

								//debug_low("ttt: %ld\n", ttt);

								debug_low("%-15s - %.03lf kB - %lf sec - %.03lf kB/s - %s_%dsec_%d_%dbps\n",
										inet_ntoa(newFrag.choosed_interface),
										((double) newFrag.frag_bytesize) / 1024.0,
										((double) ttt) / 1000000.0,
										(((double) newFrag.frag_bytesize) / 1024.0) / (((double) ttt) / 1000000.0 ),
										newFrag.video_name, newFrag.frag_seconds, newFrag.frag_number, newFrag.bps);
							}

							if (newFrag.isMS4) {
								/*printf("1- Sono %d e sono vivo\n", mypid);
								sem_wait(sem_estimator);
								printf("1.1- Sono %d e sono vivo\n", mypid);
								updateBestIfAlgo(interface_vector, &newFrag, interface_idx_choosed);
								sem_post(sem_estimator);
								printf("2- Sono %d e sono vivo\n", mypid);*/



								if(newFrag.frag_number <= START_FRAGMENT) {
									gettimeofday(t0, NULL);
								}

								if((newFrag.frag_number > 0) && (newFrag.frag_number < (int)(buffer_vec_size / sizeof(int)))) {
									buffer_vec[newFrag.frag_number - 1] = 1;
								}
							}

							if (sendBackClient) {
								printf("3- Sono %d e sono vivo\n", mypid);fflush(stdout);
								int valSem;
								sem_getvalue(sem, &valSem);
								printf("3.0- Sono %d e sono vivo- status %d\n", mypid, valSem);fflush(stdout);
								sem_wait(sem);
								printf("3.1- Sono %d e sono vivo\n", mypid);fflush(stdout);
								makeFragStat(&newFrag, exp_name);
								sem_post(sem);
								printf("4- Sono %d e sono vivo\n", mypid);
							}
						}

						debug_medium("Ending GET management\n");
					}
					else {

						debug_low("No a GET request [%d byte]... sending a BAD reply\n%s\n", nrcv, buffer);

						send(newsockfd, "400 : BAD REQUEST\nONLY HTTP GET REQUESTS ALLOWED",
								sizeof("400 : BAD REQUEST\nONLY HTTP GET REQUESTS ALLOWED"), 0);
					}
				}

			} while (isGET);

			debug_medium("Ending child process\n");

			close(sockfd_child);
			close(newsockfd);

			munmap(glob_var, map_size);
			munmap(buffer_vec, buffer_vec_size);
			munmap(t0, sizeof(struct timeval));

			_exit(EXIT_SUCCESS);
		}
	}

	munmap(glob_var, map_size);
	munmap(buffer_vec, buffer_vec_size);
	munmap(t0, sizeof(struct timeval));

	return EXIT_SUCCESS;
}

void updateIfVector(vector<struct sockaddr_in> &if_vector, char *client_if) {

	vector<struct sockaddr_in> new_vector;
	int *tmp_glob_var = NULL;
	size_t tmp_map_size;

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

					// getting all the interface "eth*" and "wlan*"
					if (	( 	(strncmp(ifa->ifa_name, "eth", 3) == 0) ||
								(strncmp(ifa->ifa_name, "wlan", 4) == 0) ||
								(strncmp(ifa->ifa_name, "usb", 3) == 0) ) &&
							(strcmp(ifa->ifa_name, client_if) != 0) ){
					//if ( (strcmp(ifa->ifa_name, client_if) != 0) && (strncmp(ifa->ifa_name, "lo", 2) != 0)) {
						struct sockaddr_in new_if;

						//printf("Sono wlan o eth o usb\n");

						bzero((char*)&new_if,sizeof(new_if));

						new_if.sin_family=AF_INET;
						new_if.sin_port=htons(0);
						new_if.sin_addr.s_addr=inet_addr(ip_addr);

						new_vector.push_back(new_if);
					}
				}
			}
		}
	}

	// controllo se ci sono stati cambiamenti
	bool change = false;
	if (if_vector.size() != new_vector.size()) {
		change = true;
	}
	else {
		for (int i = 0; i < (int)if_vector.size(); i++) {
			bool trovato = false;

			for (int j = 0; j < (int)new_vector.size(); j++) {
				if (if_vector[i].sin_addr.s_addr == new_vector[j].sin_addr.s_addr) {
					trovato = true;
					break;
				}
			}

			if (!trovato) {
				change = true;
			}
		}
	}

	if (change) {
		// Il numero delle interfacce (o l'indirizzo ip) è cambiato... rifaccio tutta la mappa
		// faccio sempre tutto d'accapo (tanto non succederà così spesso...)
		tmp_map_size = new_vector.size() * NUMBER_VECTORS * PROTOCOL_N_VAL * sizeof(int);
		tmp_glob_var = (int *) mmap(NULL, tmp_map_size,	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

		if (tmp_glob_var == MAP_FAILED) {
			perror("MAP_FAILED");
			exit(EXIT_FAILURE);
		}

		memset (tmp_glob_var, 0, tmp_map_size * sizeof(int));

		if (glob_var != NULL) {
			// devo salvarmi nella nuova mappa i dati delle interfaccie che non sono cambiate
			for (int if_idx_old = 0; if_idx_old < (int)if_vector.size(); if_idx_old++) {
				for (int if_idx_new = 0; if_idx_new < (int)new_vector.size(); if_idx_new++) {
					if (if_vector[if_idx_old].sin_addr.s_addr == new_vector[if_idx_new].sin_addr.s_addr) {

						for (int i = 0; i < NUMBER_VECTORS; i++) {
							for (int j = 0; j < PROTOCOL_N_VAL; j++) {
								tmp_glob_var[matrix_idx(if_idx_new, i, j)] = glob_var[matrix_idx(if_idx_old, i, j)];
							}
						}

						break;
					}
				}
			}

			// dealloco la vecchia mappa
			munmap(glob_var, map_size);
		}

		// rendo la nuova mappa globale
		glob_var = tmp_glob_var;
		map_size = tmp_map_size;

		// copio il nuovo set di interfacce
		if_vector.clear();
		if_vector = new_vector;

		debug_low("Found new interfaces: ");
		for (int i = 0; i < (int)if_vector.size(); i++) {
			debug_low("%s ", inet_ntoa(if_vector[i].sin_addr));
		}
		debug_low("\n");
	}
	/*else {
		printf("No interfaces changes\n");
	}*/
}

void chooseIF(int socket, vector<struct sockaddr_in> &if_vector, fragment_t *frag, int &if_choosed) {
	int ris = -1;

	if (if_vector.size() > 0) {
		int idx_ok;
		vector<struct sockaddr_in>::iterator it_ifList;

		// choosing randomly
		//idx_ok = rand() % if_vector.size();

		// use best algoithm
		idx_ok = chooseBestIfAlgo(if_vector, frag);

		for (it_ifList = if_vector.begin(); it_ifList != if_vector.end(); it_ifList++) {
			debug_medium("Choosing from: %s\n", inet_ntoa(it_ifList->sin_addr));
		}

		debug_medium("Binding on the %d_th (over %d): %s\n", idx_ok, (int)if_vector.size(), inet_ntoa(if_vector[idx_ok].sin_addr));

		frag->choosed_interface.s_addr = if_vector[idx_ok].sin_addr.s_addr;

		if_choosed = idx_ok;

		//ris = bind (socket, (struct sockaddr *)&if_vector[idx_ok], sizeof(if_vector[idx_ok]));
	}
}

int chooseIFandBIND(int socket, vector<struct sockaddr_in> &if_vector, fragment_t *frag, int &if_choosed) {
	int ris = 0;

	if (if_vector.size() > 0) {
		int idx_ok;
		vector<struct sockaddr_in>::iterator it_ifList;

		// choosing randomly
		//idx_ok = rand() % if_vector.size();

		// use best algoithm
		idx_ok = chooseBestIfAlgo(if_vector, frag);

		for (it_ifList = if_vector.begin(); it_ifList != if_vector.end(); it_ifList++) {
			debug_medium("Choosing from: %s\n", inet_ntoa(it_ifList->sin_addr));
		}

		debug_medium("Binding on the %d_th (over %d): %s\n", idx_ok, (int)if_vector.size(), inet_ntoa(if_vector[idx_ok].sin_addr));

		frag->choosed_interface.s_addr = if_vector[idx_ok].sin_addr.s_addr;

		if_choosed = idx_ok;

		ris = bind (socket, (struct sockaddr *)&if_vector[idx_ok], sizeof(if_vector[idx_ok]));
	}

	return ris;
}

void fillFragmentField(char *path ,fragment_t *frag) {
	int extra_sec;
	char extra_videoname[32];
	char *ptr, *ptr_tmp, *ptr_tmp2;
	char tmp_buff[128];
	int stage = 0;

	//printf("Path: %s\n\n", path);

	ptr = strtok (path, "/");
	while (ptr != NULL) {
		//printf ("[%d] %s\n", stage, ptr);

		switch (stage) {
		case 0:
			sscanf(ptr, "%s", frag->video_name);

			break;

		case 1:
			snprintf(tmp_buff, sizeof(tmp_buff), "%s", ptr);
			ptr_tmp = strchr (tmp_buff, 's');
			*ptr_tmp = 0;
			//printf ("TMP: %s\n", tmp_buff);
			sscanf(tmp_buff, "%i", &frag->frag_seconds);

			break;

		case 2:
			snprintf(tmp_buff, sizeof(tmp_buff), "%s", ptr);
			ptr_tmp = strchr (tmp_buff, '_');
			*ptr_tmp = 0;
			//printf ("TMP: %s\n", tmp_buff);
			sscanf(tmp_buff, "%s", frag->small_video_name);

			ptr_tmp++;
			ptr_tmp2 = strchr (ptr_tmp, 'b');
			*ptr_tmp2 = 0;
			//printf ("TMP: %s\n", ptr_tmp);
			sscanf(ptr_tmp, "%d", &frag->bps);

			break;

		case 3:

			//	TYPE_BIGBUNNY	0 		-- BigBuckBunny/1sec/bunny_46980bps/BigBuckBunny_1s3.m4s
			//	TYPE_REDBULL	1		-- RedBullPlayStreets/1sec/redbull_101976bps/RedBull2.m4s
			//	TYPE_TEAR		2		-- TearsOfSteel/1sec/tos_101bps/TearsOfSteel_1s_2.m4s
			//	TYPE_VALKAAMA	3		-- Valkaama/2sec/valkaama_46208bps/Valkaama_23.m4s

			if (	(strncmp(path, "BigBuckBunny", 12) == 0) ||
					(strncmp(path, "ElephantsDream", 14) == 0) ||
					(strncmp(path, "OfForestAndMen", 14) == 0) ||
					(strncmp(path, "TheSwissAccount", 15) == 0) ) {
				//	TYPE_BIGBUNNY	0 		-- BigBuckBunny/1sec/bunny_46980bps/BigBuckBunny_1s3.m4s
				snprintf(tmp_buff, sizeof(tmp_buff), "%s", ptr);
				ptr_tmp = strchr (tmp_buff, '_');
				*ptr_tmp = 0;
				sscanf(tmp_buff, "%s", extra_videoname);

				ptr_tmp++;
				ptr_tmp2 = strchr (ptr_tmp, 's');
				*ptr_tmp2 = 0;
				sscanf(ptr_tmp, "%d", &extra_sec);

				ptr_tmp2++;
				ptr_tmp = strchr (ptr_tmp2, '.');
				*ptr_tmp = 0;
				sscanf(ptr_tmp2, "%d", &frag->frag_number);

			}
			else if (strncmp(path, "RedBullPlayStreets", 18) == 0) {
				//	TYPE_REDBULL	1		-- RedBullPlayStreets/1sec/redbull_101976bps/RedBull2.m4s
				sscanf(ptr, "RedBull%d.m4s", &frag->frag_number);
			}
			else if (strncmp(path, "TearsOfSteel", 12) == 0) {
				//	TYPE_TEAR		2		-- TearsOfSteel/1sec/tos_101bps/TearsOfSteel_1s_2.m4s
				sscanf(ptr, "TearsOfSteel_1s_%d.m4s", &frag->frag_number);
			}
			else if (strncmp(path, "Valkaama", 8) == 0) {
				//	TYPE_VALKAAMA	3		-- Valkaama/2sec/valkaama_46208bps/Valkaama_23.m4s
				sscanf(ptr, "Valkaama_%d.m4s", &frag->frag_number);
			}

			break;
		}

		stage++;
		ptr = strtok (NULL, "/");
	}
}

/*
typedef struct fragment {
	char video_name[32];
	char small_video_name[16];
	int frag_seconds;
	int bps;
	int frag_number;
	time_t start_request_time;
	time_t end_request_time;
	bool reply_ok;
	bool isMS4;
} fragment_t;
 */

void makeFragStat(fragment_t *frag, char *experiment_name) {
	FILE *fileExp;
	char buff_stat[256];

	if (frag->isMS4) {
		//snprintf(buff_stat, sizeof(buff_stat), "%s-%d_%d", experiment_name, getppid(), getpid());
		//snprintf(buff_stat, sizeof(buff_stat), "%s-%s_%dsec_%d",
				//experiment_name, frag->video_name, frag->frag_seconds, getpid());
		snprintf(buff_stat, sizeof(buff_stat), "%s-%s_%dsec",
				experiment_name, frag->video_name, frag->frag_seconds);

		fileExp = fopen(buff_stat, "a");
		if (fileExp) {
			struct tm *tm_start, *tm_end;
			char buffer_s[32], buffer_e[32];
			//double useconds, millisec;
			//double throughput;

			tm_start = localtime(&frag->start_request_time);
			tm_end = localtime(&frag->end_request_time);

			strftime(buffer_s, 32, "%H:%M:%S", tm_start);
			strftime(buffer_e, 32, "%H:%M:%S", tm_end);

			//double seconds = difftime(frag->end_request_time, frag->start_request_time);

			long int useconds_st = frag->start_request_timeval.tv_sec * 1000000.0 + frag->start_request_timeval.tv_usec;
			long int useconds_et = frag->end_request_timeval.tv_sec * 1000000.0 + frag->end_request_timeval.tv_usec;

			//long int useconds = (	(frag->end_request_timeval.tv_sec * 1000000.0 + frag->end_request_timeval.tv_usec)
			//		  	  	  	  -	(frag->start_request_timeval.tv_sec * 1000000.0 + frag->start_request_timeval.tv_usec));

			long int useconds = useconds_et - useconds_st;

			//double millisec = useconds / 1000.0;

			double throughput = (((double) frag->frag_bytesize) / ((double) useconds)) * 1000000.0;

			int buff_size = 0;
			struct timeval t1;
			gettimeofday(&t1, NULL);

			long int diff = timevaldiff_usec(t0, &t1);
			double diff_sec = diff / 1000000.0;
			//int buff_size = (frag->frag_seconds * frag->frag_number) - ceil(diff_sec);
			buff_size = 0;
			int idx = ceil(diff_sec);
			while (buffer_vec[idx] != 0) {
				buff_size++;
				idx++;
			}

			// videoname bitrate ok? sec_per_frag frag_number INTERFACE_IP frag_byte start_time end_time useconds throughput(B/s) buff_size

			//int nw = snprintf(buff_stat, sizeof(buff_stat), "%s\t%d\t%d\t%d\t%s\t%d\t%s.%ld\t%s.%ld\t%ld\t%f\n",
			//		frag->video_name, frag->bps, frag->frag_seconds, frag->frag_number,
			//		inet_ntoa(frag->choosed_interface), frag->frag_bytesize,
			//		buffer_s, frag->start_request_timeval.tv_usec,
			//		buffer_e, frag->end_request_timeval.tv_usec,
			//		useconds, throughput);
			int nw = snprintf(buff_stat, sizeof(buff_stat), "%s\t%d\t%d\t%d\t%d\t%s\t%d\t%ld\t%ld\t%ld\t%f\t%d\n",
					frag->video_name, frag->bps, frag->reply_ok ? 1 : 0,
					frag->frag_seconds, frag->frag_number,
					inet_ntoa(frag->choosed_interface), frag->frag_bytesize,
					useconds_st, useconds_et,
					useconds, throughput, buff_size);

			fwrite(buff_stat, 1, nw, fileExp);

			fclose(fileExp);
		}
	}
}

void updateBestIfAlgo_data(int idx_if, int size_pkt, long int time_pkt) {

	printf("Updating interface %d, size %d, time %ld\n", idx_if, size_pkt, time_pkt);

	if (idx_if < 0)	return;
	if (size_pkt <= 0) return;
	if (time_pkt <= 0) return;

	for (int vec_type = 0; vec_type < NUMBER_VECTORS; vec_type++) {
		for (int i = 0; i < PROTOCOL_N_VAL; i++) {

			if (i < (PROTOCOL_N_VAL - 1)) {
				glob_var[matrix_idx(idx_if, vec_type, i)] = glob_var[matrix_idx(idx_if, vec_type, i + 1)];
				//glob_var[idx_if][vec_type][i] = glob_var[idx_if][vec_type][i + 1];
			}
			else {

				switch (vec_type) {
				case SIZE_VECTOR:
					//glob_var[idx_if][SIZE_VECTOR][i] = frag->frag_bytesize;
					glob_var[matrix_idx(idx_if, SIZE_VECTOR, i)] = size_pkt;
					break;

				case TIME_VECTOR:
					//printf("*** Start: %ld, End: %ld, Diff: %d\n", tmp_start_time, tmp_end_time, tmp_time);

					//glob_var[idx_if][TIME_VECTOR][i] = tmp_time;
					glob_var[matrix_idx(idx_if, TIME_VECTOR, i)] = time_pkt;
					break;
				}
			}

		}

	}
}

void updateBestIfAlgo(vector<struct sockaddr_in> &if_vector, fragment_t *frag, int idx_if) {
	long int tmp_start_time, tmp_end_time;
	int tmp_time;

	if (idx_if < 0)	return;

	if (frag->frag_bytesize <= 0) return;

	for (int vec_type = 0; vec_type < NUMBER_VECTORS; vec_type++) {
		for (int i = 0; i < PROTOCOL_N_VAL; i++) {

			if (i < (PROTOCOL_N_VAL - 1)) {
				glob_var[matrix_idx(idx_if, vec_type, i)] = glob_var[matrix_idx(idx_if, vec_type, i + 1)];
				//glob_var[idx_if][vec_type][i] = glob_var[idx_if][vec_type][i + 1];
			}
			else {

				switch (vec_type) {
				case SIZE_VECTOR:
					//glob_var[idx_if][SIZE_VECTOR][i] = frag->frag_bytesize;
					glob_var[matrix_idx(idx_if, SIZE_VECTOR, i)] = frag->frag_bytesize;
					break;

				case TIME_VECTOR:
					tmp_start_time = frag->start_request_timeval.tv_sec * 1000000 + frag->start_request_timeval.tv_usec;
					tmp_end_time = frag->end_request_timeval.tv_sec * 1000000 + frag->end_request_timeval.tv_usec;
					tmp_time = tmp_end_time - tmp_start_time;

					//printf("*** Start: %ld, End: %ld, Diff: %d\n", tmp_start_time, tmp_end_time, tmp_time);

					//glob_var[idx_if][TIME_VECTOR][i] = tmp_time;
					glob_var[matrix_idx(idx_if, TIME_VECTOR, i)] = tmp_time;
					break;

				}
			}
		}
	}

	/*for (int if_idx = 0; if_idx < (int)if_vector.size(); if_idx++) {
		printf ("IDX: %d\n", if_idx);
		for (int vec_type = 0; vec_type < NUMBER_VECTORS; vec_type++) {
			switch (vec_type) {
			case SIZE_VECTOR:
				printf ("SIZE - ");
				break;

			case TIME_VECTOR:
				printf ("TIME - ");
				break;
			}

			for (int i = 0; i < PROTOCOL_N_VAL; i++) {
				//printf ("%d ", glob_var[idx_if][vec_type][i]);
				printf ("%d ", glob_var[matrix_idx(if_idx, vec_type, i)]);
			}
			printf ("\n");
		}
	}*/

	/*for (int if_idx = 0; if_idx < (int)if_vector.size(); if_idx++) {
		printf ("IDX: %d - ", if_idx);

		for (int i = 0; i < PROTOCOL_N_VAL; i++) {
			//printf ("%d ", glob_var[idx_if][vec_type][i]);
			if (glob_var[matrix_idx(if_idx, SIZE_VECTOR, i)] > 0) {
				printf ("%lf ", (((double) glob_var[matrix_idx(if_idx, SIZE_VECTOR, i)])  /
						((double) glob_var[matrix_idx(if_idx, TIME_VECTOR, i)])) * (1000000.0 / 1024.0));
			}
			else {
				printf ("0 ");
			}
		}
		printf ("\n");
	}*/
}

int chooseBestIfAlgo(vector<struct sockaddr_in> &if_vector, fragment_t *frag) {
	int idx_ris = 0;
	int randomChoice_percent = 5;

	//if ((rand() % 100) < randomChoice_percent) {
	if (false) {
		idx_ris = rand() % if_vector.size();
	}
	else {

		int block_size = PROTOCOL_N_VAL / BLOCK_NUMBER;

		vector< interface_stat_t > if_thr_vector;

		//block_vector.resize(PROTOCOL_N_VAL/BLOCK_SIZE);
		if_thr_vector.resize (if_vector.size());

		for (int if_idx = 0; if_idx < (int)if_vector.size(); if_idx++) {

			//if_thr_vector[if_idx].block_vector.resize(PROTOCOL_N_VAL/BLOCK_SIZE);
			if_thr_vector[if_idx].block_vector.resize(BLOCK_NUMBER);
			if_thr_vector[if_idx].filled_block = 0;

			for (int block_idx = 0; block_idx < (int)if_thr_vector[if_idx].block_vector.size(); block_idx++) {
				//double sumByte = 0;
				//double sumTime = 0;
				double sumThr = 0;
				int count_thr = 0;

				for (int i = 0; i < block_size; i++) {
					int j_idx = (block_idx * block_size) + i;

					if (glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)] > 0) {
						//sumByte += glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)];
						//sumTime += glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)];

						sumThr += (((double) glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)]) / ((double) glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)])) * (1000000.0 / 1024.0);
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

					if (glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)] > 0) {
						double thr_act = (((double) glob_var[matrix_idx(if_idx, SIZE_VECTOR, j_idx)]) /
								((double) glob_var[matrix_idx(if_idx, TIME_VECTOR, j_idx)])) * (1000000.0 / 1024.0);

						sumVariance += pow (thr_act - if_thr_vector[if_idx].block_vector[block_idx].mean, 2);

						num_block_ok++;
					}
				}

				if (num_block_ok > 1) {
					if_thr_vector[if_idx].block_vector[block_idx].variance = sumVariance / ((double) (num_block_ok - 1));
					if_thr_vector[if_idx].block_vector[block_idx].standard_dev = sqrt (if_thr_vector[if_idx].block_vector[block_idx].variance);
				}
				else {
					if_thr_vector[if_idx].block_vector[block_idx].variance = 0,
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

		idx_ris = rand() % if_vector.size();
		if (enaught_info) {
			double best_dr = -1;
			for (int if_idx = 0; if_idx < (int)if_thr_vector.size(); if_idx++) {
				if (if_thr_vector[if_idx].expected_thr > best_dr) {
					best_dr = if_thr_vector[if_idx].expected_thr;
					idx_ris = if_idx;
				}
			}
		}

		debug_medium("\nBlock throughput\n");
		for (int if_idx = 0; if_idx < (int)if_vector.size(); if_idx++) {
			debug_medium("IDX: %d - ", if_idx);
			for (int i = 0; i < (int)if_thr_vector[if_idx].block_vector.size(); i++) {
				//printf ("%d ", glob_var[idx_if][vec_type][i]);
				debug_medium ("%lf[%lf] ", if_thr_vector[if_idx].block_vector[i].mean,  if_thr_vector[if_idx].block_vector[i].standard_dev );
			}
			debug_medium ("- P_std: %lf - P_mean: %lf - Filled: %d\n",
					if_thr_vector[if_idx].p_standardDev,
					if_thr_vector[if_idx].p_mean,
					if_thr_vector[if_idx].filled_block);
		}
	}


	return idx_ris;
}



