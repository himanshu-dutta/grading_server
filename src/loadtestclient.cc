#include <arpa/inet.h>
#include <sys/socket.h>
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
#define MAX_CONN_TRIES 3

struct LoadClientState {
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

  while (state.numLoops) {
    bool gotSuccessfulResponse = clientLoop(connFd, evaluationFileData);
    state.numSuccessfulResp += gotSuccessfulResponse ? 1 : 0;
    sleep(state.numSecs);
    state.numLoops--;
  }
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
  int numConnTries = 0;
  while (numConnTries < MAX_CONN_TRIES) {
    std::cout << "Establishing connection" << std::endl;
    int conn_fd;
    sockaddr_in server_addr;
    check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "error at socket creation", false);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = inet_addr(serverIP.data());

    int connectResp;
    lcState.loopStTime = getTimeInMicroseconds();
    check_error((connectResp = connect(conn_fd, (struct sockaddr*)&server_addr,
                                       sizeof(server_addr)) >= 0),
                "error at socket connect", false);

    if (connectResp >= 0) {
      loadTest(conn_fd, data, lcState);
      lcState.loopEnTime = getTimeInMicroseconds();
      long loopTime = lcState.loopEnTime - lcState.loopStTime;
      long avgRespTime = loopTime / (uint)lcState.numSuccessfulResp;
      std::cout << "ART:" << avgRespTime << ",";
      std::cout << "NSR:" << lcState.numSuccessfulResp << ",";
      std::cout << "LT:" << loopTime << std::endl;
      break;
    } else {
      numConnTries++;
      close(conn_fd);
      continue;
    }
    close(conn_fd);
  }

  return 0;
}