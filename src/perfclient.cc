#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <climits>
#include <functional>
#include <iostream>
#include <sstream>

#include "utils.h"

template <typename T>
T calcAvg(std::vector<T>* arr) {
  std::vector<T> vals = *arr;
  T arrSum = 0;
  for (auto val : vals) arrSum += val;
  int numElems = vals.size();
  return (arrSum / numElems);
}

std::function<int()> makeClientFn(std::string clientBinPath,
                                  std::string serverIp, std::string serverPort,
                                  std::string sourceCodeFilePath,
                                  double clientTimeout) {
  std::function<int()> clientFn = [&]() -> int {
    int procId;

    check_error((procId = fork()) >= 0,
                "perfclient - error while spawnning client process");

    if (procId == 0) {
      char* args[] = {
          strdup(clientBinPath.data()),
          strdup((serverIp + ":" + serverPort).data()),
          strdup(sourceCodeFilePath.data()),
          strdup(std::to_string(clientTimeout).data()),
          NULL,
      };
      execv(args[0], args);
      exit(1);
    }

    int status;
    check_error(waitpid(procId, &status, 0) >= 0,
                "perfclient - waiting for client to exit");

    if (!WIFEXITED(status)) return 0;
    return (int)WEXITSTATUS(status);
  };
  return clientFn;
}

enum class ClientReturnStatus { SUCCESS, TIMEOUT, ERROR };

struct ClientProcessRecord {
 public:
  int numSuccessful;
  int numTimeout;
  int numError;
  long clientStTime;
  long clientEnTime;
  long avgResponseTime;

  ClientProcessRecord(int nSuccessful = 0, int nTimeout = 0, int nError = 0,
                      long cST = 0, long cET = 0, long aRT = 0)
      : numSuccessful(nSuccessful),
        numTimeout(nTimeout),
        numError(nError),
        clientStTime(cST),
        clientEnTime(cET),
        avgResponseTime(aRT) {}
};

struct ClientProcessArgs {
 public:
  std::function<int()> clientFn;
  int numRequests;

  ClientProcessArgs(std::function<int()> cFn, int nR)
      : clientFn(cFn), numRequests(nR) {}
};

void* clientProcess(void* _args) {
  ClientProcessArgs* args = (ClientProcessArgs*)_args;
  std::function<int()> clientFn = args->clientFn;
  int numRequests = args->numRequests;

  ClientProcessRecord* cpr = new ClientProcessRecord();
  std::vector<long> respTime;

  long stTime = getTimeInMicroseconds();

  // make numRequests number of calls to client sequentially. For each request,
  // note down the start time and end time also note down the number of
  // successful, timedout and error results.
  for (int reqId = 0; reqId < numRequests; ++reqId) {
    long reqStTime = getTimeInMicroseconds();
    ClientReturnStatus clientRetStatus = (ClientReturnStatus)clientFn();
    long reqEnTime = getTimeInMicroseconds();

    if (clientRetStatus == ClientReturnStatus::SUCCESS) {
      cpr->numSuccessful += 1;
      respTime.push_back(reqEnTime - reqStTime);
    } else if (clientRetStatus == ClientReturnStatus::TIMEOUT) {
      cpr->numTimeout += 1;
    } else {
      cpr->numError += 1;
    }
  }

  long enTime = getTimeInMicroseconds();
  cpr->clientStTime = stTime;
  cpr->clientEnTime = enTime;
  cpr->avgResponseTime = calcAvg(&respTime);

  return (void*)cpr;
}

