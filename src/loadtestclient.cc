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

void clientLoop(std::string serverIP, short port,
                std::string evaluationFilePath) {
  std::cout << "Evaluation File: " << evaluationFilePath << std::endl;
  std::string data = readFileFromPath(evaluationFilePath);

  std::cout << "=====FILE CONTENTS=====\n";
  std::cout << data << std::endl;
  if (data != "") {
    int conn_fd;
    sockaddr_in server_addr;
    check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "error at socket creation");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(serverIP.data());

    check_error(connect(conn_fd, (struct sockaddr*)&server_addr,
                        sizeof(server_addr)) >= 0,
                "error at socket connect");

    autograder::ClientProtocol::sendRequest(
        conn_fd, "CHECK",
        {
            {"Content-Length", std::to_string(data.length())},
        },
        data);

    std::cout << "Sent grading request to the server...\n";

    autograder::Response* resp =
        autograder::ClientProtocol::parseResponse(conn_fd);
    std::cout << "Response from the server:\n" << resp->body << std::endl;
    close(conn_fd);
  } else {
    std::cout << "Either empty file or file doesn't exists\n";
  }
}

void loadTest(std::string serverIP, short port, std::string evaluationFilePath,
              int num_loops, int num_secs = -1) {
  timeval timestamp;
  num_secs = num_secs == -1 ? randInt(1, 5) : num_secs;

  std::vector<std::time_t> respTimes;

  for (int idx = 0; idx < num_loops; ++idx) {
    gettimeofday(&timestamp, NULL);
    std::time_t stTime = timestamp.tv_sec * 1000000 + timestamp.tv_usec;

    clientLoop(serverIP, port, evaluationFilePath);

    gettimeofday(&timestamp, NULL);
    std::time_t enTime = timestamp.tv_sec * 1000000 + timestamp.tv_usec;

    respTimes.push_back(enTime - stTime);
    std::cout << "\033[32m"
              << "Response Time #" << (idx + 1) << ": " << (respTimes.back())
              << " microsecs" << std::endl
              << "\033[0m";
    sleep(num_secs);
  }

  std::time_t respTimeSum = 0;
  for (auto rt : respTimes) respTimeSum += rt;
  std::time_t avgRespTime = respTimeSum / (uint)respTimes.size();
  std::cout << "\033[32m"
            << "Average Response Time: " << avgRespTime << "  microsecs"
            << std::endl
            << "\033[0m";
}

int main(int argc, char* argv[]) {
  check_error(argc == 5,
              "Usage: ./server <serverIP:port> <sourceCodeFileTobeGraded> "
              "<loopNum> <sleepTimeSeconds>");
  auto [host, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  int numloops = atoi(argv[3]);
  int sleeptimeseconds = atoi(argv[4]);
  short portno = atoi(port.data());

  loadTest(host, portno, evaluationFilePath, numloops, sleeptimeseconds);
  //   clientLoop(host, portno, evaluationFilePath);
  return 0;
}