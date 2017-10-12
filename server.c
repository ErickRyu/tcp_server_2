/* 
   A simple server in the internet domain using TCP
   Usage:./server port (E.g. ./server 10000 )
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void *pthread_read_and_write(void *arg);
int writeToClient(int newsockfd, char* msg);
void sendError(int newsockfd);
void sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType);
void requestHandler(int newsockfd, char *reqMsg);
int main(int argc, char *argv[])
{
    int sockfd, newsockfd; //descriptors rturn from socket and accept system calls
     int portno; // port number
     socklen_t clilen;
     
     
     /*sockaddr_in: Structure Containing an Internet Address*/
     struct sockaddr_in serv_addr, cli_addr;
     
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     /*Create a new socket
       AF_INET: Address Domain is Internet 
       SOCK_STREAM: Socket Type is STREAM Socket */
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]); //atoi converts from String to Integer
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
     serv_addr.sin_port = htons(portno); //convert from host to network byte order
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
              error("ERROR on binding");
     
     listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5
     
     clilen = sizeof(cli_addr);
     /*accept function: 
       1) Block until a new connection is established
       2) the new socket descriptor will be used for subsequent communication with the newly connected client.
     */

     pthread_t pthread;
     while(1){
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0) 
             error("ERROR on accept");
         pthread_create(&pthread, NULL, *pthread_read_and_write, (void *)(intptr_t)newsockfd);
         pthread_detach(pthread);
     }
     close(sockfd);
     close(newsockfd);
     
     return 0; 
}

void *pthread_read_and_write(void *arg){
    int newsockfd = (int)arg;
    char reqMsg[500];
    int n;

    bzero(reqMsg,500);
    n = read(newsockfd,reqMsg,499); //Read is a block function. It will read at most 255 bytes
    if (n < 0) error("ERROR reading from socket");
    printf("========Request Message======\n%s\n",reqMsg);
    requestHandler(newsockfd, reqMsg);
    printf("=============================\n");

    return NULL;
}
void requestHandler(int newsockfd, char *reqMsg){
    char file[100];
    char *method = strtok(reqMsg, " /");
    strcpy(file, strtok(NULL, " /"));
    char tmpFileName[100];
    strcpy(tmpFileName, file);
    strtok(tmpFileName, ".");
    char *extension = strtok(NULL, ".");
    printf("method : %s\n", method);
    printf("file : %s\n", file);
    printf("extension : %s\n", extension);
    if(strcmp(method, "GET") == 0 || strcmp(method, "get") == 0){

    }else{
        sendError(newsockfd);
        return;
    }
    if(strcmp(file, "HTTP") == 0 || strcmp(file, "http") == 0){
        strcpy(file , "index.html");
    }
    printf("compare success\n");
    long fsize;
    char type[20];
    
    if(extension == NULL){
        strcpy(type, "text/html");
    }else if(strcmp(extension, "jpeg") == 0){
        strcpy(type,"image/jpeg");
    }else if(strcmp(extension, "gif") == 0){
        strcpy(type,"image/gif");
    }else if(strcmp(extension, "mp3") == 0){
        strcpy(type, "audio/mpeg");
    }else if(strcmp(extension, "pdf") == 0){
        strcpy(type, "application/pdf");
    }else{
        strcpy(type, "text/plain");
    }
    printf("compare success\n");
    printf("type : %s\n", type);
    /*
    FILE *fp = fopen(file, "rb");
    if(fp == NULL){
        sendError(newsockfd);
        return;
    }
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    char *msg = (char*)malloc(fsize);
    fread(msg, fsize, 1, fp);
    fclose(fp);
    */
    char rcvBuf[BUFSIZ+1];
    int fd;
    printf("reading file...\n");
    if((fd = open(file, O_RDONLY)) <0 ){
        printf("sending error...\n");
        sendError(newsockfd);
        printf("send error OK\n");
        return;
    }
    printf("open fd OK\n");

    //printf("fsize : %ld\n", fsize);
    //printf("strlen(msg) : %ld\n", strlen(msg));

    char *httpMsgOK = "200 OK";
    printf("sending header...\n");
    sendResponseHeader(newsockfd, httpMsgOK, 100, type);
    printf("send header OK\n");

    /*
    int n = send(newsockfd,msg, fsize, 0); 
    if (n < 0) error("ERROR writing to socket");
    */
    int n;
    if(fd >= 0) {
        while((n=read(fd, rcvBuf, BUFSIZ)) > 0){
            printf("read %d , sending\n", n);
            int res = write(newsockfd, rcvBuf, n);
            if(res <0) error("ERROR writing to socket");
        }
    }
    close(newsockfd);

}
void sendError(int newsockfd){
    char *msg = "<html><body><h1>400 Bad Request</h1></body></html>";
    sendResponseHeader(newsockfd, "400 Bad Request", strlen(msg), "text/html");
    writeToClient(newsockfd, msg);
}

void sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType){
    char resMsg[40];
    char conLen[40];
    char conType[50];
    sprintf(resMsg, "HTTP/1.1 %s\r\n",httpMsg);
    printf("sprintf resMsg : %s\n", resMsg);
    //sprintf(conLen, "Content-length: %ld\r\n", contentLen);
    sprintf(conType, "Content-Type: %s\r\n\r\n", contentType);
    printf("sprintf conType : %s\n", conType);
    writeToClient(newsockfd, resMsg);
    printf("send resMsg OK\n");
    //writeToClient(newsockfd, conLen);
    writeToClient(newsockfd, conType);
    printf("send conType OK\n");
}

int writeToClient(int newsockfd, char* msg){
    int n =  write(newsockfd, msg, strlen(msg));
    if (n < 0) error("ERROR writing to socket");
    return n;
}

