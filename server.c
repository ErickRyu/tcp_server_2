#include <stdio.h>
#include <sys/types.h>   
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t pthread;
void error(char *msg)
{
    perror(msg);
    exit(1);
}
void cleanExit(){exit(0);}


void *pthread_read_and_write(void *arg);
int writeToClient(int newsockfd, char* msg);
void sendError(int newsockfd);
void sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType);
void requestHandler(int newsockfd, char *reqMsg);
int main(int argc, char *argv[])
{
    //signal(SIGPIPE, SIG_IGN);
    sigignore(SIGPIPE);
    signal(SIGTERM, cleanExit);
    signal(SIGINT, cleanExit);
    int sockfd, newsockfd; 
    int portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; 
    serv_addr.sin_port = htons(portno); 

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    listen(sockfd,20); 

    clilen = sizeof(cli_addr);

    while(1){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");
        char reqMsg[500];
        ssize_t n;

        bzero(reqMsg,500);
        n = read(newsockfd,reqMsg,499); 
        if(n == 0) {close(newsockfd);continue;};
        if (n < 0) error("ERROR reading from socket");
        printf("========Request Message======\n%s\n",reqMsg);
        if(reqMsg[0] == 0){
            printf("req msg is null\n");
            close(newsockfd);
            continue;
        }
        requestHandler(newsockfd, reqMsg);
        bzero(reqMsg, 500);
        printf("=============================\n");



        /*
        pthread_create(&pthread, NULL, *pthread_read_and_write, (void *)(intptr_t)newsockfd);
        pthread_detach(pthread);
        */
    }
    close(sockfd);

    return 0; 
}

void *pthread_read_and_write(void *arg){
    int newsockfd = (int)arg;
    char reqMsg[500];
    ssize_t n;

    bzero(reqMsg,500);
    n = read(newsockfd,reqMsg,499); 
    if(n == 0) return NULL;
    if (n < 0) error("ERROR reading from socket");
        printf("========Request Message======\n%s\n",reqMsg);
        if(reqMsg[0] == 0){
            printf("req msg is null\n");
            close(newsockfd);
            return NULL;
        }
        requestHandler(newsockfd, reqMsg);
        bzero(reqMsg, 500);
        printf("=============================\n");

    return NULL;
}
void requestHandler(int newsockfd, char *reqMsg){


    printf("client socket : %d\n", newsockfd);

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

    if(extension == NULL || strcmp(extension, "html") == 0){
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
    FILE *fp = fopen(file, "rb");
    if(fp == NULL){
        sendError(newsockfd);
        return;
    }
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    char *msg = (char*)malloc(fsize);
    rewind(fp);
    fread(msg, fsize, 1, fp);
    fclose(fp);

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

    char *httpMsgOK = "200 OK";
    printf("sending header...\n");
    pthread_mutex_lock(&mutex);
    sendResponseHeader(newsockfd, httpMsgOK, fsize, type);
    printf("send header OK\n");

    pthread_mutex_unlock(&mutex);
    int n;
    bzero(rcvBuf, BUFSIZ + 1);
    /*
    if(fd >= 0) {
        while((n=read(fd, rcvBuf, BUFSIZ)) > 0){

            printf("sending rcvBuf : %d, remain : %ld\n", n, fsize-=n);
            ssize_t res = send(newsockfd, rcvBuf, n, 0);
            if(n == EOF | n == 0) { printf("finish file\n"); break; }
            if(res == 0) { printf("sending is 0\n");}
            if(res < 0) {
                char errMsg[100];
                sprintf(errMsg, "ERROR writing to socket __sock : %d __", newsockfd);
                error(errMsg);
            }
            bzero(rcvBuf, BUFSIZ + 1);
        }
    }
    */
    ssize_t res = write(newsockfd, msg, fsize);
    if(res == 0){
        printf("end file\n");
    }else if(res < 0){
        error("ERROR writing to socket");
    }
    close(newsockfd);
    printf("closed client socket\n");

}
void sendError(int newsockfd){
    char *msg = "<html><body><h1>400 Bad Request</h1></body></html>";
    sendResponseHeader(newsockfd, "400 Bad Request", strlen(msg), "text/html");
    writeToClient(newsockfd, msg);
    close(newsockfd);
    printf("closed client socket\n");
}

void sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType){
    char resMsg[40];
    char conLen[100];
    char conType[50];
    char responseHeader[200];
    sprintf(resMsg, "HTTP/1.1 %s\r\n",httpMsg);
    sprintf(conLen, "Content-length: %ld\r\n", contentLen);
    sprintf(conType, "Content-Type: %s\r\n\r\n", contentType);
    strcpy(responseHeader, resMsg);
    strcat(responseHeader, conLen);
    strcat(responseHeader, conType);

    printf("response message\n%s\n\n", responseHeader);
    /*
    printf("response message\n%s\n%s\n%s\n", resMsg, conLen, conType);
    writeToClient(newsockfd, resMsg);
    printf("send resMsg OK\n");
    writeToClient(newsockfd, conLen);
    writeToClient(newsockfd, conType);
    */
    writeToClient(newsockfd, responseHeader);
    printf("send conType OK\n");
}

int writeToClient(int newsockfd, char* msg){
    ssize_t n;
    printf("sending %ld length msg\n", strlen(msg));
    long toSend = strlen(msg);
    while(toSend > 0){
        n = write(newsockfd, msg, toSend);
        if(n == 0){
            printf("write is zero\n");
            return -1;
        }
        printf("write :  %ld\n", n);
        toSend -= n;
    }
    if (n < 0) error("ERROR writing to socket");
    return 0;
}

