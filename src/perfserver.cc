#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "tcpsocket.h"
#include "utils.h"

const int measureIntervalSecs = 10;

template <typename T>
T calcAvg(std::vector<T>* arr) {
  std::vector<T> vals = *arr;
  T arrSum = 0;
  for (auto val : vals) arrSum += val;
  int numElems = vals.size();
  return (arrSum / numElems);
}

int spawnServerProcess(std::string serverBinPath, short portno,
                       int threadpoolSize, std::string logFilePath) {
  int procId;
  check_error((procId = fork()) >= 0, "error while spawnning server process");
  if (procId == 0) {
    char* args[] = {
        strdup(serverBinPath.data()),
        strdup(std::to_string(portno).data()),
        strdup(std::to_string(threadpoolSize).data()),
        strdup(logFilePath.data()),
        NULL,
    };
    execv(args[0], args);
    exit(1);
  }
  return procId;
}

struct ServerMeasureArgs {
 public:
  int serverPid;
  int intervalSecs;
  bool* continueMeasuring;
  ServerMeasureArgs(int s, int i, bool* c)
      : serverPid(s), intervalSecs(i), continueMeasuring(c) {}
};

void* serverMeasure(void* argsPtr) {
  ServerMeasureArgs* args = (ServerMeasureArgs*)argsPtr;

  int serverPid = args->serverPid;
  int intervalSecs = args->intervalSecs;
  bool* continueMeasuring = args->continueMeasuring;

  std::vector<std::vector<double>*>* measurements =
      new std::vector<std::vector<double>*>();

  measurements->push_back(new std::vector<double>());
  measurements->push_back(new std::vector<double>());

  std::ostringstream ostream;

  while (*continueMeasuring) {
    char numThreadsOut[1024];
    FILE* fp;

    ostream << "ps -M " << serverPid << " | grep -v USER | wc -l";
    fp = popen(ostream.str().c_str(), "r");
    if (!fp) continue;
    if (fgets(numThreadsOut, sizeof(numThreadsOut), fp) == NULL) continue;
    std::string numThreadsOutStr(numThreadsOut);
    auto nseNumThreadsOutStr = std::remove_if(numThreadsOutStr.begin(),
                                              numThreadsOutStr.end(), isspace);
    numThreadsOutStr.erase(nseNumThreadsOutStr, numThreadsOutStr.end());
    double numThreads = atof(numThreadsOutStr.c_str());

    pclose(fp);
    ostream.str("");
    ostream.clear();

    char cpuUtlilOut[1024];
    fp = popen(
        "top -l  2 | grep -E \"^CPU\" | tail -1 | awk '{ print $3 + $5 }'",
        "r");
    if (!fp) continue;
    if (fgets(cpuUtlilOut, sizeof(cpuUtlilOut), fp) == NULL) continue;
    std::string cpuUtlilOutStr(cpuUtlilOut);
    auto nseCpuUtlilOutStr =
        std::remove_if(cpuUtlilOutStr.begin(), cpuUtlilOutStr.end(), isspace);
    cpuUtlilOutStr.erase(nseCpuUtlilOutStr, cpuUtlilOutStr.end());
    double cpuUtil = atof(cpuUtlilOutStr.c_str());
    pclose(fp);

    measurements->at(0)->push_back(numThreads);
    measurements->at(1)->push_back(cpuUtil);

    sleep(intervalSecs);
  }

  return measurements;
}

int main(int argc, char* argv[]) {
  // parsing the arguments
  check_error(argc == 6,
              "usage: ./perfserver <port> <logFilePath> <serverBinPath> "
              "<serverPort> <serverThreadpoolSize>");
  int portNo = atoi(argv[1]);
  std::string logFilePath(argv[2]);
  std::string serverBinPath(argv[3]);
  int serverPortNo = atoi(argv[4]);
  int serverThreadpoolSize = atoi(argv[5]);
  long startTime, endTime;

  // setting up the server environment
  int listenerFd = autograder::setupTCPSocket("0.0.0.0", portNo, 4, true);

  bool serverSpawned = false;
  int serverPid = -1;
  std::string serverLogFilePath;
  int numClients = -1;
  bool* continueMeasurnig = new bool;
  pthread_t serverMeasureTh;

  std::ostringstream ostream;
  ostream
      << "echo 'num_clients,throughput,avg_num_threads,avg_cpu_utilization' > "
      << logFilePath;
  system(ostream.str().c_str());
  ostream.str("");
  ostream.clear();

  while (true) {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd = accept(listenerFd, (sockaddr*)&clientAddr, &clientAddrLen);
    check_error(clientFd >= 0, "accept error");

    check_error(
        (read(clientFd, &numClients, sizeof(numClients)) > 0),
        "perfserver - error while reading number of clients from perfclient");

    check_error(numClients > 0, "perfserver - received number of clients as 0");

    if (!serverSpawned) {
      std::cout << "\033[92m"
                << "Starting performance test for " << numClients << " clients"
                << "\033[0m" << std::endl;

      ostream << "/Users/himanshu/Projects/DECS_Project/logs/server_run_"
              << numClients << ".log";
      serverLogFilePath = ostream.str();
      ostream.str("");
      ostream.clear();

      // spawn a new server process
      // retain its process id for future use
      serverPid = spawnServerProcess(serverBinPath, serverPortNo,
                                     serverThreadpoolSize, serverLogFilePath);

      // run a thread for measuring CPU utilization
      // and number of threads
      *continueMeasurnig = true;
      ServerMeasureArgs* smargs = new ServerMeasureArgs(
          serverPid, measureIntervalSecs, continueMeasurnig);
      pthread_create(&serverMeasureTh, NULL, serverMeasure, (void*)smargs);

      // set server spawnned to true
      serverSpawned = true;
      startTime = getTimeInMicroseconds();

      // respond to the perfclient
      int success = 1;
      check_error(write(clientFd, &success, sizeof(success)) >= 0,
                  "perfserver - error while responding to the perfclient");
    } else {
      std::cout << "\033[92m"
                << "Ending performance test for " << numClients << " clients..."
                << "\033[0m" << std::endl;
      // kill server
      // make use of server logs to calculate throughput
      kill(serverPid, SIGKILL);

      // stop thread from measuring and collect results
      *continueMeasurnig = false;
      std::vector<std::vector<double>*>* serverMeasureRes;
      pthread_join(serverMeasureTh, (void**)&serverMeasureRes);

      // note end time
      endTime = getTimeInMicroseconds();

      std::ifstream serverLogFile(serverLogFilePath);
      int numResponses =
          std::count(std::istreambuf_iterator<char>(serverLogFile),
                     std::istreambuf_iterator<char>(), '\n');

      double durationInSecs = (double)(endTime - startTime) / 1000000;
      double throughput = numResponses / durationInSecs;

      double avgNumThreads = calcAvg(serverMeasureRes->at(0));
      double avgCPUUtil = calcAvg(serverMeasureRes->at(1));

      ostream << "echo \"" << numClients << "," << throughput << ","
              << avgNumThreads << "," << avgCPUUtil << "\""
              << ">> " << logFilePath;
      system(ostream.str().c_str());
      ostream.str("");
      ostream.clear();

      // reset state
      serverSpawned = false;
      serverPid = -1;
      numClients = -1;

      // respond to the perfclient
      int success = 1;
      check_error(write(clientFd, &success, sizeof(success)) >= 0,
                  "perfserver - error while responding to the perfclient");
    }

    close(clientFd);
  }

  close(listenerFd);
}
