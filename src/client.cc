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

int main(int argc, char* argv[]) {
  check_error(
      argc == 4,
      "Usage: ./client <serverIP:port> <sourceCodeFileTobeGraded> <timeout>");
  auto [host, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  short portno = atoi(port.data());
  float timeOutVal = atof(argv[3]);
  struct timeval daytime;
  std::string data = readFileFromPath(evaluationFilePath);
  int numTry = 0;

  if (data != "") {
    int connFd;
    check_error((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "client - error at socket creation");

    sockaddr_in serverAddr = getSockaddrIn(host, portno);
    check_error(
        connect(connFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) >= 0,
        "client - error at socket connect");

    autograder::ClientProtocol::sendRequest(
        connFd, "CHECK",
        {
            {"Content-Length", std::to_string(data.length())},
        },
        data);

    std::cout << "Sent grading request to the server...\n";

    autograder::Response* resp =
        autograder::ClientProtocol::parseResponse(connFd);
    std::cout << "Response from the server:\n" << resp->body << std::endl;
    close(connFd);
  } else {
    std::cout << "Either empty file or file doesn't exists\n";
  }

  exit(0);
}