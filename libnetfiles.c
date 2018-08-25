#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "libnetfiles.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>

//need this or else h_addr won't work
#define h_addr h_addr_list[0]

/*WHAT'S GOOD MY DUDE, NOTES FROM CLASS THAT YOU MISSED:

	1) possible buffer for netwrite: {32,w,-12,39,kl;dsfaljl,892nads;;U8ke}
		- buffer format: {numbytesToRead from buffer,functioncall,fileDescriptor,
			bytesToWrite to file,restOfMessage}
		- we will only use the first four commas as delimeters, every comma after that means 
			that the message sent contains a comma
		- use atoi on numbytes to read and bytesToWrite 
	
	2) netwrite(fd, msg, 28) take sizeof each param and add them together to know how many bytes to send to the server
	
	3) DO NOT USE:
		int permission = O_RDWR;
		write(socketFD, &permission, 4);
		
	4) open("./somenewfile", O_RDWR | O_CREAT, 00600)
	
	5) DO NOT USE STRLEN(BUFFER)
	
	6) strncmp(str1, str2, 7) compares amount of bytes, use this instead of strcmp()
	
	7) close connection everytime a net command is called
	
	PROGRAM INFO, HOW TO RUN:
	
	1) makefiles are already there so just call make, if you wanna see the line number of
		runtime error line numbers use make clang
		
	2) we need to first run the server on a different machine ex) utility.cs.rutgers.edu
		- to run just call make and then ./server
	
	3) for the client side its called test.c to test the library functions (client.c), I'll
		change the name later
		- just call make and run ./client
		
	4) when writing netfunctions you can have the buffer in any format you'd like as long as
		you decode it the same way in the handleFunctions in the server program
		
	5) when closing the server, just hit control+c to end connection
*/

ssize_t netread(int fd, void* readbuffer, size_t nbytes){
	char buffer[buffersize];
	readbuffer = (char*)readbuffer;
	int numbytes;
	errno = 0;
	
	//sets up client socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//checks if socket has been opened
	if(sockfd < 0){
		printf("ERROR OPENING SOCKET\n");
	}
	
	//checks if connection has been made
	if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		printf("ERROR CONNECTING\n");
		//also have to set errno to appropriate value
	}
	
	if(fd == 0)
		printf("ERROR, NO FILE DESCRIPTOR WAS GIVEN\n");
	
	//CODE TO SET UP BUFFER FORMAT
	//clear out buffer
	bzero(buffer, buffersize);
	char* buffptr = buffer;
	
	char sfd[10];
	sprintf(sfd, "%d", fd);
	int sizefd = strlen(sfd);
	
	char snbytes[10];
	sprintf(snbytes, "%zd", nbytes);
	int sizenbytes = strlen(snbytes);
	
	int totsize = sizefd + sizenbytes + sizeof('r') + 2;
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	
	memcpy(buffptr, stotsize, strlen(stotsize)); //add totsize to buffer
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, "r", 1); //add read flag to buffer
	buffptr+=1;
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sfd, strlen(sfd)); //add file descriptor to buffer
	buffptr+=strlen(sfd);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, snbytes, strlen(snbytes)); //add numbytes to buffer
	buffptr+=strlen(snbytes);
	*buffptr = '\0';	
	
	//write buffer to server
	numbytes = write(sockfd, buffer, sizeof(buffer)); //strlen(buffer)
	if(numbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
	
	//clear buffer, sets every value in buffer to zero
	bzero(buffer, buffersize);
	
	//read buffer from server
	//returns the first comma delimited value read from client
	int readsize = getMsgSize(sockfd);
	
	//reads the rest of the message sent from server minus the first comma delimited value
	numbytes = read(sockfd, buffer, readsize);
	
	if(numbytes < 0){
		printf("ERROR READING FROM SOCKET\n");
	}
	
	//print whatever was recieved from server
	//printf("Message recieved: %sEnd\n", buffer);
	
	//CODE TO DECODE THE BUFFER AND RETURN VAL TO USER HERE
	buffptr = buffer;
	int retval = atoi(buffptr); //get return value from buffer
	while(*buffptr != ',')
		buffptr += 1;
	buffptr+=1;
	int err = atoi(buffptr); //get errno from buffer
	
	while(*buffptr != ',')
		buffptr += 1;
	buffptr+=1;
	memcpy(readbuffer, buffptr, strlen(buffptr)); //get read lines from buffer
	
	if(err != 0)
		errno = err;
	close(sockfd);
	
	if(retval == -1 || retval == 0){
		errno = atoi(buffptr);
		return 0;
	}
	return retval;
}

