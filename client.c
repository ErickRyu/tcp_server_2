
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

void error(char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd; //Socket descriptor
	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server; //contains tons of information, including the server's IP address

	char buffer[BUFSIZ];

	char *requestMsg = "GET / HTTP1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.naver.com\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip,  deflate\r\nConnection: Keep-Alive)";
	portno = PORT_NO;
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //create a new socket
	if (sockfd < 0) 
		error("ERROR opening socket");
	printf("open socket\n");

	server = gethostbyname("www.naver.com"); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
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
		printf("%s\n",buffer);
		bzero(buffer,BUFSIZ);
	}
	if (n < 0) 
		error("ERROR reading from socket");

	close(sockfd); //close socket

	return 0;
}
