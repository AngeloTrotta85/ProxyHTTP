#include "InterfacesManager.h"
#include "StatManager.h"
#include "TransferManager.h"

static long long timevaldiff_usec(struct timeval *start, struct timeval *end) {
   return (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
}

void TransferManager::forkAndDownloadFrame(int frameNumber){


}

void TransferManager::customGetRequest(int frameNumber,RequestManager rm, char *customGET){

   char text[4096];
   strcpy(text, rm.getCopyOfGET());

   std::string str = std::string(text);

   size_t index = 0;
   /* Locate the substring to replace. */
   index = str.find(".m4s", index);
   if (index == std::string::npos)
	   return;

   int len_sub = 0;
   while(len_sub<10){
      len_sub++;

      char ptr = text[index-len_sub];
      if(ptr == 's'){
         break;
      }
   }

   /* Make the replacement. */
   char c[15];
   sprintf(c, "%d", frameNumber);

   int fine = index;
   int inizio = fine-(len_sub-1);
   int nChar = fine - inizio;
   str.replace(inizio, nChar, c);

   strcpy(customGET, str.c_str());
}

/*---------- Download From server in normal Mode -----------*/
bool TransferManager::manageRequest(RequestManager rm,struct sockaddr_in *if_to_use,int socketfd_client, VideoInfo* videoInfo, int counter) {

   int sockfd_VideoServer;

   printf("CHILD %d:: start sending the GET to the server using %s:%d\n", counter, inet_ntoa(if_to_use->sin_addr), rm.getServerPort());

   StatManager::getInstance().actual_stats.isMS4 = true;
   StatManager::getInstance().actual_stats.choosed_interface.s_addr = if_to_use->sin_addr.s_addr;
   StatManager::getInstance().actual_stats.reply_ok = false;
   StatManager::getInstance().fillFragmentField(rm.getPathName());

	if (TransferManager::sendGETtoDest(rm, sockfd_VideoServer, if_to_use)) {
		TransferManager::manageTransferFromDestToClient(if_to_use, rm, sockfd_VideoServer, socketfd_client,  videoInfo);
		close (sockfd_VideoServer);
		sockfd_VideoServer = -1;
	}
	else {
		debug_low("Error sending request to the destination\n");
		return false;
	}

	return true;
}

bool TransferManager::sendGETtoDest(RequestManager rm,int &sockfd_VideoServer, struct sockaddr_in *if_to_bind) {
   struct sockaddr_in host_addr;

   host_addr.sin_port = htons(rm.getServerPort());
   host_addr.sin_family=AF_INET;
   host_addr.sin_addr.s_addr = rm.getServerAddr();

   //printf("CHILD:: start sending the GET to the server using %s:%d\n", inet_ntoa(if_to_bind->sin_addr), rm.getServerPort());

   sockfd_VideoServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd_VideoServer < 0) {
      perror("Error on creating socket for the server connection");
      return false;
   }
   else {
      int risBind = 0;  // if I don't have to execute the bind '0' will be OK
      if_to_bind->sin_family = AF_INET;
      if_to_bind->sin_port = htons(0);
      if_to_bind->sin_addr.s_addr = INADDR_ANY;
      if (if_to_bind != NULL) {
         risBind = bind (sockfd_VideoServer, (struct sockaddr *)if_to_bind, sizeof(struct sockaddr_in));
      }
      if (risBind == 0) {

         if (connect(sockfd_VideoServer, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) < 0) {
            perror("Error in connecting to remote server");
            return false;
         }
         else {

            int n_send = send(sockfd_VideoServer, rm.getCopyOfGET(), strlen(rm.getCopyOfGET()), 0);
            if (n_send < 0) {
               perror("Error writing to server socket");
               return false;
            }
         }
      }
      else {
         perror("Error binding");
         return false;
      }
   }

   //start STATS
   time(&StatManager::getInstance().actual_stats.start_request_time);
   gettimeofday(&(StatManager::getInstance().actual_stats.start_request_timeval), NULL);

   return true;
}

