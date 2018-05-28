#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 1048576
char outputBuffer[BUFFER_SIZE] = "";
int curNumOfThreads = 0;
int visitCounter = 0;
int curNumOfFilledBytes = 0;
int totalNumOfBytesInOutput = 0; //remember to increase in thread_func
int outputFileFD = 0;
int globalIter = 1;
pthread_mutex_t bufferLock;
pthread_cond_t  all_threads_finished_iteration;

void* thread_func(void* thread_param) {
	char* inputFilePath = (char*) thread_param;
    int inputFileFD = 0;
    int bytesRead = 0;
    int rc = 0, i=0;
    int localIter = 1;
    char tempBuffer[BUFFER_SIZE] = "";
    bool isFinished = false;
    //opening input file
    inputFileFD = open(inputFilePath, O_RDONLY);
    if (inputFileFD<0) {
        printf ("ERROR opening output file\n");
        exit(-1);
    }


    while ((bytesRead = read(inputFileFD,tempBuffer,BUFFER_SIZE)) >= 0) {
        //1 - lock the buffer lock
        while (localIter > globalIter) {
            rc = pthread_mutex_lock(&bufferLock);
        }

        //exit if lock failed
        if (rc) {
            printf ("ERROR locking the lock\n");
            exit(-1);
        }

        //2 - check if i will be finished after this iteration
        if (bytesRead < BUFFER_SIZE) {
            curNumOfThreads--;
            isFinished = true;
        }

        //3 - xor the buffer
        for (i=0; i<bytesRead; i++) {
            outputBuffer[i] = (outputBuffer[i] ^ tempBuffer[i]);
        }
            

        //4 - update the part updated in buffer if necessary
        if (curNumOfFilledBytes < bytesRead)
            curNumOfFilledBytes = bytesRead;
        if (!isFinished)
            visitCounter++;
        if (curNumOfThreads == visitCounter)  {
            rc = write(outputFileFD,outputBuffer, curNumOfFilledBytes);
            if (rc !=  curNumOfFilledBytes) {
                printf ("ERROR reading from thread file\n");
                exit(-1);
            }
            for (i=0; i<BUFFER_SIZE; i++) {
                tempBuffer[i] = (char) 0;
            }
            totalNumOfBytesInOutput += curNumOfFilledBytes;
            visitCounter = 0;
            curNumOfFilledBytes = 0;
            globalIter++;
            localIter++;
            rc = pthread_cond_broadcast(&all_threads_finished_iteration);
            if (rc) {
                printf ("ERROR broadcasting signal\n");
                exit(-1);
            }
            rc = pthread_mutex_unlock(&bufferLock);
            if (rc) {
                printf ("ERROR unlocking lock\n");
                exit(-1);
            }
        }
        else {
            localIter++;
            while (localIter > globalIter) {
                rc = pthread_cond_wait(&all_threads_finished_iteration,&bufferLock);
            }
            if (rc) {
                printf ("ERROR waiting for thread signal\n");
                exit(-1);
            }
            rc = pthread_mutex_unlock(&bufferLock);
            if (rc) {
                printf ("ERROR unlocking the lock\n");
                exit(-1);
            }
        }
    }

    if (bytesRead<0) {
        printf ("ERROR reading from input file\n");
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
    int curInputFileIndex = 0;
    //step 1 - print welcome message
    printf("Hello, creating %s from %d input files\n", outputPath, numOfInputFiles);

    //step 2 - initializing vars
    curNumOfThreads = numOfInputFiles;
    pthread_t* threadArray =(pthread_t*) malloc(sizeof(pthread_t) * numOfInputFiles);
    if (threadArray == NULL) {
        printf ("ERROR creating thread array file\n");
        exit(-1);
    }
    rc = pthread_cond_init (&all_threads_finished_iteration, NULL);

    //step 3 - Open the output file for writing with truncation (use O_TRUNC)
    outputFileFD = open(outputPath, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (outputFileFD<0) {
        printf ("ERROR opening output file\n");
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
        rc = pthread_join(threadArray[i],NULL); 
	    if (rc){
		    printf("ERROR in pthread_join(): %s\n", strerror(rc));
		    exit(-1);
	    }
    }


    //step 6 - Close output file
    rc = close(outputFileFD);
    if (rc) {
        printf ("ERROR closing output file\n");
        exit(-1);
    }

    //add free if program is finished normally
    free(threadArray);
    pthread_mutex_destroy(&bufferLock);
    pthread_cond_destroy(&all_threads_finished_iteration);

    //step 7 - print message
    printf("Created %s with size %d bytes\n",outputPath, totalNumOfBytesInOutput);

    //step 8 - exit
    exit(0);
	
}