#ifndef _netfileserver_h
#define _netfileserver_h

//structs
typedef struct inputStruct{
	int fd;
	int nbytes;
	int buffersize;
	char* buffer;
}InStruct;

//global vars

int fileDescriptors[5000];
char* fileNames[5000];
int indexFileDes = 0;

//prototypes
int findFileByFD(int);
int findFileByName(char*);
int removeFD(int);
int getMsgSize(int);
void addToListFiles(int, char*);
void processConnection(void*);
void handleOpen(void*);
void handleClose(void*);
void handleWrite(void*);
void handleRead(void*);

#endif
