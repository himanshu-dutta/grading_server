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
  check_error(argc == 4,
              "Usage: ./client <new|status> <serverIP:port> "
              "<sourceCodeFileTobeGraded|requestID>");

  std::string reqType(argv[1]);
  auto [host, port] = splitHostPort(argv[2]);

  short portno = atoi(port.data());

  if (reqType == "status") {
    std::string token = argv[3];
    int connFd;
    check_error((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "client - error at socket creation");

    sockaddr_in serverAddr = getSockaddrIn(host, portno);
    check_error(
        connect(connFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) >= 0,
        "client - error at socket connect");

    autograder::ClientProtocol::sendRequest(
        connFd, "STATUS",
        {
            {"Content-Length", std::to_string(token.length())},
        },
        token);

    std::cout << "Sent status request to the server...\n";

    autograder::Response* resp =
        autograder::ClientProtocol::parseResponse(connFd);
    std::cout << "Response from the server:\n" << resp->body << std::endl;
    close(connFd);

    std::size_t found = resp->body.find("DONE");
    if (found != std::string::npos) exit(0);
    exit(1);
  } else if (reqType == "new") {
    std::string evaluationFilePath = argv[3];
    std::string data = readFileFromPath(evaluationFilePath);
    int numTry = 0;

    if (data != "") {
      int connFd;
      check_error((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                  "client - error at socket creation");

      sockaddr_in serverAddr = getSockaddrIn(host, portno);
      check_error(connect(connFd, (struct sockaddr*)&serverAddr,
                          sizeof(serverAddr)) >= 0,
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
  } else {
    std::cout << "Wrong request type: " << reqType << std::endl;
    exit(1);
  }

  exit(0);
}