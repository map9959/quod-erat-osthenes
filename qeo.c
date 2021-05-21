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
#define MAX_THREADS 1000

struct sieve_args{
    int *primes;
    int primestart;
    int primeend;
    int chunk;
    int threads;
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

void *parallel_print(void *vargp){
    struct sieve_args *args = vargp;
    FILE* fptrc;
    char filename[14];
    sprintf(filename, "primesc%d.txt", args->chunk);
    if((fptrc = fopen(filename, "w")) == NULL){ //try opening the output file
        printf("File cannot be created.\n");
        exit(1);
    }
    const int chunkstart = fmax(2, args->chunk*MAX/args->threads);
    const int chunkend = (args->chunk+1)*MAX/args->threads;
    //printf("Chunk begin: %d\nChunk bound: %d\n", chunkbegin, chunkbound);
    for(int n = chunkstart; n <= chunkend; n++){
        if(args->primes[n] == 0){ //if the number's a prime
            char number[11];
            sprintf(number, "%d", n);
            fprintf(fptrc, "%s\n", number);
        }
    }
    fclose(fptrc);
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("intended use: ./qeo [THREADS]\n");
        return 1;
    }
    const int threads = atoi(argv[1]);
    if(threads < 2 || threads > MAX_THREADS){
        printf("Threads must be a valid number >= 2 and <= %d\n", MAX_THREADS);
        return 1;
    }

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
            char number[11];
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
    pthread_t thread_ids[threads];
    memset(thread_ids, 0, sizeof(pthread_t));
    //create 10 processes
    for(int i = 0; i < threads; i++){
        //parallelized algorithm, checking the numbers in chunks of MAX/THREADS
        //putting chunk boundaries in separate variables to maximize efficiency
        const int primechunkbegin = fmax(2, i*(int)(sqrt(MAX)/threads));
        const int primechunkbound = (i+1)*(int)(sqrt(MAX)/threads);
        
        struct sieve_args *args = malloc(sizeof(struct sieve_args));
        args->primes = primesc;
        args->primestart = primechunkbegin;
        args->primeend = primechunkbound;
        args->chunk = i;
        pthread_create(&thread_ids[i], NULL, sieve_subroutine, args);
    }
    //wait for all the processes to finish
    for(int i = 0; i < threads; i++){
        pthread_join(thread_ids[i], NULL);
    }

    for(int i = 0; i < threads; i++){
        struct sieve_args *args = malloc(sizeof(struct sieve_args));
        args->primes = primesc;
        args->chunk = i;
        args->threads = threads;
        pthread_create(&thread_ids[i], NULL, parallel_print, args);
    }
    //wait for all the processes to finish
    for(int i = 0; i < threads; i++){
        pthread_join(thread_ids[i], NULL);
    }

    //open the main file
    FILE* fptrmain;
    if((fptrmain = fopen("primesc.txt", "a")) == NULL){ //try opening the input file
        printf("File cannot be created.\n");
        exit(1);
    }
    
    for(int i = 0; i < threads; i++){//write each file to the main file one character at a time, for safety
        FILE* fptrc;
        char filename[16];
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
        fclose(fptrc);
        if(remove(filename) != 0){
            perror("File could not be deleted");
        }
    }
    //check time elapsed
    clock_gettime(CLOCK_MONOTONIC, &endcm);
    printf("Time for concurrent prime finder: %f\n", (endcm.tv_sec-startcm.tv_sec)+(endcm.tv_nsec-startcm.tv_nsec)/1.0e9);
}