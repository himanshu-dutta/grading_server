#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "config.h"
#include "protocol.h"
#include "tcpsocket.h"
#include "types.h"
#include "utils.h"

#define BACKLOG 1
#define MAX_CONN_TRIES 3
// struct timeval time_out;
struct timespec time_out;

void setNonBlocking(int sockfd)
{
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1)
  {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
  if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
  {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
}

struct LoadClientState
{
public:
  int numLoops;
  int numSecs;
  int numSuccessfulResp;
  long loopStTime;
  long loopEnTime;
  LoadClientState(int numLoops, int numSecs)
      : numLoops(numLoops),
        numSecs(numSecs),
        numSuccessfulResp(0),
        loopStTime(-1),
        loopEnTime(-1) {}
};

// variable for response thread
pthread_t resp_th;
pthread_mutex_t resp_mu;
pthread_cond_t resp_cond;
// sockaddr_in server_addr;
// int connectResp;

void *ResponseWrapper(void *connFd_ptr)
{
  int connFd = *(int *)connFd_ptr;
  autograder::Response *resp = autograder::ClientProtocol::parseResponse(connFd);
  pthread_cond_signal(&resp_cond);
  pthread_exit((void *)resp);
}

int totalTimeout=0;
bool clientLoop(int connFd, std::string evaluationFileData)
{
  bool sentRequestSuccessfully = autograder::ClientProtocol::sendRequest(

      connFd, "CHECK",
      {
          {"Content-Length", std::to_string(evaluationFileData.length())},
      },
      evaluationFileData);
  if (!sentRequestSuccessfully)
    return false;
  std::cout << "Sent grading request to the server...\n";

   void* resp=NULL;
  // autograder::ClientProtocol::parseResponse(connFd);
  pthread_cond_init(&resp_cond, NULL);
  pthread_mutex_init(&resp_mu, NULL);
  pthread_create(&resp_th, NULL, &ResponseWrapper, &connFd);
  int n = pthread_cond_timedwait(&resp_cond, &resp_mu, &time_out);
  if(n>=0){
      pthread_join(resp_th,&resp);
      std::cout << "Response from the server:\n"
            << ((autograder::Response* )resp)->body << std::endl;
    return (((autograder::Response* )resp)->resp_type != "ERROR");
  }
  else{
    totalTimeout++;
    std::cout<<"timeout occured for req-res";  
    return false;
    }
}

void loadTest(int connFd, std::string evaluationFileData,
              LoadClientState &state)
{
  state.numSecs = state.numSecs == -1 ? randInt(1, 5) : state.numSecs;

  while (state.numLoops)
  {
    bool gotSuccessfulResponse = clientLoop(connFd, evaluationFileData);
    state.numSuccessfulResp += gotSuccessfulResponse ? 1 : 0;
    sleep(state.numSecs);
    state.numLoops--;
  }
}

pthread_t conn_th;
pthread_mutex_t conn_mu;
pthread_cond_t conn_cond;
sockaddr_in server_addr;
int connectResp;

void *connectWrapper(void *conn_fd_ptr)
{
  int conn_fd = *(int *)conn_fd_ptr;
  int connectResp;
  check_error((connectResp = connect(conn_fd, (struct sockaddr *)&server_addr,
                                     sizeof(server_addr)) >= 0),
              "error at socket connect", true);
  pthread_cond_signal(&conn_cond);
  return NULL;
};

int main(int argc, char *argv[])
{
  check_error(argc == 6,
              "Usage: ./server <serverIP:port> <sourceCodeFileTobeGraded> "
              "<loopNum> <sleepTimeSeconds> <timeout-seconds>");
  auto [serverIP, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  int numloops = atoi(argv[3]);
  int sleeptimeseconds = atoi(argv[4]);
  short portno = atoi(port.data());

  // time_out.tv_sec = atoi(argv[5]);
  // time_out.tv_usec = 0;
  time_out.tv_sec = atoi(argv[5]);
  time_out.tv_nsec = 0;

  std::cout << "Evaluation File: " << evaluationFilePath << std::endl;
  std::string data = readFileFromPath(evaluationFilePath);

  std::cout << "=====FILE CONTENTS=====\n";
  std::cout << data << std::endl;
  std::cout << "=======================\n";
  if (data.size() <= 0)
    std::cout << "Either empty file or file doesn't exists\n";

  LoadClientState lcState(numloops, sleeptimeseconds);
  int numConnTries = 0;
  //   while (numConnTries < MAX_CONN_TRIES) {
  std::cout << "Creating socket" << std::endl;
  int conn_fd;

  check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "error at socket creation", false);

  // setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portno);
  server_addr.sin_addr.s_addr = inet_addr(serverIP.data());

  std::cout << "Trying to establish connection " << std::endl;
  connectResp = -1;
  lcState.loopStTime = getTimeInMicroseconds();

  pthread_cond_init(&conn_cond, NULL);
  pthread_mutex_init(&conn_mu, NULL);
  pthread_create(&conn_th, NULL, &connectWrapper, &conn_fd);
  struct timeval tv;
  struct timespec ts;
  gettimeofday(&tv, NULL);
  ts.tv_sec = time(NULL) + 5000 / 1000;
  ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (5000 % 1000);
  ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
  ts.tv_nsec %= (1000 * 1000 * 1000);

  std::cout << "Before connection\n";
  int n = pthread_cond_timedwait(&conn_cond, &conn_mu, &ts);

  if (n >= 0)
  {
    std::cout << "Established connection\n";
    loadTest(conn_fd, data, lcState);
    lcState.loopEnTime = getTimeInMicroseconds();
    long loopTime = lcState.loopEnTime - lcState.loopStTime;
    long avgRespTime = loopTime / (uint)lcState.numSuccessfulResp;
    long ttOut = totalTimeout / loopTime;

    std::cout << "ATiOUT:" << ttOut << ",";
    std::cout << "ART:" << avgRespTime << ",";
    std::cout << "NSR:" << lcState.numSuccessfulResp << ",";
    std::cout << "LT:" << loopTime << std::endl;
    // break;
  }
  else
  {
    std::cout << "Failed to establish connection\n";
    numConnTries++;
    close(conn_fd);
    // continue;
  }

  close(conn_fd);
  //   }

  return 0;
}