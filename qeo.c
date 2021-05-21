//Author: Michael Paciullo

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define MAX 1000000
#define THREADS 10

struct sieve_args{
    int *primes;
    int primestart;
    int primeend;
};

void *sieve_subroutine(void *vargp){
    struct sieve_args *args = vargp;
    //printf("Prime chunk begin: %d\nPrime chunk bound: %d\n", args->primestart, args->primeend);
    for(int n = args->primestart; n < args->primeend; n++){
        if(args->primes[n] == 0){
            for(int j = n*n; j <= MAX; j += n){
                args->primes[j] = 1;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){
    //set up the clock
    struct timespec startm, endm;
    clock_gettime(CLOCK_MONOTONIC, &startm);

    //make an output file
    FILE* fptr;
    if((fptr = fopen("primes.txt", "w")) == NULL){ //try opening the output file
        printf("File cannot be created.\n");
        exit(1);
    }
    
    //make a list of primes using sieve of eratosthenes
    int primes[MAX] = {};
    for(int i = 2; i <= (int)sqrt(MAX); i++){
        if(primes[i] == 0){
            for(int j = i*i; j <= MAX; j += i){
                primes[j] = 1;
            }
        }
    }
    //output it to a text file
    for(int i = 2; i <= MAX; i++){
        if(primes[i] == 0){ //if the number's a prime
            char number[7];
            sprintf(number, "%d", i);
            fprintf(fptr, "%s\n", number);
        }
    }
    fclose(fptr);

    //check time elapsed
    clock_gettime(CLOCK_MONOTONIC, &endm);
    printf("Time for non-concurrent prime finder: %f\n", (endm.tv_sec-startm.tv_sec)+(endm.tv_nsec-startm.tv_nsec)/1.0e9);

    //CONCURRENT VERSION

    struct timespec startcm, endcm;
    clock_gettime(CLOCK_MONOTONIC, &startcm);

    //make a list of primes using a naive method
    int primesc[MAX] = {};
    pthread_t thread_ids[THREADS] = {};
    //create 10 processes
    for(int i = 0; i < 10; i++){
        //parallelized algorithm, checking the numbers in chunks of MAX/10
        //putting chunk boundaries in separate variables to maximize efficiency
        const int primechunkbegin = fmax(2, i*(int)(sqrt(MAX)/10));
        const int primechunkbound = (i+1)*(int)(sqrt(MAX)/10);
        struct sieve_args *args = malloc(sizeof(struct sieve_args));
        args->primes = primesc;
        args->primestart = primechunkbegin;
        args->primeend = primechunkbound;
        pthread_create(&thread_ids[i], NULL, sieve_subroutine, args);
    }
    //wait for all the processes to finish
    for(int i = 0; i < 10; i++){
        pthread_join(thread_ids[i], NULL);
    }

    for(int i = 0; i < 10; i++){
        pid_t child = fork();
        //printf("hello from process %d\n", child);
        if(child == 0){
            FILE* fptrc;
            char filename[14];
            sprintf(filename, "primesc%d.txt", i);
            if((fptrc = fopen(filename, "w")) == NULL){ //try opening the output file
                printf("File cannot be created.\n");
                exit(1);
            }

            const int chunkbegin = fmax(2, i*MAX/10);
            const int chunkbound = (i+1)*MAX/10;
            //printf("Chunk begin: %d\nChunk bound: %d\n", chunkbegin, chunkbound);
            for(int n = chunkbegin; n <= chunkbound; n++){
                if(primesc[n] == 0){ //if the number's a prime
                    char number[7];
                    sprintf(number, "%d", n);
                    fprintf(fptrc, "%s\n", number);
                }
            }
            fclose(fptrc);
            exit(0);
        }

    }
    int status;
    for(int i = 0; i < 10; i++){
        wait(&status);
    }
    //open the main file
    FILE* fptrmain;
    if((fptrmain = fopen("primesc.txt", "a")) == NULL){ //try opening the input file
        printf("File cannot be created.\n");
        exit(1);
    }
    
    for(int i = 0; i < 10; i++){//write each file to the main file one character at a time, for safety
        FILE* fptrc;
        char filename[14];
        sprintf(filename, "primesc%d.txt", i);
        if((fptrc = fopen(filename, "r")) == NULL){ //try opening the input file
            printf("File %s cannot be read.\n", filename);
            exit(1);
        }
        //write the file to the main file
        char c = fgetc(fptrc);
        while(c != EOF){
            fputc(c, fptrmain);
            c = fgetc(fptrc);
        }
        
    }
    //check time elapsed
    clock_gettime(CLOCK_MONOTONIC, &endcm);
    printf("Time for concurrent prime finder: %f\n", (endcm.tv_sec-startcm.tv_sec)+(endcm.tv_nsec-startcm.tv_nsec)/1.0e9);
}