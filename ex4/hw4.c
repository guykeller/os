#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define BUFFER_SIZE 1048576
char outputBuffer[BUFFER_SIZE] = "";
int curNumOfThreads = 0;
int visitCounter = 0;
int curNumOfFilledBytes = 0;
int numOfBytesInOutput = 0; //remember to increase in thread_func
pthread_mutex_t bufferLock;

void* thread_func(void* thread_param) {
	char* inputFilePath = (char*) thread_param;
    int inputFileFD = 0;
    int bytesRead = 0;
    int rc = 0;
    char outputBuffer[BUFFER_SIZE] = "";
    
    //opening input file
    inputFileFD = open(inputFilePath, O_RDONLY);
    if (inputFileFD<0) {
        printf ("ERROR opening output file");
        exit(-1);
    }


    while ((bytesRead = read(inputFileFD,outputBuffer,BUFFER_SIZE)) >= 0) {
        //lock the buffer lock
        rc = pthread_mutex_lock(&bufferLock);

        //exit if lock failed
        if (rc) {
            printf ("ERROR locking the lock");
            exit(-1);
        }

        //check if i will be finished after this iteration
        if (bytesRead < BUFFER_SIZE) {
            curNumOfThreads--;
        }

    }

    if (bytesRead<0) {
        printf ("ERROR reading from input file");
        exit(-1);
    }

}

int main(int argc, char *argv[]) {
	//initializing vars
    pthread_t thread_id;
	int i=0, rc = 0;
    char* outputPath = argv[1];
    char* curInputFilePath;
    int numOfInputFiles = argc - 2;
    int curInputFileIndex = 0, outputFileFD = 0;
    //step 1 - print welcome message
    printf("Hello, creating %s from %d input files", outputPath, numOfInputFiles);

    //step 2 - initializing vars
    curNumOfThreads = numOfInputFiles;
    pthread_t* threadArray =(pthread_t*) malloc(sizeof(pthread_t) * numOfInputFiles);
    if (threadArray == NULL) {
        printf ("ERROR creating thread array file");
        exit(-1);
    }

    //step 3 - Open the output file for writing with truncation (use O_TRUNC)
    outputFileFD = open(outputPath, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (outputFileFD<0) {
        printf ("ERROR opening output file");
        exit(-1);
    }

    //step 4 - creating threads
    for (i=0; i<numOfInputFiles; i++) {
        curInputFileIndex = i+2;
        curInputFilePath = argv[curInputFileIndex];
        rc = pthread_create(&threadArray[i], NULL, thread_func, (void *)curInputFilePath); 
	    if (rc){
		    printf("ERROR in pthread_create(): %s\n", strerror(rc));
		    exit(-1);
	    }
    }
    
    //step 5 - Wait for all reader threads to finish
    for (i=0; i<numOfInputFiles; i++) {
        curInputFileIndex = i+2;
        curInputFilePath = argv[curInputFileIndex];
        rc = pthread_join(threadArray[i], &status); 
	    if (rc){
		    printf("ERROR in pthread_join(): %s\n", strerror(rc));
		    exit(-1);
	    }
    }


    //step 6 - Close output file
    rc = close(outputFileFD);
    if (rc) {
        printf ("ERROR closing output file");
        exit(-1);
    }

    //add free if program is finished normally
    free(threadArray);

    //step 7 - print message
    printf("Created <output> with size <size> bytes",outputPath, numOfBytesInOutput);

    //step 8 - exit
    exit(0);
	
}