# Assignment 2 - Joshua Ganschow

## Build and run instructions

### R1
To build, run, and observe the output of the static master-worker implementation (filename `r1.cpp`), follow the instructions below.

1. run `mpic++ -std=c++17 r1.cpp -o r1`
2. run `mpirun -np 4 ./r1 200000 50000` to count the primes between 2 and 200000 using one master, three workers, and work segments containing 50000 integers
3. Observe the output to see the segments assigned to each worker, the number of primes found in each segment, the total gathered by the master, and whether the total matches the sequential result.

The master divides the range into segments before computation begins and assigns those segments to the workers in a round-robin order. Each worker processes all of its assigned segments and sends its partial count back to the master.

### R2
To build, run, and observe the output of the dynamic master-worker implementation (filename `r2.cpp`), follow the instructions below.

1. run `mpic++ r2.cpp -o r2`
2. run `mpirun -np 4 ./r2 200000 50000` to count the primes between 2 and 200000 using one master, three workers, and work segments containing 50000 integers
3. Observe the output to see the number of primes found by each worker and whether the total gathered by the master matches the sequential result.

Workers request one segment at a time from the master. After processing a segment, a worker returns its result and requests more work. When no work remains, the master sends a stop message to each worker when they request new work.

### Running on Rushmore
To run the programs on Rushmore using the provided `machinefile.dsu` hostfile, follow the instructions below.

1. run `mpic++ -std=c++17 r1.cpp -o r1` and `mpic++ -std=c++17 r2.cpp -o r2` to build both programs on Rushmore
2. run `mpirun -np=4 --hostfile machinefile.dsu ./r1 50000000 50000` to run the static version with 4 processes
3. run `mpirun -np=4 --hostfile machinefile.dsu ./r2 50000000 50000` to run the dynamic version with 4 processes

For the scaling test, run the dynamic version with the required process counts using the following commands.

1. run `mpirun -np=4 --hostfile machinefile.dsu ./r2 50000000 50000` for one process on each Rushmore machine
2. run `mpirun -np=8 --hostfile machinefile.dsu ./r2 50000000 50000` for two processes on each Rushmore machine
3. run `mpirun -np=16 --hostfile machinefile.dsu ./r2 50000000 50000` for four processes on each Rushmore machine

## Command-line arguments

`N` is the inclusive upper bound of the range `[2, N]`. `CHUNK` is the number of integers placed in each work segment, except when the remaining integers are included in the final segment.

Both programs use the command-line format `{executable} N CHUNK`.

## Primality test

Each integer is tested for primality using trial division. The program tries possible divisors beginning at 2 and stops after reaching the square root of the integer.

## Timing method

I used `MPI_Wtime` to time the parallel work-distribution and computation section. The timer starts after setup, right before the master starts sending work in R1 or receiving requests in R2. It stops after the master has gathered all results and, for R2, sent a stop message to every worker. The sequential correctness check and final output are not included in the timing.

For the times used in the report, I ran each configuration three times and used the average of those runs.

## Assumptions and known limitations

I assume that each program is run with at least two MPI processes because rank 0 acts only as the master and ranks 1 through P-1 perform the work.

For debugging and correctness purposes, the `DEBUG` flag must be set to `true` at the beginning of the file before compilation to force the master worker to sequentially calculate the correct answer and print segment-level information.

## AI use statement

I used AI for only the allowed purposes of debugging and general questions about MPI functions. The link to the chat that I used in ChatGPT is here: [link](https://chatgpt.com/c/6ad4341d-bbec-83ea-85b5-23d0952de7b3)
