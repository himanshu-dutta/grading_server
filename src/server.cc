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
#include "tcpsocket.h"
#include "threadpool.h"
#include "utils.h"

#define BACKLOG 16

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

  void* handler(void* clientFdPtr) {
    int clientFd = *(int*)clientFdPtr;
    std::cout << "================================================\n";
    std::cout << "Connected to a client: " << clientFd << std::endl;

    while (true) {
      autograder::Request* req =
          autograder::ServerProtocol::parseRequest(clientFd);
      if (req->req_type == "ERROR") break;
      autograder::Response* resp = (*grader)(req);
      bool sentSuccessfully =
          autograder::ServerProtocol::sendResponse(clientFd, resp);
      if (sentSuccessfully) logger->info("sent response successfully");
    }

    int resp = close(clientFd);
    if (resp != 0) std::cout << "CLIENT CLOSE UNSUCCESSFUL!!\n";
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

  int listenerFd = autograder::setupTCPSocket(SERVER_IP, portno, BACKLOG, true);

  autograder::Threadpool threadpool(threadpoolSize);
  std::function<void*(void*)> threadFunc = [&](void* args) -> void* {
    return app->handler(args);
  };

  shutdownHandler = [&](int sig) -> void {
    std::cerr << "Shutting the server down, yo!\n";
    threadpool.close();
    close(listenerFd);
  };

  while (true) {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd = accept(listenerFd, (sockaddr*)&clientAddr, &clientAddrLen);
    check_error(clientFd >= 0, "accept error");
    int* clientFdCopy = new int;
    *clientFdCopy = clientFd;
    threadpool.queueJob(threadFunc, clientFdCopy);
  }
}
// #############################################################################