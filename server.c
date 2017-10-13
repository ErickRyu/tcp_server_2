#include <stdio.h>
#include <sys/types.h>   
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}
void cleanExit(){exit(0);}


char* getContentType(char *filename);
int writeToClient(int newsockfd, char* msg);
void sendError(int newsockfd);
int sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType);
void requestHandler(int newsockfd, char *reqMsg);
int main(int argc, char *argv[])
{
    // Ignore SIGPIPE
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
    }
    close(sockfd);

    return 0; 
}
char *getContentType(char *fileName){
    char tmpFileName[100];
    strcpy(tmpFileName, fileName);
    strtok(tmpFileName, ".");
    char *extension = strtok(NULL, ".");
    if(extension == NULL || strcmp(extension, "html") == 0){
        return "text/html";
    }else if(strcmp(extension, "jpeg") == 0){
        return "image/jpeg";
    }else if(strcmp(extension, "gif") == 0){
        return "image/gif";
    }else if(strcmp(extension, "mp3") == 0){
        return "audio/mpeg";
    }else if(strcmp(extension, "pdf") == 0){
        return "application/pdf";
    }
    return "text/plain";
}

void requestHandler(int newsockfd, char *reqMsg){

    // Tokenizing http method and file name from request message
    char file[100];
    char *method = strtok(reqMsg, " /");
    strcpy(file, strtok(NULL, " /"));

    // Return if http method is not GET
    if(strcmp(method, "GET") != 0 && strcmp(method, "get") != 0){
        sendError(newsockfd);
        return;
    }

    /* Use index.html as default file. But I commented this lines because I don't know what is your default index file.
    if(strcmp(file, "HTTP") == 0 || strcmp(file, "http") == 0){
        strcpy(file , "index.html");
    }
    */

    
    // Get content-type by compare string of file's extention
    char *type = getContentType(file);
    printf("type : %s\n", type);

    // Calculate file size and Open file. When file is not exist, return 400 Bad Request
    long fsize;
    FILE *fp = fopen(file, "rb");
    if(fp == NULL){
        sendError(newsockfd);
        return;
    }
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    
    // Read file to msg
    char *msg = (char*)malloc(fsize);
    rewind(fp);
    fread(msg, fsize, 1, fp);
    fclose(fp);

    // Send response message. If return is less than 0, client socket connection is closed. because write functino returned 0
    char *httpMsgOK = "200 OK";
    if( sendResponseHeader(newsockfd, httpMsgOK, fsize, type) < 0){
        printf("Client socket connection closed\n");
        close(newsockfd);
    }

    // Write file to client
    ssize_t res = write(newsockfd, msg, fsize);
    if(res == 0){
        printf("end file\n");
    }else if(res < 0){
        error("ERROR writing to socket");
    }
    close(newsockfd);
}
void sendError(int newsockfd){
    char *msg = "<html><body><h1>400 Bad Request</h1></body></html>";
    sendResponseHeader(newsockfd, "400 Bad Request", strlen(msg), "text/html");
    writeToClient(newsockfd, msg);
    close(newsockfd);
    printf("closed client socket\n");
}

int sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType){
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
    return writeToClient(newsockfd, responseHeader);
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

