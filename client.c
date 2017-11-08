
/*
	 A simple client in the internet domain using TCP
Usage: ./client server_hostname server_port (./client 192.168.0.151 10000)
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT_NO 80

void client(char* reqMsg);
char* getContentType(char *filename);
int writeToClient(int newsockfd, char* msg);
void sendError(int newsockfd);
int sendResponseHeader(int newsockfd, char *httpMsg, long contentLen, char *contentType);
void requestHandler(int newsockfd, char *reqMsg);

void error(char *msg)
{
	perror(msg);
	exit(0);
}
int main(int argc, char *argv[])
{
    // Ignore SIGPIPE
    //sigignore(SIGPIPE);
    //signal(SIGTERM, cleanExit);
    //signal(SIGINT, cleanExit);

    int sockfd, newsockfd; 
    int portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 9999; 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; 
    serv_addr.sin_port = htons(portno); 

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    listen(sockfd,20); 

    clilen = sizeof(cli_addr);

    //while(1){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");
        char reqMsg[BUFSIZ];
        ssize_t n;

        bzero(reqMsg,500);
        n = read(newsockfd,reqMsg,BUFSIZ); 
        if (n < 0) error("ERROR reading from socket");
        //printf("========Request Message======\n%s\n",reqMsg);
				client(reqMsg);
				/*
        requestHandler(newsockfd, reqMsg);
        bzero(reqMsg, 500);
				*/
    //}
		close(newsockfd);
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

void client(char *reqMsg){
	int sockfd; //Socket descriptor
	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server; //contains tons of information, including the server's IP address

	char buffer[BUFSIZ];
	char *hostName = "www.egloos.com";
	char requestMsg[1024];
	//sprintf(requestMsg,  "GET /ww.egloos.com HTTP1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: %s\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip,  deflate\r\nConnection: Keep-Alive)", hostName);
	printf("%s\n",  reqMsg);
	portno = PORT_NO;
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //create a new socket
	if (sockfd < 0) 
		error("ERROR opening socket");
	printf("open socket\n");

	server = gethostbyname(hostName); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	printf("get host by name\n");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //initialize server's address
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);


	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //establish a connection to the server
		error("ERROR connecting");

	printf("connection OK\n");
	n = write(sockfd,requestMsg,strlen(requestMsg)); //write to the socket
	if (n < 0) 
		error("ERROR writing to socket");
	printf("write to socket OK\n");



	while((n = read(sockfd,buffer,BUFSIZ)) > 0) {//read from the socket
		printf("n is %d\n",  n);
		printf("%.*s\n",n,  buffer);
		bzero(buffer,BUFSIZ);
	}
	if (n < 0) 
		error("ERROR reading from socket");

	close(sockfd); //close socket

}
