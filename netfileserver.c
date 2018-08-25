#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "netfileserver.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <pthread.h>

extern int errno;
pthread_mutex_t lock;


int main(int argc, char* argv[]){

	//initialization
	pthread_mutex_init(&lock, NULL);
	int servsockfd, newsockfd, portno, buffersize;
	portno = 9516;
	unsigned int clilen;
	buffersize = 256;
	char buffer[buffersize];
	struct sockaddr_in serverAddr, cli_addr;
	int nbytes;
	nbytes = 0;	
	
	//sets up server socket
	servsockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//check if socket has been opened
	if(servsockfd < 0){
		printf("ERROR OPENING SOCKET\n");
	}
	
	//zero out addr struct
	bzero((char*) &serverAddr, sizeof(serverAddr));
	
	//setup addr struct
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portno);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	
	//binds socket to port
	if(bind(servsockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0){
		printf("ERROR ON BINDING\n");
	}
	
	//listens for connection from client
	listen(servsockfd, 5);
	clilen = sizeof(cli_addr);
	
	while((newsockfd = accept(servsockfd, (struct sockaddr *) &cli_addr, &clilen)) > 0){
		
		//set up struct to pass into function
		InStruct inputStruct;
		inputStruct.fd = newsockfd;
		inputStruct.nbytes = nbytes;
		inputStruct.buffersize = buffersize;
		inputStruct.buffer = buffer;
		
		//creates thread and runs function to process what the client wants
		pthread_t threadid;
		pthread_create(&threadid, NULL, (void*)processConnection, &inputStruct);
		pthread_join(threadid, NULL);
	}
	pthread_mutex_destroy(&lock);
	close(newsockfd);
	return 0;
}

//function to be passed in when creating thread to handle what the client wants
void processConnection(void* inputStruct){
	
	//pulls from the inputStruct all the necessary info
	InStruct *inStruct = (InStruct*)inputStruct;
	int newsockfd = inStruct->fd;
	int nbytes = inStruct->nbytes;
	int buffersize = inStruct->buffersize;
	char* buffer = inStruct->buffer;
	
	//checks if socket has been accepted
	if(newsockfd < 0){
		printf("ERROR ON ACCEPT\n");
	}
	
	//zero out buffer
	bzero(buffer, buffersize);
	
	int remainingMsgSize = getMsgSize(newsockfd);
	
	//read from client
	nbytes = read(newsockfd, buffer, remainingMsgSize); //-1 UNCOMMENT THIS BACK
	
	if(nbytes < 0){
		printf("ERROR READING FROM SOCKET\n");
	}
	
	//printf("Message recieved: %sEnd\n", buffer); //CHANGE BACK TO BUFFER
	//CODE TO DECODE THE BUFFER HERE
	char* netcommand = buffer;
	
	//ADD CODE TO CALL OPEN, READ, WRITE, AND CLOSE HERE
	switch(*netcommand){
		case 'o': handleOpen(inputStruct); break;
		case 'r': handleRead(inputStruct); break;
		case 'w': handleWrite(inputStruct); break;
		case 'c': handleClose(inputStruct); break;
		default: printf("ERROR, INVALID FORMAT OF BUFFER");
	}
}

