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

int main(int argc, char* argv[]) {
  check_error(argc == 3,
              "Usage: ./client <serverIP:port> <sourceCodeFileTobeGraded>");
  auto [host, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  short portno = atoi(port.data());

  std::cout << "Evaluation File: " << evaluationFilePath << std::endl;
  std::string data = readFileFromPath(evaluationFilePath);

  std::cout << "=====FILE CONTENTS=====\n";
  std::cout << data << std::endl;

  if (data != "") {
    int conn_fd;
    sockaddr_in server_addr;
    check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
                "error at socket creation");
    // values stored for server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = inet_addr(host.data());
    
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
    // parsing the server response
    autograder::Response* resp =
        autograder::ClientProtocol::parseResponse(conn_fd);
    std::cout << "Response from the server:\n" << resp->body;
    close(conn_fd);
  } else {
    std::cout << "Either empty file or file doesn't exists\n";
  }
}