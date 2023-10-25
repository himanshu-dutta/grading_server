#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "config.h"
#include "grader.h"
#include "logger.h"
#include "protocol.h"
#include "threadpool.h"
#include "utils.h"

#define BACKLOG 1

autograder::FileLogger* logger;

// #############################################################################
// SHUTDOWN SIGNAL HANDLER
namespace {
std::function<void(int)> shutdownHandler;
void shutdownSignalHandler(int signal) {
  shutdownHandler(signal);
  exit(0);
}
}  // namespace
// #############################################################################

// #############################################################################
// APPLICATION
class Application {
 public:
  autograder::Grader* grader;
  Application(autograder::Grader* grader) : grader(grader) {}

  void* handler(void* client_fd_ptr) {
    int client_fd = *(int*)client_fd_ptr;
    std::cout << "================================================\n";
    std::cout << "Connected to a client...\n";

    while (true) {
      autograder::Request* req =
          autograder::ServerProtocol::parseRequest(client_fd);
      if (req->req_type == "ERROR") break;
      autograder::Response* resp = (*grader)(req);
      bool sentSuccessfully =
          autograder::ServerProtocol::sendResponse(client_fd, resp);
      if (sentSuccessfully) logger->info("sent response successfully");
    }

    close(client_fd);
    std::cout << "================================================\n";
    return nullptr;
  }
};
// #############################################################################

// #############################################################################
// MAIN EXECUTION LOOP
int main(int argc, char* argv[]) {
  // registering signal handlers
  signal(SIGINT, shutdownSignalHandler);
  signal(SIGKILL, shutdownSignalHandler);

  // parsing the arguments
  check_error(argc == 3 || argc == 4,
              "Usage: ./server <port> <threadpoolSize> <logFilePath>");
  short portno = atoi(argv[1]);
  int threadpoolSize = atoi(argv[2]);
  std::string logFilePath(argv[3]);

  // logging the working directory
  logCWD();
  logger = new autograder::FileLogger(logFilePath);

  Application* app = new Application(
      new autograder::Grader(GRADER_ROOT_DIR, IDLE_OUT_FILE_PATH));

  int listener_fd, client_fd;
  sockaddr_in server_addr = getSockaddrIn(SERVER_IP, portno), client_addr;
  check_error((listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "error at socket creation");

  const int32_t enable = 1;
  check_error((setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &enable,
                          sizeof(int))) >= 0,
              "setsockopt error");

  check_error(
      (bind(listener_fd, (sockaddr*)&server_addr, sizeof(server_addr))) >= 0,
      "bind error");
  check_error((listen(listener_fd, BACKLOG)) >= 0, "listen error");

  autograder::Threadpool threadpool(threadpoolSize);
  std::function<void*(void*)> threadFunc = [&](void* args) -> void* {
    return app->handler(args);
  };

  shutdownHandler = [&](int sig) -> void {
    std::cerr << "Shutting the server down, yo!\n";
    threadpool.close();
    close(listener_fd);
  };

  while (true) {
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(listener_fd, (sockaddr*)&client_addr, &client_addr_len);
    check_error(client_fd >= 0, "accept error");
    threadpool.queueJob(threadFunc, &client_fd);
  }
}
// #############################################################################