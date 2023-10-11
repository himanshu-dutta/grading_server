#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "config.h"
#include "protocol.h"
#include "tcpsocket.h"
#include "types.h"
#include "utils.h"

#define BACKLOG 8

struct LoadClientState {
 public:
  int numLoops;
  int numSecs;
  int numSuccessfulResp;
  std::time_t loopStTime;
  std::time_t loopEnTime;
  std::vector<std::time_t> respTimes;
  LoadClientState(int numLoops, int numSecs)
      : numLoops(numLoops),
        numSecs(numSecs),
        numSuccessfulResp(0),
        loopStTime(-1),
        loopEnTime(-1),
        respTimes(0) {}
};

std::time_t getTimeInMicroseconds() {
  timeval timestamp;
  gettimeofday(&timestamp, NULL);
  std::time_t tm = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
  return tm;
}

bool clientLoop(int connFd, std::string evaluationFileData) {
  bool sentRequestSuccessfully = autograder::ClientProtocol::sendRequest(
      connFd, "CHECK",
      {
          {"Content-Length", std::to_string(evaluationFileData.length())},
      },
      evaluationFileData);
  if (!sentRequestSuccessfully) return false;
  std::cout << "Sent grading request to the server...\n";

  autograder::Response* resp =
      autograder::ClientProtocol::parseResponse(connFd);
  std::cout << "Response from the server:\n" << resp->body << std::endl;

  return (resp->resp_type != "ERROR");
}

void loadTest(int connFd, std::string evaluationFileData,
              LoadClientState& state) {
  state.numSecs = state.numSecs == -1 ? randInt(1, 5) : state.numSecs;
  state.loopStTime = getTimeInMicroseconds();

  while (state.numLoops--) {
    std::time_t reqStTime = getTimeInMicroseconds();

    bool gotSuccessfulResponse = clientLoop(connFd, evaluationFileData);
    state.numSuccessfulResp += gotSuccessfulResponse ? 1 : 0;

    std::time_t reqEnTime = getTimeInMicroseconds();

    state.respTimes.push_back(reqEnTime - reqStTime);
    sleep(state.numSecs);
    if (!gotSuccessfulResponse) return;
  }

  state.loopEnTime = getTimeInMicroseconds();
  std::time_t loopTime = state.loopEnTime - state.loopStTime;

  std::time_t respTimeSum = 0;
  for (auto rt : state.respTimes) respTimeSum += rt;
  std::time_t avgRespTime = respTimeSum / (uint)state.respTimes.size();
  std::cout << "ART:" << avgRespTime << ",";
  std::cout << "NSR:" << state.numSuccessfulResp << ",";
  std::cout << "LT:" << loopTime << std::endl;
}

int main(int argc, char* argv[]) {
  check_error(argc == 5,
              "Usage: ./server <serverIP:port> <sourceCodeFileTobeGraded> "
              "<loopNum> <sleepTimeSeconds>");
  auto [serverIP, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  int numloops = atoi(argv[3]);
  int sleeptimeseconds = atoi(argv[4]);
  short portno = atoi(port.data());

  std::cout << "Evaluation File: " << evaluationFilePath << std::endl;
  std::string data = readFileFromPath(evaluationFilePath);

  std::cout << "=====FILE CONTENTS=====\n";
  std::cout << data << std::endl;
  std::cout << "=======================\n";
  if (data.size() <= 0)
    std::cout << "Either empty file or file doesn't exists\n";

  LoadClientState lcState(numloops, sleeptimeseconds);

  while (lcState.numLoops > 0) {
    std::cout << "Establishing connection for #request: " << lcState.numLoops
              << std::endl;
    int conn_fd;
    sockaddr_in server_addr;
    check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "error at socket creation", false);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = inet_addr(serverIP.data());

    int connectResp;
    check_error((connectResp = connect(conn_fd, (struct sockaddr*)&server_addr,
                                       sizeof(server_addr)) >= 0),
                "error at socket connect", false);

    if (connectResp >= 0) loadTest(conn_fd, data, lcState);

    close(conn_fd);
  }

  //   clientLoop(host, portno, evaluationFilePath);
  return 0;
}