void perfClientExecute(std::string logFilePath, int numConcurrentClients,
                       int numRequestsPerClient, std::string clientBinPath,
                       std::string serverIp, std::string serverPort,
                       std::string sourceCodeFilePath, double clientTimeout) {
  // spawn numConcurrentClients processes concurrently
  // each process should make numRequestsPerClient number of requests
  // serially

  int numThreadsCollected = 0;
  std::vector<pthread_t>* finishedThreads = new std::vector<pthread_t>(0);

  std::function<int()> clientFn = makeClientFn(
      clientBinPath, serverIp, serverPort, sourceCodeFilePath, clientTimeout);
  ClientProcessArgs* args =
      new ClientProcessArgs(clientFn, numRequestsPerClient);
  std::vector<ClientProcessRecord*> records(numConcurrentClients);

  for (int clientProcId = 0; clientProcId < numConcurrentClients;
       ++clientProcId) {
    pthread_t tId;
    pthread_create(&tId, NULL, clientProcess, (void*)args);
    finishedThreads->push_back(tId);
  }
  for (auto tId : *finishedThreads) {
    pthread_join(tId, (void**)&records[numThreadsCollected++]);
  }

  delete finishedThreads;

  // processing the per client output
  int totalNumSuccessfulReq = 0;
  int totalNumTimeoutReq = 0;
  int totalNumErrorReq = 0;
  long earliestStTime = LONG_MAX;
  long latestEnTime = LONG_MIN;
  double respTimes = 0;

  for (auto record : records) {
    totalNumSuccessfulReq += record->numSuccessful;
    totalNumTimeoutReq += record->numTimeout;
    totalNumErrorReq += record->numError;
    earliestStTime = std::min({earliestStTime, record->clientStTime});
    latestEnTime = std::max({latestEnTime, record->clientEnTime});
    respTimes += record->avgResponseTime;
  }

  double duration = (double)(latestEnTime - earliestStTime) / 1000000;
  double reqSentRate =
      (double)(numConcurrentClients * numRequestsPerClient) / duration;
  double successfulReqRate = (double)totalNumSuccessfulReq / duration;
  double timeoutReqRate = (double)totalNumTimeoutReq / duration;
  double errorReqRate = (double)totalNumErrorReq / duration;
  double avgRespTime = respTimes / (double)(1000000 * records.size());

  std::ostringstream ostream;
  ostream << "echo \"" << numConcurrentClients << "," << numRequestsPerClient
          << "," << reqSentRate << "," << successfulReqRate << ","
          << timeoutReqRate << "," << errorReqRate << "," << avgRespTime
          << "\" >> " << logFilePath;

  system(ostream.str().c_str());
  ostream.str("");
  ostream.clear();
}

int main(int argc, char* argv[]) {
  // parsing args
  check_error(argc == 9,
              "Usage: ./perfclient <perfserverIP:port> <numClients> "
              "<numRequests> <logFilePath> "
              "<clientBinPath> <serverIP:port> <sourceCodeFileToBeGraded> "
              "<clientTimeout>");

  auto [perfHost, perfPort] = splitHostPort(argv[1]);
  int numClients = atoi(argv[2]);
  int numRequests = atoi(argv[3]);
  std::string logFilePath(argv[4]);
  std::string clientBinPath(argv[5]);
  auto [serverHost, serverPort] = splitHostPort(argv[6]);
  std::string sourceCodeFileToBeGraded(argv[7]);
  double clientTimeout = atof(argv[8]);

  int connFd;
  int status;
  sockaddr_in perfServerAddr = getSockaddrIn(perfHost, atoi(perfPort.data()));

  std::cout << "\033[92m"
            << "Starting performance test for " << numClients << " clients"
            << "\033[0m" << std::endl;
  check_error((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "perfclient - error at socket creation");
  check_error(connect(connFd, (struct sockaddr*)&perfServerAddr,
                      sizeof(perfServerAddr)) >= 0,
              "perfclient - error at socket connect");
  write(connFd, &numClients, sizeof(numClients));
  read(connFd, &status, sizeof(status));
  close(connFd);

  perfClientExecute(logFilePath, numClients, numRequests, clientBinPath,
                    serverHost, serverPort, sourceCodeFileToBeGraded,
                    clientTimeout);

  std::cout << "\033[92m"
            << "Ending performance client for " << numClients << " clients"
            << "\033[0m" << std::endl;
  check_error((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "perfclient - error at socket creation");
  check_error(connect(connFd, (struct sockaddr*)&perfServerAddr,
                      sizeof(perfServerAddr)) >= 0,
              "perfclient - error at socket connect");
  write(connFd, &numClients, sizeof(numClients));
  read(connFd, &status, sizeof(status));
  close(connFd);
}