ssize_t netwrite(int fd, void* writebuffer, size_t nbytes){
	char buffer[buffersize];
	writebuffer = (char*)writebuffer;
	int numbytes;
	errno = 0;
	
	//sets up client socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//checks if socket has been opened
	if(sockfd < 0){
		printf("ERROR OPENING SOCKET\n");
	}
	
	//checks if connection has been made
	if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		printf("ERROR CONNECTING\n");
		//also have to set errno to appropriate value
	}
	
	if(fd == 0)
		printf("ERROR, NO FILE DESCRIPTOR WAS GIVEN\n");
	
	//CODE TO SET UP BUFFER FORMAT
	//clear out buffer
	bzero(buffer, buffersize);
	char* buffptr = buffer;
	
	char sfd[10];
	sprintf(sfd, "%d", fd);
	int sizefd = strlen(sfd);
	
	int sizewb = strlen(writebuffer);
	
	char snbytes[10];
	sprintf(snbytes, "%zd", nbytes);
	int sizenbytes = strlen(snbytes);
	
	int totsize = sizefd + sizewb + sizenbytes + sizeof('w') + 3;
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, "w", 1);
	buffptr+=1;
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sfd, strlen(sfd));
	buffptr+=strlen(sfd);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, writebuffer, strlen(writebuffer));
	buffptr+=strlen(writebuffer);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, snbytes, strlen(snbytes));
	buffptr+=strlen(snbytes);
	*buffptr = '\0';
	//END CODE TO SET UP BUFFER FORMAT
	
	//write buffer to server
	numbytes = write(sockfd, buffer, sizeof(buffer)); //strlen(buffer)
	if(numbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
	
	//clear buffer, sets every value in buffer to zero
	bzero(buffer, buffersize);
	
	//read buffer from server
	//returns the first comma delimited value read from client
	int readsize = getMsgSize(sockfd);
	
	//reads the rest of the message sent from server minus the first comma delimited value
	numbytes = read(sockfd, buffer, readsize);
	
	if(numbytes < 0){
		printf("ERROR READING FROM SOCKET\n");
	}
	
	//print whatever was recieved from server
	//printf("Message recieved: %sEnd\n", buffer);
	
	//CODE TO DECODE THE BUFFER AND RETURN VAL TO USER HERE
	buffptr = buffer;
	int retval = atoi(buffptr);
	while(*buffptr != ',')
		buffptr += 1;
	buffptr+=1;
	int err = atoi(buffptr);
	if(err != 0)
		errno = err;
	
	close(sockfd);
	
	if(retval == -1 || retval == 0){
		errno = atoi(buffptr);
		return 0;
	}
	return retval;
}

int netclose(int fd){
	char buffer[buffersize];
	int numbytes;
	errno = 0;
	
	//sets up client socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//checks if socket has been opened
	if(sockfd < 0){
		printf("ERROR OPENING SOCKET\n");
	}
	
	//checks if connection has been made
	if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		printf("ERROR CONNECTING\n");
		//also have to set errno to appropriate value
	}
	
	if(fd == 0)
		printf("ERROR, NO FILE DESCRIPTOR WAS GIVEN\n");
	
	//CODE TO SET UP BUFFER FORMAT
	//clear out buffer
	bzero(buffer, buffersize);
	char* buffptr = buffer;
	char sfd[10];
	sprintf(sfd, "%d", fd);
	int totsize = strlen(sfd) + sizeof('c') + 1; //32,c,fd
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, "c", 1);
	buffptr+=1;
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sfd, strlen(sfd));
	buffptr+=strlen(sfd);
	*buffptr = '\0';
	//END CODE TO SET UP BUFFER FORMAT
	
	//write buffer to server
	numbytes = write(sockfd, buffer, sizeof(buffer)); //strlen(buffer)
	if(numbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
	
	//clear buffer, sets every value in buffer to zero
	bzero(buffer, buffersize);
	
	//read buffer from server
	//returns the first comma delimited value read from client
	int readsize = getMsgSize(sockfd);
	
	//reads the rest of the message sent from server minus the first comma delimited value
	numbytes = read(sockfd, buffer, readsize);
	
	if(numbytes < 0){
		printf("ERROR READING FROM SOCKET\n");
	}
	
	//print whatever was recieved from server
	//printf("Message recieved: %sEnd\n", buffer);
	
	//CODE TO DECODE THE BUFFER AND RETURN VAL TO USER HERE
	buffptr = buffer;
	int retval = atoi(buffptr);
	while(*buffptr != ',')
		buffptr += 1;
	buffptr+=1;
	int err = atoi(buffptr);
	if(err != 0)
		errno = err;
	
	close(sockfd);
	
	if(retval == -1){
		errno = atoi(buffptr);
		return -1;
	}
	return 0;
}

