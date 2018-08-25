#ifndef _libnetfiles_h
#define _libnetfiles_h

//global vars
int sockfd;
struct hostent *server;
struct sockaddr_in serverAddr;
char* hostname = NULL;
int portno = 9516;
int buffersize = 256;

//prototypes
int netserverinit(char*);
int netopen(const char*, int);
int getMsgSize(int);
int netclose(int);
ssize_t netwrite(int, void*, size_t);
ssize_t netread(int, void*, size_t);

#endif
