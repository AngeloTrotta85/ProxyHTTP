/*
 * RequestManager.cpp
 *
 *  Created on: 20/apr/2015
 *      Author: angelo
 */

#include "RequestManager.h"


RequestManager::~RequestManager() { }

RequestManager::RequestManager() {
	init();
}

void RequestManager::init(void) {
	loaded = false;
	isget = false;
	mpeg_dash = MPEGDASH_NO_TYPE;
	server_port = 0;
	server_addr = 0;

	memset(buff_req, 0, sizeof(buff_req));
	memset(path_name, 0, sizeof(path_name));
	memset(host_name, 0, sizeof(host_name));
}

bool RequestManager::isLoaded(void) {
	return loaded;
}

bool RequestManager::isGET(void) {
	return isget;
}

bool RequestManager::isMPEGDASHreq(void) {
	return (mpeg_dash != MPEGDASH_NO_TYPE);
}

bool RequestManager::isMPEGDASH_M4S(void) {
	return (mpeg_dash == MPEGDASH_FRAME);
}

bool RequestManager::isManifest(void) {
	return (mpeg_dash == MPEGDASH_MANIFEST);
}

bool RequestManager::isInit(void) {
	return (mpeg_dash == MPEGDASH_INIT);
}

bool RequestManager::load_req(char *str_req, int size_str) {

	char *start_row, *end_row, *tmp_ptr, *tmp_start;
	bool ris = true;

	// init variables
	init();

	// copy the entire request
	strncpy(buff_req, str_req, std::min(size_str, ((int) sizeof(buff_req))));

	//TODO
	int reqStr_len = strlen(buff_req);
	sprintf(&buff_req[reqStr_len],"Connection: close\r\n\r\n");

	start_row = end_row = buff_req;

	debug_high("\n***REQEST TO PARSE: \n%s\n", buff_req);

	if (strncmp(buff_req, "GET ", 4) == 0) {
		isget = true;
		while ((end_row = strstr(start_row, "\r\n")) != NULL) {
			int row_len = end_row - start_row;

			printf("STRING TO PARSE %d: %s\n",row_len, std::string(start_row, row_len).c_str());
			fflush(stdout);
			if(row_len == 0 ) break;
			if (strncmp(start_row, "GET ", 4) == 0) {

				req_fields[std::string("GET")] = std::string(start_row, 4, row_len - 4);

				// parse the field host post and path
				server_port = 80;
				memset(host_name, 0, sizeof(host_name));
				memset(path_name, 0, sizeof(path_name));

				tmp_start = start_row + 4;
				tmp_ptr = strchr(tmp_start, ' ');

				if (tmp_ptr != NULL) {
					int size_tot_path = tmp_ptr - tmp_start;

					// skip "http://"
					tmp_ptr = strstr(tmp_start, "//");
					if ((tmp_ptr != NULL) && ((tmp_ptr - tmp_start) < size_tot_path)) {

						size_tot_path -= (tmp_ptr + 2) - tmp_start;

						tmp_start = tmp_ptr + 2;
					}

					// catch the port and the host-name
					tmp_ptr = strstr(tmp_start, ":");
					if ((tmp_ptr != NULL) && ((tmp_ptr - tmp_start) < size_tot_path)) {
						*tmp_ptr = 0;
						sscanf(tmp_start, "%s", host_name);

						*tmp_ptr = ':';
						sscanf(tmp_ptr+1, "%d/%*s", &server_port);
					}
					else {
						tmp_ptr = strstr(tmp_start, "/");
						if (tmp_ptr != NULL) {
							*tmp_ptr = 0;
							sscanf(tmp_start, "%s", host_name);
							*tmp_ptr = '/';
						}
						else {
							sscanf(tmp_start, "%s", host_name);
						}
					}

					//locate the path
					tmp_ptr = strstr(tmp_start, "/");
					if ((tmp_ptr != NULL) && ((tmp_ptr - tmp_start) < size_tot_path)) {
						sscanf(tmp_ptr, "%s %*s", path_name);
					}
				}
			}
			else  {

				tmp_ptr = strstr(start_row, ": ");
				// check the format: FIELD: VALUE
				if ((tmp_ptr != NULL)) {
					int filed_str_size = tmp_ptr - start_row;
					
					s11 = std::string(start_row, filed_str_size);
					s12 = std::string(start_row, filed_str_size + 2, row_len - (filed_str_size + 2));
					req_fields[std::string(start_row, filed_str_size)] = std::string(start_row, filed_str_size + 2, row_len - (filed_str_size + 2));
					
					// Only for test, analytics packet will fuck all
					std::size_t found = s12.find("ma1-r.analytics.edgesuite.net");					
				  	if (found!=std::string::npos){
					    std::cout << "Trovato analytics prova ad andare avanti: " << found << '\n';
				    	return false;
				  	}
				}
			}

			start_row = end_row + 2;
			if ((start_row - buff_req) >= size_str) break;	// I'm at the end but (maybe) I didn't realize it
		}
		printf("CHECK MPHDASH: %s\n", path_name);
		fflush(stdout);
		// check if it is an MPEG-DASH packet
		int path_len = strlen(path_name);
		if (path_len > 0) {
			if (strncmp(&path_name[path_len - 4], ".m4s", 4) == 0) {
				mpeg_dash = MPEGDASH_FRAME;
			}
			else if (strncmp(&path_name[path_len - 4], ".mpd", 4) == 0) {
				mpeg_dash = MPEGDASH_MANIFEST;
			}
			else if (strncmp(&path_name[path_len - 4], ".mp4", 4) == 0) {
				mpeg_dash = MPEGDASH_INIT;
			}
		}
		printf("CHECK HOSTNAME: %s\n", host_name);
		fflush(stdout);
		if (strlen(host_name) > 0) {
			struct hostent* host = gethostbyname(host_name); // get host informations
			if(host == NULL){
				printf("CHECK HOSTNAME: NULL HOST \n");
				fflush(stdout);
				//bcopy((char*)host->h_addr, (char*)&server_addr, host->h_length);
				//inet_aton("dfsff", server_addr);
				server_addr = inet_addr("2.228.46.114");
			}else{
				printf("CHECK HOSTNAME2: %s\n", host->h_addr);
				fflush(stdout);
				bcopy((char*)host->h_addr, (char*)&server_addr, host->h_length);
			}

		}


		debug_high("NEW REQ RECEIVED!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

		debug_high("FIELDS found:\n");
		for (std::map <std::string, std::string>::iterator it = req_fields.begin();  it != req_fields.end();  it++) {
			debug_high("%s -> %s\n", it->first.c_str(), it->second.c_str());
		}

		debug_low("\n");
		debug_low("GET: %s\n", req_fields[std::string("GET")].c_str());
		debug_low("HOST: %s - ", host_name);
		debug_low("PORT: %d - ", server_port);
		debug_low("PATH: %s\n", path_name);

		switch(mpeg_dash){
		case MPEGDASH_FRAME:
			debug_medium("GET type: MPEGDASH_FRAME\n");
			break;
		case MPEGDASH_MANIFEST:
			debug_medium("GET type: MPEGDASH_MANIFEST\n");
			break;
		case MPEGDASH_INIT:
			debug_medium("GET type: MPEGDASH_INIT\n");
			break;
		case MPEGDASH_NO_TYPE:
			debug_medium("GET type: NOT MPEG-DASH\n");
			break;
		}

		debug_high("******************************************\n");

	}
	else {
		// this is not a get... take the address and the port of the server in another way
		debug_low("Not a get: \n%s\n", buff_req);
		ris = false;
	}

	loaded = true;

	return ris;
}
