/*
 * StatManager.cpp
 *
 *  Created on: 26/mag/2015
 *      Author: angelo
 */

#include "StatManager.h"

static long int timevaldiff_usec(struct timeval *start, struct timeval *end) {
	return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

void StatManager::setFileName(char *file_name) {
	snprintf(stat_file_name, sizeof(stat_file_name), "%s", file_name);
}

void StatManager::makeStat() {
	FILE *fileExp;
	char buff_stat[256];

	if ((actual_stats.isMS4) && (strlen(stat_file_name) > 0)) {

		sem_wait(sem_file_stat);

		if(actual_stats.frag_number <= START_FRAGMENT) {
			gettimeofday(t0, NULL);
		}

		if((actual_stats.frag_number > 0) && (actual_stats.frag_number < (int)buffer_vec_size)) {
			buffer_vec[actual_stats.frag_number - 1] = 1;
		}

		//snprintf(buff_stat, sizeof(buff_stat), "%s-%d_%d", experiment_name, getppid(), getpid());
		//snprintf(buff_stat, sizeof(buff_stat), "%s-%s_%dsec_%d",
		//experiment_name, frag->video_name, frag->frag_seconds, getpid());
		snprintf(buff_stat, sizeof(buff_stat), "%s-%s_%dsec",
				stat_file_name, actual_stats.video_name, actual_stats.frag_seconds);

		fileExp = fopen(buff_stat, "a");
		if (fileExp) {
			struct tm *tm_start, *tm_end;
			char buffer_s[32], buffer_e[32];
			//double useconds, millisec;
			//double throughput;

			tm_start = localtime(&actual_stats.start_request_time);
			tm_end = localtime(&actual_stats.end_request_time);

			strftime(buffer_s, 32, "%H:%M:%S", tm_start);
			strftime(buffer_e, 32, "%H:%M:%S", tm_end);

			//double seconds = difftime(frag->end_request_time, frag->start_request_time);

			printf("CIAOCIAO - time sec: %ld, time usec: %ld\n", actual_stats.start_request_timeval.tv_sec,
						actual_stats.start_request_timeval.tv_usec);

			long int useconds_st = actual_stats.start_request_timeval.tv_sec * 1000000.0 + actual_stats.start_request_timeval.tv_usec;
			long int useconds_et = actual_stats.end_request_timeval.tv_sec * 1000000.0 + actual_stats.end_request_timeval.tv_usec;

			//long int useconds = (	(frag->end_request_timeval.tv_sec * 1000000.0 + frag->end_request_timeval.tv_usec)
			//		  	  	  	  -	(frag->start_request_timeval.tv_sec * 1000000.0 + frag->start_request_timeval.tv_usec));

			long int useconds = useconds_et - useconds_st;

			//double millisec = useconds / 1000.0;

			double throughput = (((double) actual_stats.frag_bytesize) / ((double) useconds)) * 1000000.0;

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
							actual_stats.video_name, actual_stats.bps, actual_stats.reply_ok ? 1 : 0,
									actual_stats.frag_seconds, actual_stats.frag_number,
							inet_ntoa(actual_stats.choosed_interface), actual_stats.frag_bytesize,
							useconds_st, useconds_et,
							useconds, throughput, buff_size);

			fwrite(buff_stat, 1, nw, fileExp);

			fclose(fileExp);
		}

		sem_post(sem_file_stat);
	}
}