int netopen(const char* path, int flags){

	//initialization
	char buffer[buffersize];
	int numbytes;
	errno = 0;
	
	//sets up client socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//checks if socket has been opened
	if(sockfd < 0){
		printf("ERROR OPENING SOCKET\n");
	}
	
	//checks if connection has been made
	if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		printf("ERROR CONNECTING\n");
		//also have to set errno to appropriate value
	}
	
	//checks input flags and sets mode to appropriate char
	char *flag = NULL;
	switch(flags){
		case O_RDONLY: 	flag = "r"; break;
		case O_WRONLY: 	flag = "w"; break;
		case O_RDWR:	flag = "b"; break;
		default:	printf("FLAG GIVEN IS INVALID");
				return -1;
	}
	
	//CODE TO SET UP BUFFER FORMAT FOR OPEN HERE
	//clear buffer
	bzero(buffer, buffersize);
	char* buffptr = buffer;
	int pathsize = strlen(path); //sizeof(path)-1 maybe?
	int flagsize = strlen(flag);
	int totsize = pathsize + flagsize + sizeof('o') + 2; //change back to 3?
	
	//sprintf() is a function to convert an int to an string
	//do this to send an int value to the server or client
	//then we can use atoi() to convert back to int
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, "o", 1);
	buffptr+=1;
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, flag, 1);
	buffptr+=1;
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, path, pathsize);
	buffptr+=(pathsize);//+1
	*buffptr = '\0';
	//END OF CODE TO SET UP BUFFER FORMAT
	
	//write buffer to server
	numbytes = write(sockfd, buffer, sizeof(buffer)); //strlen(buffer)
	if(numbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
	
	//clear buffer, sets every value in buffer to zero
	bzero(buffer, buffersize);
	
	//returns the first comma delimited value read from client
	int readsize = getMsgSize(sockfd);
	
	//reads the rest of the message sent from server minus the first comma delimited value
	numbytes = read(sockfd, buffer, readsize);
	
	if(numbytes < 0){
		printf("ERROR READING FROM SOCKET\n");
	}
	
	//print whatever was recieved from server
	//printf("Message recieved: %sEnd\n", buffer);
	
	//CODE TO DECODE THE BUFFER AND RETURN VAL TO USER HERE
	buffptr = buffer;
	int fd = atoi(buffptr);
	while(*buffptr != ',')
		buffptr += 1;
	buffptr+=1;
	if(fd == -1)
		errno = atoi(buffptr);
	close(sockfd);
	return fd;
}

//get size of message and chops off first comma delimited value from buffer
int getMsgSize(int sockfd){
	char charbuffsize[10];
	char* tempstr = charbuffsize;
	char c;
	read(sockfd, &c, 1);
	int i = 0;
	while(c != ','){
		memcpy(tempstr, &c, 1);
		i++;
		tempstr+=1;
		read(sockfd, &c, 1);
	}
	*tempstr = '\0';
	int msgsize = atoi(charbuffsize);
	return msgsize;
}

//set up connection between client and server, client must first
//call this before any net functions
int netserverinit(char* host){

	//sets global var hostname and sets server for example: server = "cd.cs.rutgers.edu"
	hostname = host;
	server = gethostbyname(hostname);
	
	if(server == NULL){
		printf("ERROR, SERVER NAME INVALID");
		return -1;
		//HAVE TO SET ERRNO I THINK
	}
	
	//zero out serverAddr
	bzero((char*) &serverAddr, sizeof(serverAddr));
	
	//setup serverAddr struct
	serverAddr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portno);
	
	return 0;	
	//also need to set h_errnor accordingly
}