void TransferManager::manageTransferFromDestToClient(struct sockaddr_in *if_used, RequestManager rm, int sockfd_VideoServer, int socketfd_VideoClient,  VideoInfo* videoInfo) {
   int n_recv = 0;
   int n_tot_recv = 0;
   int block_stat_recv = 0;
   struct timeval time_st;
   char buffer[16384]; // Check this

   gettimeofday(&time_st, NULL);

   FILE * pFile = NULL;
   char *mName = "manifest.mpd";
   if(rm.isManifest()){
      pFile = fopen (mName, "w+");
   }
   do {
      memset(buffer, 0, sizeof(buffer));
      n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);

      if (n_recv > 0) {
         int n_sent = send(socketfd_VideoClient, buffer, n_recv, 0);
         //printf("CHILD:: provo ad inviare al client: %d Ricevuti: %d Inviati: %d \n", new_sockfd_VideoClient, n_recv, n_sent);
         //printf("CHILD:: pacchetto da inviare: \n %s\n\n\n",buffer );
         /*If is manifest append it on file*/
         if(rm.isManifest()){

            std::string s = buffer;
            //debug_medium("FILE: Grandezza stringhe!%lo  \n", s.size());
            size_t found = s.find("\r\n\r\n");
            int inizio = (int) found;
            size_t errorFound = s.find("<!DOCTYPE");
            int errorInt = (int) errorFound;
            std::string sub = s;
            //debug_medium("FILE: Inizio xml!%d  %d\n", inizio, errorFound);
            if(errorInt > 0){
               s = s.substr(0,errorFound);
            }
            if(inizio > 0){
               sub = s.substr(found);
            }
            fwrite (sub.c_str(), sub.size(), 1, pFile);

         }
         if(n_sent < 0) {
            perror("Error sending to client the server reply");
         }
         else if(n_sent == 0) {
            debug_low("Connection closed by the client while sending back the message!?!?!\n");
         }
         else {

            n_tot_recv += n_recv;
            block_stat_recv += n_recv;

            debug_high("\r%d of %d  ", n_recv, n_tot_recv);

            StatManager::getInstance().actual_stats.reply_ok = true;

            /*if (block_stat_recv >= byte_update) {
               gettimeofday(&time_en, NULL);

               InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));

               block_stat_recv = 0;
               gettimeofday(&time_st, NULL);
            }*/

         }

      }
      else if (n_recv == 0) {
         debug_high("connection closed by the server\n");
      }
      else {
         perror("Error receiving from server while forwarding");
      }

   } while (n_recv > 0);
   /*If is a manifest close file and read manifest*/
   if(rm.isManifest()){
      fclose (pFile);
      TransferManager::settingsManifestParams(mName, videoInfo);
   }

   StatManager::getInstance().actual_stats.frag_bytesize = n_tot_recv;
   time(&StatManager::getInstance().actual_stats.end_request_time);
   gettimeofday(&StatManager::getInstance().actual_stats.end_request_timeval, NULL);

   // debug_high("\n");

   // if (n_tot_recv == block_stat_recv) {   // never made stats (packet size less then "byte_update")
   //    gettimeofday(&time_en, NULL);
   //    InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
   // }

   StatManager::getInstance().makeStat();
}

void TransferManager::settingsManifestParams(char *mName,VideoInfo *videoInfo){
   std::string source = mName;
   pugi::xml_document document;
   pugi::xml_parse_result result = document.load_file(source.c_str());
   if (result)
   {
      pugi::xml_node doc = document.child("MPD").child("Period");

      std::string duration = doc.attribute("duration").value();
      std::string durationSegment = doc.child("AdaptationSet").child("SegmentTemplate").attribute("duration").value();
      std::string media = doc.child("AdaptationSet").child("SegmentTemplate").attribute("media").value();
      std::string timescale = doc.child("AdaptationSet").child("SegmentTemplate").attribute("timescale").value();
      std::string initSegment = doc.child("AdaptationSet").child("SegmentTemplate").attribute("initialization").value();

      videoInfo->init(duration, durationSegment, media, timescale, initSegment);
   }
 }
/*----------END Download From server in normal Mode -----------*/

/*----------Download From server custom Frame -----------*/
bool TransferManager::getVideoFrame(const char* GET, RequestManager rm, struct sockaddr_in *if_used, int &localServerSocket){

   struct sockaddr_in host_addr;
   //Random Port???
   host_addr.sin_port = htons(rm.getServerPort());
   host_addr.sin_family=AF_INET;
   host_addr.sin_addr.s_addr = rm.getServerAddr();

   printf("CUSTOM CHILD:: start sending the GET to the server using %s:%d\n", inet_ntoa(if_used->sin_addr), rm.getServerPort());

   localServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (localServerSocket < 0) {
      perror("Error on creating socket for the server connection");
      return false;
   }
   else {
      int risBind = 0;  // if I don't have to execute the bind '0' will be OK

      if (if_used != NULL) {
         risBind = bind (localServerSocket, (struct sockaddr *)if_used, sizeof(struct sockaddr_in));
      }
      if (risBind == 0) {

         if (connect(localServerSocket, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) < 0) {
            perror("Error in connecting to remote server");
            return false;
         }
         else {

            int n_send = send(localServerSocket, GET, strlen(GET), 0);
            if (n_send < 0) {
               perror("Error writing to server socket");
               return false;
            }
         }
      }
      else {
         perror("Error binding");
         return false;
      }
   }
   return true;
}

