#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "config.h"
#include "grader.h"
#include "protocol.h"
#include "utils.h"

#define BACKLOG 16

namespace {
std::function<void(int)> shutdownHandler;
void signalHandler(int signal) {
  shutdownHandler(signal);
  exit(0);
}
}  // namespace

int main(int argc, char* argv[]) {
  signal(SIGINT, signalHandler);
  signal(SIGKILL, signalHandler);

  check_error(argc == 2, "Usage: ./server <port>");
  short portno = atoi(argv[1]);

  char cwd[BUFF_SIZE];
  getcwd(cwd, BUFF_SIZE);
  std::cout << "WORKING DIR: " << cwd << std::endl;

  autograder::Grader* grader =
      new autograder::Grader(GRADER_ROOT_DIR, IDLE_OUT_FILE_PATH);

  int listener_fd, client_fd;
  sockaddr_in server_addr, client_addr;
  check_error((listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "error at socket creation");

  const int32_t enable = 1;
  check_error((setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &enable,
                          sizeof(int))) >= 0,
              "setsockopt error");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portno);
  server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

  check_error(
      (bind(listener_fd, (sockaddr*)&server_addr, sizeof(server_addr))) >= 0,
      "bind error");
  check_error((listen(listener_fd, BACKLOG)) >= 0, "listen error");

  shutdownHandler = [&](int sig) -> void {
    std::cerr << "Shutting the server down, yo!\n";
    close(listener_fd);
  };

  while (true) {
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(listener_fd, (sockaddr*)&client_addr, &client_addr_len);
    check_error(client_fd >= 0, "accept error");

    std::cout << "================================================\n";
    std::cout << "Connected to a client...\n";

    while (true) {
      autograder::Request* req =
          autograder::ServerProtocol::parseRequest(client_fd);
      //   std::cout << "Received client request for: " << req->req_type
      //             << std::endl;
      if (req->req_type == "ERROR") break;
      autograder::Response* resp = (*grader)(req);
      autograder::ServerProtocol::sendResponse(client_fd, resp);
    }

    close(client_fd);
    std::cout << "================================================\n";
  }
}