void handleRead(void* inputStruct){

	//INITIALIZE FIELDS WITH STRUCT VALUES
	InStruct *inStruct = (InStruct*)inputStruct;
	int newsockfd = inStruct->fd;
	int nbytes = inStruct->nbytes;
	int buffersize = inStruct->buffersize;
	char* buffer = inStruct->buffer;
	
	errno = 0;
	
	char* buffptr = buffer+2;
	int fd = findFileByFD(atoi(buffptr)); //take file descriptor from buffer
	int retval = 0;
	while(*buffptr != ',') //iterate until next comma
		buffptr+=1;
	buffptr+=1;
	int numbytes = atoi(buffptr); //take numbytes to read from buffer
	
	char readbuffer[numbytes+1];
	
	if(fd != 0 && fd!= -1){
		pthread_mutex_lock(&lock);
		retval = read(fd*(-1), &readbuffer, numbytes);
		pthread_mutex_unlock(&lock);
		readbuffer[numbytes] = '\0';
	}
	else{
		printf("ERROR, COULD NOT READ FILE\n");
	}
	if(retval == 0 || retval == -1)
		printf("ERROR, COULD NOT READ FILE\n");

	//CODE TO CONVERT ALL INT VALUES TO STRINGS
	char sretval[10];
	sprintf(sretval, "%d", retval);
	
	char serrno[10];
	sprintf(serrno, "%d", errno);
	
	int totsize = strlen(sretval) + strlen(serrno) + retval + 2;
	
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	//END CONVERT STRINGS CODE
	
	bzero(buffer, buffersize);
	buffptr = buffer;
	
	//CODE FOR BUFFER FORMAT TO SEND BACK TO CLIENT
	memcpy(buffptr, stotsize, strlen(stotsize)); //put total size in buffer
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sretval, strlen(sretval)); //put return value in buffer
	buffptr+=strlen(sretval);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, serrno, strlen(serrno)); //put errno in buffer
	buffptr+=strlen(serrno);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, readbuffer, retval); //put read lines in buffer
	buffptr+=strlen(readbuffer);
	*buffptr = '\0'; //null terminate the whole buffer	
	//END BUFFER FORMAT CODE
	
	//write back to client
	nbytes = write(newsockfd, buffer, strlen(buffer));//dont use strlen
	//checks if bytes were actually written to socket
	if(nbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
	
}

void handleWrite(void* inputStruct){
	InStruct *inStruct = (InStruct*)inputStruct;
	int newsockfd = inStruct->fd;
	int nbytes = inStruct->nbytes;
	int buffersize = inStruct->buffersize;
	char* buffer = inStruct->buffer;
	
	errno = 0;
	
	char* buffptr = buffer+2;
	int fd = findFileByFD(atoi(buffptr));
	int retval = 0;
	
	while(*buffptr != ',')
		buffptr+=1;
	buffptr+=1;
	
	char* writebuffer = buffptr;
	
	while(*buffptr != ',')
		buffptr+=1;
	buffptr+=1;
	
	int numbytes = atoi(buffptr);
	
	if(fd != 0 && fd!= -1){
		pthread_mutex_lock(&lock);
		retval = write(fd*(-1), writebuffer, numbytes);
		pthread_mutex_unlock(&lock);
	}
	else{
		printf("ERROR, COULD NOT WRITE TO FILE\n");
	}
	if(retval == 0 || retval == -1)
		printf("ERROR, COULD NOT WRITE TO FILE\n");
		
	char sretval[10];
	sprintf(sretval, "%d", retval);
	
	char serrno[10];
	sprintf(serrno, "%d", errno);
	
	int totsize = strlen(sretval) + strlen(serrno) + 1;
	
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	
	//set up buffer to send back to client
	bzero(buffer, buffersize);
	buffptr = buffer;
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sretval, strlen(sretval));
	buffptr+=strlen(sretval);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, serrno, strlen(serrno));
	buffptr+=strlen(serrno);
	*buffptr = '\0';
	
	//write back to client
	nbytes = write(newsockfd, buffer, sizeof(buffer));//dont use strlen
	
	//checks if bytes were actually written to socket
	if(nbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
}

void handleClose(void* inputStruct){
	
	//pulls from the inputStruct all the necessary info
	InStruct *inStruct = (InStruct*)inputStruct;
	int newsockfd = inStruct->fd;
	int nbytes = inStruct->nbytes;
	int buffersize = inStruct->buffersize;
	char* buffer = inStruct->buffer;
	
	errno = 0;
	
	char* buffptr = buffer+2;
	int fd = findFileByFD(atoi(buffptr));
	int retval = 0;
	
	//close the file
	if(fd != 0 && fd!= -1){
		retval = close(fd*(-1)); //close actual file, multiply file descriptor by -1 first
		removeFD(fd);		//remove file descriptor from list of fds
	}
	else{
		
		printf("ERROR, CANNOT CLOSE FILE\n");
	}
	
	if(retval == -1){
		printf("ERROR, CANNOT CLOSE FILE\n");
	}
	
	
	char sretval[10];
	sprintf(sretval, "%d", retval);
	
	char serrno[10];
	sprintf(serrno, "%d", errno);
	
	int totsize = strlen(sretval) + strlen(serrno) + 1;
	
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	
	//set up buffer to send back to client
	bzero(buffer, buffersize);
	buffptr = buffer;
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, sretval, strlen(sretval));
	buffptr+=strlen(sretval);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, serrno, strlen(serrno));
	buffptr+=strlen(serrno);
	*buffptr = '\0';
	
	//write back to client
	nbytes = write(newsockfd, buffer, sizeof(buffer));//dont use strlen
	
	//checks if bytes were actually written to socket
	if(nbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
}

