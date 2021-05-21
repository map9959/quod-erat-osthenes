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

#define MAX_PRIMES 2000000
#define MAX_THREADS 1000

struct sieve_args{
    int *primes;
    int primestart;
    int primeend;
    int primes_size;
};

void *sieve_subroutine(void *vargp){
    struct sieve_args *args = vargp;
    //printf("Prime chunk begin: %d\nPrime chunk bound: %d\n", args->primestart, args->primeend);
    for(int n = args->primestart; n < args->primeend; n++){
        if(args->primes[n] == 0){
            for(int j = n*n; j <= args->primes_size; j += n){
                args->primes[j] = 1;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("intended use: ./qeo [PRIMES] [THREADS]\n");
        return 1;
    }
    const int max_primes = atoi(argv[1]);
    if(max_primes < 2 || max_primes > MAX_PRIMES){
        printf("Primes must be a valid number >= 2 and <= %d\n", MAX_PRIMES);
        return 1;
    }
    const int threads = atoi(argv[2]);
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
    int primes[max_primes];
    memset(primes, 0, max_primes*sizeof(int));
    for(int i = 2; i <= (int)sqrt(max_primes)+1; i++){
        if(primes[i] == 0){
            for(int j = i*i; j <= max_primes; j += i){
                primes[j] = 1;
            }
        }
    }
    //output it to a text file
    for(int i = 2; i <= max_primes; i++){
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
    int primesc[max_primes];
    memset(primesc, 0, max_primes*sizeof(int));
    pthread_t thread_ids[threads];
    memset(thread_ids, 0, sizeof(pthread_t));
    //create 10 processes
    for(int i = 0; i < threads; i++){
        //parallelized algorithm, checking the numbers in chunks of MAX/THREADS
        //putting chunk boundaries in separate variables to maximize efficiency
        const int primechunkbegin = fmax(2, (int)(i*(sqrt(max_primes)/threads)));
        const int primechunkbound = (int)((i+1)*(sqrt(max_primes)/threads)+1);
        //printf("%d to %d\n", primechunkbegin, primechunkbound);
        struct sieve_args *args = malloc(sizeof(struct sieve_args));
        args->primes = primesc;
        args->primestart = primechunkbegin;
        args->primeend = primechunkbound;
        args->primes_size = max_primes;
        pthread_create(&thread_ids[i], NULL, sieve_subroutine, args);
    }
    //wait for all the processes to finish
    for(int i = 0; i < threads; i++){
        pthread_join(thread_ids[i], NULL);
    }

    //open the main file
    FILE* fptrmain;
    if((fptrmain = fopen("primesc.txt", "w")) == NULL){ //try opening the input file
        printf("File cannot be created.\n");
        exit(1);
    }

    for(int i = 2; i <= max_primes; i++){
        if(primesc[i] == 0){ //if the number's a prime
            char number[11];
            sprintf(number, "%d", i);
            fprintf(fptrmain, "%s\n", number);
        }
    }

    //check time elapsed
    clock_gettime(CLOCK_MONOTONIC, &endcm);
    printf("Time for concurrent prime finder: %f\n", (endcm.tv_sec-startcm.tv_sec)+(endcm.tv_nsec-startcm.tv_nsec)/1.0e9);
}