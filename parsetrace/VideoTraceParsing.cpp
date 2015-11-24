//============================================================================
// Name        : VideoTraceParsing.cpp
// Author      : Angelo Trotta
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <list>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fstream>

using namespace std;

typedef struct fragment_in {
	char video_name[32];
	int frag_seconds;
	int bps;
	int frag_number;
	int frag_bytesize;
	struct in_addr choosed_interface;
	long long start_request_time;
	long long end_request_time;
	long long tot_useconds;
	int buff;
	double throughput;
	bool reply_ok;
	int isCustom;
	char algo;
	int mode;
} fragment_in_t;

bool compare_seg_number (const fragment_in_t& first, const fragment_in_t& second) {
  return first.frag_number < second.frag_number;
}
bool compare_start_req_time (const fragment_in_t& first, const fragment_in_t& second) {
  return first.start_request_time < second.start_request_time;
}
bool compare_end_req_time (const fragment_in_t& first, const fragment_in_t& second) {
  return first.end_request_time < second.end_request_time;
}

int main(int argc, char* argv[]) {
	FILE *inputFile, *stat_file;
	list<fragment_in_t> frag_list;
	list<fragment_in_t>::iterator frag_list_it;
	list<std::string> file_list;
	list<std::string>::iterator file_list_it;
	list<std::string> file_final_list;
	list<std::string>::iterator file_final_list_it;
	char big_buff[512];

	cout << "START!!!" << endl;

	if (argc != 3) {
		perror("USAGE: VideoTraceParsing input_file stat_file");
		return EXIT_FAILURE;
	}

	//inputFile = fopen(argv[1], "r");
	//stat_file = fopen(argv[2], "w");

	
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (argv[1])) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {

			//printf ("%s\n", ent->d_name);
			if(strcmp(ent->d_name, "..") && strcmp(ent->d_name, "."))
				file_list.push_back(std::string(ent->d_name));
		}
		closedir (dir);

		for (file_list_it = file_list.begin(); file_list_it != file_list.end(); file_list_it++) {
			std::string act_file = *file_list_it;
			int pos = act_file.find("1sec_");
			std::string tmp = act_file.substr(0, pos + 5);
			
			//copy in new file
			//printf ("Found 2 file %s %s\n", act_file.c_str(), act_second_file.c_str());
		    fstream combined_file;

     		combined_file.open(tmp.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);


      		// If file does not exist, Create new file
			if (!combined_file ){
				combined_file.open(tmp.c_str(),  fstream::in | fstream::out | fstream::trunc);
				combined_file <<"\n";
				combined_file.close();
	     		combined_file.open(tmp.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);


			} 

			// use existing file
			std::ifstream file1(string(argv[1]+act_file).c_str()) ;
			//std::ofstream combined_file(tmp.c_str()) ;
			combined_file << file1.rdbuf();
			combined_file.close();


			file_final_list.push_back(string(tmp));
			
		}
		printf("FINISH DIR SCAN\n");
	} else {
		/* could not open directory */
		perror ("");
		return EXIT_FAILURE;
	}






	// file syntax
	// videoname bitrate ok? sec_per_frag frag_number INTERFACE_IP frag_byte start_time end_time useconds throughput(B/s)



	for (file_final_list_it = file_final_list.begin(); file_final_list_it != file_final_list.end(); file_final_list_it++) {
		std::string act_file = *file_final_list_it;
		inputFile = fopen(act_file.c_str(), "r");
		frag_list.clear();
		printf ("%s\n", act_file.c_str());

		if (inputFile) {
			char buff_line [256];
			do {
				if ( fgets ( buff_line, sizeof(buff_line), inputFile ) ) {
					fragment_in_t read_frag;
					char dummy_ip[20];
					int dummy_ok;

					//printf("LINE: %s", buff_line);

					sscanf(buff_line, "%s\t%d\t%d\t%d\t%d\t%s\t%d\t%lld\t%lld\t%lld\t%lf\t%d\t%d\t%c\t%d\n",
							read_frag.video_name, &read_frag.bps, &dummy_ok, &read_frag.frag_seconds,
							&read_frag.frag_number, dummy_ip, &read_frag.frag_bytesize,
							&read_frag.start_request_time, &read_frag.end_request_time,
							&read_frag.tot_useconds, &read_frag.throughput, &read_frag.buff,
							&read_frag.isCustom,&read_frag.algo,&read_frag.mode);

					inet_aton(dummy_ip, &read_frag.choosed_interface);

					read_frag.reply_ok = dummy_ok > 0;

					/*printf("READ: %s\t%d\t%d\t%d\t%d\t%s\t%d\t%ld\t%ld\t%ld\t%lf\n",
							read_frag.video_name, read_frag.bps, dummy_ok, read_frag.frag_seconds,
							read_frag.frag_number, inet_ntoa(read_frag.choosed_interface), read_frag.frag_bytesize,
							read_frag.start_request_time, read_frag.end_request_time,
							read_frag.tot_useconds, read_frag.throughput);*/

					frag_list.push_back(read_frag);
				}
				else {
					break;
				}
			} while(true);
			//return 1;
			fclose(inputFile);
			remove(act_file.c_str());
		}
		else {
			//perror("Input file not found");
			//return EXIT_FAILURE;
			continue;
		}

		stat_file = fopen(string(argv[2]+act_file).c_str(), "w");
		long long last_start_req = 0;
		int time_video_tot_sec = 0;
		long long video_req_time_tot_usec = 0;
		long long first_start_req_time = 0;
		long long first_reply_req_time = 0;
		long long last_req_time = 0;
		bool setup = true;

		long long pause_offset = 0;

		frag_list.sort(compare_end_req_time);
		first_reply_req_time = frag_list.begin()->end_request_time;

		// first row
		int n_big_buff_ttt = snprintf (big_buff, sizeof(big_buff),
				"#1(Time_start)\t2(Name)\t3(bps)\t4(received?)\t5(sec)\t6(frag_num)\t7(interface)\t"
				"8(size_byte)\t9(size_bit)\t10(start_req_t)\t11(end_req_t)\t12(send_t_sec)\t"
				"13(?)\t14(throug_bytesec)\t15(throug_bitsec)\t16(pause)\t17(buff)\n");

		//printf("%s", big_buff);

		if (stat_file) {
			fwrite(big_buff, 1, n_big_buff_ttt, stat_file);
		}

		frag_list.sort(compare_seg_number);

		list<fragment_in_t> frag_list_noDUP;

		for (frag_list_it = frag_list.begin(); frag_list_it != frag_list.end(); frag_list_it++) {
			fragment_in_t *act_frag = &(*frag_list_it);
			//int ii = 0;
			list<fragment_in_t *> duplicate_list;

			list<fragment_in_t>::iterator frag_list_it_dup = frag_list_it;

			duplicate_list.push_back(&(*frag_list_it_dup));
			frag_list_it_dup++;


			while ((frag_list_it_dup != frag_list.end()) && ((*frag_list_it_dup).frag_number == act_frag->frag_number) ) {
				if((*frag_list_it_dup).mode != 2){
					duplicate_list.push_back(&(*frag_list_it_dup));
				}
				frag_list_it_dup++;
				frag_list_it++;
			}
			
			printf ("Trovato %d con %d occorrenze\n", act_frag->frag_number, duplicate_list.size());

			if(duplicate_list.size() > 1) {
				fragment_in_t * maxBPS = NULL;
				fragment_in_t * minTime = NULL;
				int counter_1 = 0;
				int counter_3 = 0;

				for (list<fragment_in_t *>::iterator dup_it = duplicate_list.begin(); dup_it != duplicate_list.end(); dup_it++) {
					fragment_in_t *act_dup = *dup_it;

					printf ("Occorrenza %d con %d \n", act_dup->bps, act_dup->mode);
					fflush(stdout);
					if(act_dup->mode == 2)
						continue;
					/*else if(act_dup->mode == 1){
						frag_list_noDUP.push_back(*act_dup);
					}*/
					else if ((maxBPS == NULL) || (maxBPS->bps < act_dup->bps)) {
						long long diff_from_start = act_dup->end_request_time - first_reply_req_time;
						long long now_playing = act_dup->frag_seconds * act_dup->frag_number;

						if ((minTime == NULL) || (minTime->end_request_time > act_dup->end_request_time)) {
							minTime = act_dup;
						}

						if ((now_playing + 3) < diff_from_start) {
							maxBPS = act_dup;
						}
					}
				}

				if (maxBPS == NULL) {
					maxBPS = minTime;
				}

				if(maxBPS != NULL){
					printf ("Trovato %d con %d occorrenze\n", maxBPS->frag_number, maxBPS->bps);
					frag_list_noDUP.push_back(*maxBPS);
				}
			}
			else {
				frag_list_noDUP.push_back(*act_frag);
			}
			duplicate_list.clear();

		}
		
		//for (frag_list_it = frag_list.begin(); frag_list_it != frag_list.end(); frag_list_it++) {
		for (frag_list_it = frag_list_noDUP.begin(); frag_list_it != frag_list_noDUP.end(); frag_list_it++) {
			fragment_in_t *act_frag = &(*frag_list_it);
			double diff;
			long long actual_time, ideal_time, actual_pause;
			double buff_s;

			if (setup) {
				diff = 0;
				first_start_req_time = act_frag->start_request_time;
				//first_reply_req_time = act_frag->end_request_time;

				setup = false;
			}
			else {
				diff = ((double) (act_frag->start_request_time - last_start_req)) / 1000000.0;
			}

			if(act_frag->frag_number <= 4) {
				first_reply_req_time = act_frag->end_request_time;
			}

			actual_time = act_frag->end_request_time - first_reply_req_time;
			ideal_time = (act_frag->frag_number * act_frag->frag_seconds * 1000000.0) + pause_offset;

			if (ideal_time > actual_time) {
				buff_s = (ideal_time - actual_time) / 1000000.0;
			}
			else {
				buff_s = 0;
			}

			if(actual_time <= ideal_time) {
				// ok, no block in video playing
				actual_pause = 0;
			}
			else {
				actual_pause = actual_time - ideal_time;
				pause_offset += actual_pause;
			}

			//printf("act:%ld ideal:%ld pause:%ld offset:%ld\n", actual_time, ideal_time, actual_pause, pause_offset);

			last_req_time = act_frag->end_request_time;
			last_start_req = act_frag->start_request_time;

			time_video_tot_sec += act_frag->frag_seconds;

			int n_big_buff = snprintf (big_buff, sizeof(big_buff),
					"%lf\t%s\t%08d\t%d\t%d\t%d\t%s\t%d\t%d\t%lld\t%lld\t%lf\t%lld\t%lf\t%lf\t%lf\t%lf\t%d\n",
					((double) actual_time) / 1000000.0,
					act_frag->video_name, act_frag->bps, act_frag->reply_ok, act_frag->frag_seconds,
					act_frag->frag_number, inet_ntoa(act_frag->choosed_interface),
					act_frag->frag_bytesize, act_frag->frag_bytesize * 8,
					act_frag->start_request_time, act_frag->end_request_time, diff,
					act_frag->tot_useconds, act_frag->throughput, act_frag->throughput * 8,
					((double) actual_pause) / 1000000.0, buff_s,
					act_frag->mode);

			//printf("%s", big_buff);

			if (stat_file) {
				fwrite(big_buff, 1, n_big_buff, stat_file);
			}

			/*printf("%lf\t%s\t%d\t%d\t%d\t%d\t%s\t%d\t%ld\t%ld\t%lf\t%ld\t%lf\n",
					((double) (act_frag->start_request_time - first_req_time)) / 1000000.0,
					act_frag->video_name, act_frag->bps, act_frag->reply_ok, act_frag->frag_seconds,
					act_frag->frag_number, inet_ntoa(act_frag->choosed_interface), act_frag->frag_bytesize,
					act_frag->start_request_time, act_frag->end_request_time, diff,
					act_frag->tot_useconds, act_frag->throughput);*/
		}

		video_req_time_tot_usec = last_req_time - first_start_req_time;

		printf("Video duration: %d; download duration: %lf\n",
				time_video_tot_sec, ((double) video_req_time_tot_usec) / 1000000.0);
	/*
		fragment_in_t *tmp_frag = NULL;
		int last_frag_num = -1;

		for (frag_list_it = frag_list.begin(); frag_list_it != frag_list.end(); frag_list_it++) {
			fragment_in_t *act_frag = &(*frag_list_it);

			if (tmp_frag == NULL) {
				tmp_frag = act_frag;
			}
			else {
				if (act_frag->frag_number > last_frag_num) {
					last_frag_num = act_frag->frag_number;

					int n_big_buff = snprintf (big_buff, sizeof(big_buff),
							"%lf\t%s\t%08d\t%d\t%d\t%d\t%s\t%d\t%d\t%ld\t%ld\t%lf\t%ld\t%lf\t%lf\t%d\n",
							((double) (tmp_frag->end_request_time - first_reply_req_time)) / 1000000.0,
							tmp_frag->video_name, tmp_frag->bps, tmp_frag->reply_ok, tmp_frag->frag_seconds,
							tmp_frag->frag_number, inet_ntoa(tmp_frag->choosed_interface),
							tmp_frag->frag_bytesize, tmp_frag->frag_bytesize * 8,
							tmp_frag->start_request_time, tmp_frag->end_request_time,
							tmp_frag->tot_useconds, tmp_frag->throughput, tmp_frag->throughput * 8, tmp_frag->buff);

					tmp_frag = act_frag;

					if (stat_file) {
						//fwrite(big_buff, 1, n_big_buff, stat_file);
					}
				}
				else {
					if (act_frag->bps > tmp_frag->bps) {
						tmp_frag = act_frag;
					}
				}
			}
		}*/


		if (stat_file) {
			fclose (stat_file);
		}
		frag_list.clear();
		frag_list_noDUP.clear();
		//break;
		//cout << "END!!!" << endl;
	}
	return EXIT_SUCCESS;
}
