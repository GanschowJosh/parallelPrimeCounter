#include <mpi.h>
#include <unistd.h>
#include <bits/stdc++.h>

bool DEBUG = false;


using namespace std;


int isprime(int n) {
  for(int i = 2; i <= n/i; i++) {
    if(n%i == 0) return 0;
  }
  return 1;
}

//tags for message types
enum Tags {
  WORK=0,
  RESULT=1,
  REQUEST=2,
  STOP=3
};

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if(argc != 3) {
    if(rank == 0) cout << "run syntax `./r2 N CHUNK`" << endl;
    MPI_Finalize();
    return 1;
  }

  int N = atoi(argv[1]);
  int CHUNK = atoi(argv[2]);
  if(CHUNK <= 0) {
    if(rank == 0) cout << "CHUNK must be positive" << endl;
    MPI_Finalize();
    return 1;
  }
  if(size < 2) {
    if(rank == 0) cout << "run with at least 2 MPI processes" << endl;
    MPI_Finalize();
    return 1;
  }

  char hostname[256];
  gethostname(hostname, sizeof(hostname));

  cout << "Rank: " << rank << "\tProcesses: " << size << "\tHostname: " << hostname << endl;

  if(rank == 0) {
    //pretty much the same logic as r1 for bookkeeping of segments
    int curr_start = 2;
    int stopped = 0;
    MPI_Status status;
    int total = 0;
    int current;

    //start the timer
    double start = MPI_Wtime();

    //keep going while not all the workers have received a stop job
    while (stopped < size-1) {
      //receive any type of message from my minions
      MPI_Recv(&current, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG==REQUEST) {
        if(curr_start <= N) {
          //a single array of length 2 to represent range [a,b]
          int task[2]={curr_start, min(curr_start+CHUNK-1, N)};
          MPI_Send(task, 2, MPI_INT, status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
          curr_start = task[1]+1; //next segment will start right after this one ends
        } else { //no work left, send a stop job to the worker that asked for work
          MPI_Send(&current, 1, MPI_INT, status.MPI_SOURCE, STOP, MPI_COMM_WORLD);
          stopped++; //keep track of how many STOPs ive sent
        }
      }
      if(status.MPI_TAG==RESULT) { //got a result from a worker
        total+=current;
      }
    }

    double taken = MPI_Wtime()-start;

    if(DEBUG) {
      int expected = 0;
      for(int i = 2; i <= N; i++) {
        expected+=isprime(i);
      }

      cout << "number of primes in range [2," << N << "] = " << total << "\ncorrect? " << (expected==total ? "YES" : "NO")<< endl;
    }
    else {
      cout << "number of primes in range [2," << N << "] = " << total << endl;
    }

    cout << "Time taken: " << taken << " seconds"
         << "\nN = " << N << ", CHUNK = " << CHUNK << ", P = " << size << endl;
  }

  if(rank != 0) { //im a worker
    int task[2];
    int local_sum = 0;
    int dummy = 67;
    MPI_Status status;
    int self;
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    while(1) {
      //request work. the payload is a dummy since only the tag matters
      MPI_Send(&dummy, 1, MPI_INT, 0, REQUEST, MPI_COMM_WORLD);
      //receive work from overlord
      MPI_Recv(task, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG == STOP) {
        break;
      }
      local_sum=0;
      for(int i = task[0]; i <= task[1]; i++) {
        local_sum+=isprime(i);
      }

      if (DEBUG) cout << "[RESULT]: worker " << self << " found " << local_sum << " primes between " << task[0] << " and " << task[1] << endl;
      //send result to overlord
      MPI_Send(&local_sum, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
  return 0;
}
