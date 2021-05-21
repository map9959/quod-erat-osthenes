# Quod Erat Osthenes
Concurrent prime finder using POSIX threads made for fun. Not necessarily optimal. Only works on Linux due to the clock functions used.

## Build instructions
Type `make` in the cloned folder. You're welcome!

## Usage instructions
`./qeo [primes] [threads]`

primes is an integer >= 2 and <= `MAX_PRIMES`, set to 1000000 by default. Change at your own risk.

threads is an integer >= 2 and <= `MAX_THREADS`, set to 1000 by default