void StatManager::fillFragmentField(const char *path) {
	int extra_sec;
	char extra_videoname[32];
	char *ptr, *ptr_tmp, *ptr_tmp2;
	char tmp_buff[128];
	char tmp_path[256];
	int stage = 0;

	debug_high("fillFragmentField Path: %s\n\n", path);

	if (strncmp(path, "/ftp/datasets/DASHDataset2014/", 30) != 0) {
		return;
	}

	memset(tmp_path, 0, sizeof(tmp_path));
	strcpy(tmp_path, &path[30]);
	snprintf(tmp_path, sizeof(tmp_path), "%s", &path[30]);

	debug_high("NEW fillFragmentField Path: %s\n\n", tmp_path);

	ptr = strtok (tmp_path, "/");
	while (ptr != NULL) {
		debug_high ("[%d] %s\n", stage, ptr);

		switch (stage) {
		case 0:
			sscanf(ptr, "%s", actual_stats.video_name);

			break;

		case 1:
			snprintf(tmp_buff, sizeof(tmp_buff), "%s", ptr);
			ptr_tmp = strchr (tmp_buff, 's');
			*ptr_tmp = 0;
			//printf ("TMP: %s\n", tmp_buff);
			sscanf(tmp_buff, "%i", &actual_stats.frag_seconds);

			break;

		case 2:
			snprintf(tmp_buff, sizeof(tmp_buff), "%s", ptr);
			ptr_tmp = strchr (tmp_buff, '_');
			*ptr_tmp = 0;
			//printf ("TMP: %s\n", tmp_buff);
			sscanf(tmp_buff, "%s", actual_stats.small_video_name);

			ptr_tmp++;
			ptr_tmp2 = strchr (ptr_tmp, 'b');
			*ptr_tmp2 = 0;
			//printf ("TMP: %s\n", ptr_tmp);
			sscanf(ptr_tmp, "%d", &actual_stats.bps);

			break;

		case 3:

			//	TYPE_BIGBUNNY	0 		-- BigBuckBunny/1sec/bunny_46980bps/BigBuckBunny_1s3.m4s
			//	TYPE_REDBULL	1		-- RedBullPlayStreets/1sec/redbull_101976bps/RedBull2.m4s
			//	TYPE_TEAR		2		-- TearsOfSteel/1sec/tos_101bps/TearsOfSteel_1s_2.m4s
			//	TYPE_VALKAAMA	3		-- Valkaama/2sec/valkaama_46208bps/Valkaama_23.m4s

			if (	(strncmp(tmp_path, "BigBuckBunny", 12) == 0) ||
					(strncmp(tmp_path, "ElephantsDream", 14) == 0) ||
					(strncmp(tmp_path, "OfForestAndMen", 14) == 0) ||
					(strncmp(tmp_path, "TheSwissAccount", 15) == 0) ) {
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
				sscanf(ptr_tmp2, "%d", &actual_stats.frag_number);

			}
			else if (strncmp(tmp_path, "RedBullPlayStreets", 18) == 0) {
				//	TYPE_REDBULL	1		-- RedBullPlayStreets/1sec/redbull_101976bps/RedBull2.m4s
				sscanf(ptr, "RedBull%d.m4s", &actual_stats.frag_number);
			}
			else if (strncmp(tmp_path, "TearsOfSteel", 12) == 0) {
				//	TYPE_TEAR		2		-- TearsOfSteel/1sec/tos_101bps/TearsOfSteel_1s_2.m4s
				sscanf(ptr, "TearsOfSteel_1s_%d.m4s", &actual_stats.frag_number);
			}
			else if (strncmp(tmp_path, "Valkaama", 8) == 0) {
				//	TYPE_VALKAAMA	3		-- Valkaama/2sec/valkaama_46208bps/Valkaama_23.m4s
				sscanf(ptr, "Valkaama_%d.m4s", &actual_stats.frag_number);
			}

			break;
		}

		stage++;
		ptr = strtok (NULL, "/");
	}

	debug_high("FIND from path:\n");
	debug_high("Name: %s \n", actual_stats.video_name);
	debug_high("Small name: %s\n", actual_stats.small_video_name);
	debug_high("bps: %d\n", actual_stats.bps);
	debug_high("seconds: %d\n", actual_stats.frag_seconds);
	debug_high("frag_num: %d\n", actual_stats.frag_number);
}

void StatManager::freeMemory(void) {
	if ((buffer_vec != MAP_FAILED) && (buffer_vec != NULL)) {
		munmap(buffer_vec, buffer_vec_size * sizeof(int));
		buffer_vec = NULL;
		buffer_vec_size = 0;
	}
	if ((t0 != MAP_FAILED) && (t0 != NULL)) {
		munmap(t0, sizeof(struct timeval));
		t0 = NULL;
	}
}