//handles the event of the client calling open
void handleOpen(void* inputStruct){

	//pulls from the inputStruct all the necessary info
	InStruct *inStruct = (InStruct*)inputStruct;
	int newsockfd = inStruct->fd;
	int nbytes = inStruct->nbytes;
	int buffersize = inStruct->buffersize;
	char* buffer = inStruct->buffer;
	
	errno = 0;

	//decode buffer to retrieve values to call open function
	char* buffptr = buffer+2;
	char* cflag = buffptr;
	int flag;
	
	switch(*cflag){
		case 'r': flag = O_RDONLY; break;
		case 'w': flag = O_WRONLY; break;
		case 'b': flag = O_RDWR; break;
		default: printf("OPEN FLAG NOT SET");
	}
	
	buffptr+=2;
	char* path = buffptr;
	
	//call open with decoded values
	int fd = open(path, flag);
	
	if(fd == -1){
		printf("ERROR, COULD NOT OPEN FILE\n");
	}
	else{
		addToListFiles(fd*(-1), path); //add file descriptor to list of fileNames and list of fileDescriptors
	}
	
	//set up buffer to send back to client
	bzero(buffer, buffersize);
	buffptr = buffer;
	
	char snum[10];
	if(fd == -1)
		sprintf(snum, "%d", fd);
	else
		sprintf(snum, "%d", fd*(-1));
	
	char serrno[10];
	sprintf(serrno, "%d", errno);
	
	int fdsize = strlen(snum);
	int errnosize = strlen(serrno);
	int totsize = fdsize + errnosize + 1;
	
	char stotsize[10];
	sprintf(stotsize, "%d", totsize);
	
	memcpy(buffptr, stotsize, strlen(stotsize));
	buffptr+=strlen(stotsize);
	*buffptr = ',';
	buffptr+=1;
	memcpy(buffptr, snum, strlen(snum));
	buffptr += strlen(snum);
	
	*buffptr = ',';
	buffptr += 1;
	memcpy(buffptr, serrno, strlen(serrno));
	buffptr += strlen(serrno);
	*buffptr = '\0'; //set last spot to nullbyte just in case
	
	errno = 0;
	
	//write back to client
	nbytes = write(newsockfd, buffer, sizeof(buffer));//dont use strlen
	
	//checks if bytes were actually written to socket
	if(nbytes < 0){
		printf("ERROR WRITING TO SOCKET\n");
	}
}

//search for previously opened file by file name
//returns fd on success, -1 on fail
int findFileByName(char* fileName){
	for(int i = 0; i<indexFileDes; i++){
		if(strcmp(fileName, fileNames[i]) == 0)
			return fileDescriptors[i];
	}
	return -1;
}

//search for previously opened file by file descriptor
//returns fd on success, -1 on fail
int findFileByFD(int fd){
	for(int i = 0; i<indexFileDes; i++){
		if(fd == fileDescriptors[i])
			return fileDescriptors[i];
	}
	return -1;
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

//removes file descriptor from list when close is called
//returns 0 on success, -1 on fail
int removeFD(int fd){
	for(int i=0; i<indexFileDes; i++){
		//if file is found, remove it from list and put the last one in it's place
		if(fd == fileDescriptors[i]){
			if(i==indexFileDes-1){
				fileDescriptors[i] = 0;
				fileNames[i] = 0;
				indexFileDes--;
				return 0;
			}
			else{
				fileDescriptors[i] = fileDescriptors[indexFileDes-1];
				fileNames[i] = fileNames[indexFileDes-1];
				indexFileDes--;
				fileDescriptors[indexFileDes] = 0;
				fileNames[indexFileDes] = 0;
				return 0;
			}
		}
	}
	return -1;
}

//adds file desc and path to global lists so we can recall them later, 
//all fd's are stored as negative so multiply by -1 when using them
void addToListFiles(int fd, char* path){
	fileDescriptors[indexFileDes] = fd;
	fileNames[indexFileDes] = path;
	indexFileDes++;
}


