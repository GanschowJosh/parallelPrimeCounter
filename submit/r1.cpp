#include <mpi.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

//flag for printing segment-level logs and sequential verification
bool DEBUG = true;

int isprime(int n) {
  for(int i = 2; i <= n/i; i++) {
    if(n%i == 0) return 0;
  }
  return 1;
}


//types of tags for messages
enum Tag {
  WORK = 1,
  RESULT = 2
};

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if(argc != 3) {
    if(rank == 0) cout << "run syntax `./r1 N CHUNK`" << endl;
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

  double start = 0.0;

  //maps worker to a vector of segments [a,b]
  unordered_map<int, vector<pair<int, int>>> segs;
  if(rank == 0) {
    int curr_start = 2; //start of next segment
    int curr_worker = 1; //worker to assign next work to
    while(curr_start <= N) {
      int curr_end = min(curr_start + CHUNK - 1, N);
      //add current CHUNK-length segment to current worker's tasklist
      //and bookkeep for the next segment's start and worker
      segs[curr_worker].emplace_back(curr_start, curr_end);
      curr_start = curr_end + 1;
      curr_worker++;
      if(curr_worker == size) {
        curr_worker = 1;
      }
    }

    //translate each worker's segments to a flat list since MPI requires simple data types
    vector<vector<int>> work_messages(size);
    for(int i = 1; i < size; i++) {
      for(auto [a,b] : segs[i]) {
        if(DEBUG) cout << "master sending [" << a << "..." << b << "] to worker " << i << endl;
        work_messages[i].push_back(a);
        work_messages[i].push_back(b);
      }
    }

    start = MPI_Wtime();

    for(int i = 1; i < size; i++) {
      MPI_Send(work_messages[i].data(), work_messages[i].size(), MPI_INT,
               i, WORK, MPI_COMM_WORLD);
    }
  }

  if(rank != 0) { //not the master, do my work
    //need to probe to see how much work im being given. necessary because the number of segments may not perfectly divide the number of workers. some workers may have an additional segment.
    MPI_Status status;
    MPI_Probe(0,WORK,MPI_COMM_WORLD,&status);

    int count;
    MPI_Get_count(&status, MPI_INT, &count);

    vector<int> tmp(count);
    MPI_Recv(tmp.data(), count, MPI_INT, 0, WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //re-creating my tasklist from the flattened one sent from master
    vector<pair<int, int>> tasks;
    for(int i = 0; i < count; i+=2){
      tasks.emplace_back(tmp[i], tmp[i+1]);
    }

    if(DEBUG) {
      for(auto [a,b] : tasks) {
        cout << "worker " <<  rank << " has nums " << a << " through " << b << endl;
      }
    }

    //maintain a local sum over all my segments
    int local_sum = 0;
    for(auto [a,b] : tasks) {
      //current sum for this segment
      int curr = 0;
      for(int i = a; i <= b; i++) {
        curr+=isprime(i);
      }
      if(DEBUG) cout << "worker " << rank << " found " << curr << " primes between " << a << " and " << b << endl;
      local_sum += curr;
    }
    MPI_Send(&local_sum, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
  }

  if(rank == 0) {
    int tmp;
    int tot = 0;
    for(int i = 1; i < size; i++) {
      //master gathers and sums all results
      MPI_Recv(&tmp, 1, MPI_INT, i, RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      tot += tmp;
      if(DEBUG) cout << "master gathered " << tmp << " from worker " << i << endl;
    }

    double taken = MPI_Wtime() - start;

    if(DEBUG) { //if we're in debug mode, sequentially calculate the number of primes in the range for verification
      int expected = 0;
      for(int i = 2; i <= N; i++) {
        expected+=isprime(i);
      }

      cout << "number of primes in range [2," << N << "] = " << tot  << "\nexpected? " << (expected==tot ? "YES" : "NO") << endl;
    }
    else {
      cout << "number of primes in range [2," << N << "] = " << tot  << endl;
    }

    cout << "Time taken: " << taken << " seconds" << "\nN = " << N << ", CHUNK = " << CHUNK << ", P = " << size << endl;
  }

  MPI_Finalize();
  return 0;
}