void TransferManager::manageTransferFromDest(int localServerSocket, char* filename) {
   int n_recv = 0;
   int n_tot_recv = 0;
   int block_stat_recv = 0;
   char localBuffer[16384];
   //getsockname(new_sockfd_VideoClient, &local_address, &addr_size);

   //debug_high("Receiving from server and sending to the client %d %d \n", ipLocal,portLocal);

   FILE * pFile = NULL;
   pFile = fopen (filename, "w+");

   do {
      memset(localBuffer, 0, sizeof(localBuffer));
      n_recv = recv(localServerSocket, localBuffer, sizeof(localBuffer), 0);

      if (n_recv > 0) {
         //printf("CUSTOM CHILD:: Ricevuto dal client: %s\n", localBuffer);
         //char* separator = "\nEND\n";
         fwrite (localBuffer, n_recv, 1, pFile);
        // fwrite (separator, sizeof(separator), 1, pFile);

/*       tempPacket=(struct packet *)malloc(sizeof(struct packet));
         strcpy(tempPacket->buffer,buffer);
         tempPacket->next=NULL;
         ptr->next=tempPacket;
         ptr = tempPacket;*/

         n_tot_recv += n_recv;
         block_stat_recv += n_recv;
      }
      else if (n_recv == 0) {
         debug_high("connection closed by the server\n");
      }
      else {
         perror("Error receiving from server while forwarding");
      }

   } while (n_recv > 0);
   fclose(pFile);
}
/*----------END Download From server custom Frame -----------*/

void TransferManager::manageTransferOnStatUpdate(struct sockaddr_in *if_used, int sockfd_VideoServer) {
   int n_recv = 0;
   int n_tot_recv = 0;
   int block_stat_recv = 0;
   struct timeval time_st, time_en;
   char buffer[16384];

   gettimeofday(&time_st, NULL);

   debug_high("Receiving from client ONLY for statistics\n");

   do {
      memset(buffer, 0, sizeof(buffer));
      n_recv = recv(sockfd_VideoServer, buffer, sizeof(buffer), 0);

      if (n_recv > 0) {

         n_tot_recv += n_recv;
         block_stat_recv += n_recv;

         if (block_stat_recv > BLOCK_SIZE_STATS_BYTE) {
            gettimeofday(&time_en, NULL);

            InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));

            block_stat_recv = 0;
            gettimeofday(&time_st, NULL);
         }

      }
      else if (n_recv == 0) {
         //debug_high("connection closed by the server\n");
      }
      else {
         perror("Error receiving from client while forwarding");
      }

   } while (n_recv > 0);

   if (n_tot_recv == block_stat_recv) {   // never made stats (packet size less then "byte_update")
      gettimeofday(&time_en, NULL);
      InterfacesManager::getInstance().updateInterfaceStats(if_used, block_stat_recv, timevaldiff_usec(&time_st, &time_en));
   }
}

void TransferManager::sendToClientFromFile(int clientSocket, const char* file){

   char* localBuffer;
   int fsize = 0;
   FILE * pFile = NULL;

   //printf("CUSTOM CHILD:: Mando al client dal file %s \n", file);
   fflush(stdout);
   pFile = fopen(file, "r");


   if (pFile == NULL)
   {
       printf("CHILD:: File not found!\n");
       return ;
   }
   else
   {
       printf("CHILD:: Found file \n");

       fseek(pFile, 0, SEEK_END);
       fsize = ftell(pFile);
       rewind(pFile);
   }


   localBuffer = (char*) malloc (sizeof(char)*fsize);
   //memset(&localBuffer, 0, sizeof(localBuffer));

   while (1) //TODO: Inprove memory usage
   {
       int bytes_read = fread(localBuffer, sizeof(char), sizeof(localBuffer), pFile);
       if (bytes_read == 0)
           break;

       if (bytes_read < 0)
       {
           perror("ERROR reading from file");
       }

       char *p = localBuffer;
       while (bytes_read > 0)
       {
           int bytes_written = send(clientSocket, localBuffer, bytes_read, 0);
           if (bytes_written <= 0)
           {
               perror("ERROR writing to socket\n");
           }
           bytes_read -= bytes_written;
           p += bytes_written;
       }
   }

   printf("CHILD:: Done Sending frame from File!\n");

   fclose(pFile);
   return ;

}
