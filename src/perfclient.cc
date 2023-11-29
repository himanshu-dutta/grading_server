#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <climits>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "utils.h"

inline bool fileExists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

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
                                  double pollTime) {
  std::function<int()> clientFn = [&]() -> int {
    FILE* fp;
    std::ostringstream ostream;

    ostream << clientBinPath << " new " << (serverIp + ":" + serverPort) << " "
            << sourceCodeFilePath;
    fp = popen(ostream.str().c_str(), "r");
    char outBuff[4096];
    if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
    if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
    if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
    pclose(fp);
    std::string ln(outBuff);

    int delPos = ln.find(":");
    std::string token = ln.substr(0, delPos);

    while (true) {
      sleep(pollTime);

      FILE* fp;
      std::ostringstream ostream;

      ostream << clientBinPath << " status " << (serverIp + ":" + serverPort)
              << " " << token;
      fp = popen(ostream.str().c_str(), "r");
      char outBuff[4096];
      if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
      if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
      if (fgets(outBuff, sizeof(outBuff), fp) == NULL) return 1;
      pclose(fp);
      std::string ln(outBuff);

      int donePos = ln.find("DONE");
      if (donePos != std::string::npos) return 0;

      int errorPos = ln.find("ERROR");
      if (errorPos != std::string::npos) return 1;
    }
    return 1;
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
                       std::string sourceCodeFilePath, double clientPollTime) {
  // spawn numConcurrentClients processes concurrently
  // each process should make numRequestsPerClient number of requests
  // serially

  int numThreadsCollected = 0;
  std::vector<pthread_t>* finishedThreads = new std::vector<pthread_t>(0);

  std::function<int()> clientFn = makeClientFn(
      clientBinPath, serverIp, serverPort, sourceCodeFilePath, clientPollTime);
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
              "<clientPollTime>");

  auto [perfHost, perfPort] = splitHostPort(argv[1]);
  int numClients = atoi(argv[2]);
  int numRequests = atoi(argv[3]);
  std::string logFilePath(argv[4]);
  std::string clientBinPath(argv[5]);
  auto [serverHost, serverPort] = splitHostPort(argv[6]);
  std::string sourceCodeFileToBeGraded(argv[7]);
  double clientPollTime = atof(argv[8]);

  if (!fileExists(logFilePath)) {
    std::ostringstream ostream;
    ostream << "echo "
               "'num_clients,num_req_per_client,req_sent_rate,successful_req_"
               "rate,timeout_req_rate,error_req_rate,avg_resp_time' > "
            << logFilePath;
    system(ostream.str().c_str());
    ostream.str("");
    ostream.clear();
  }

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
                    clientPollTime);

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