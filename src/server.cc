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
#include "protocol.h"
#include "utils.h"

#define BACKLOG 1
#define SIGSERV 10

namespace {
std::function<void(int)> shutdownHandler;
void shutdownSignalHandler(int signal) {
  shutdownHandler(signal);
  exit(0);
}
}  // namespace

namespace {
std::function<void(int)> serverStateHandler;
void serverStateSignalHandler(int signal) { serverStateHandler(signal); }
}  // namespace

class ServerState {
 public:
  time_t serverStartTime;
  time_t serverEndTime;
  int numSuccessfulReqResp;
  bool startedRecording;
  int numClients;
  ServerState()
      : serverStartTime(0),
        serverEndTime(0),
        numSuccessfulReqResp(0),
        startedRecording(false) {}
  void reset() {
    serverStartTime = serverEndTime = 0;
    numSuccessfulReqResp = 0;
    startedRecording = false;
  }
};

class Application {
 public:
  autograder::Grader* grader;
  Application(autograder::Grader* grader) : grader(grader) {}
};

Application* app;

void* handler(void* client_fd_ptr) {
  long stTime = getTimeInMicroseconds();

  int client_fd = *(int*)client_fd_ptr;
  std::cout << "================================================\n";
  std::cout << "Connected to a client...\n";

  while (true) {
    autograder::Request* req =
        autograder::ServerProtocol::parseRequest(client_fd);
    if (req->req_type == "ERROR") break;
    autograder::Response* resp = (*app->grader)(req);
    autograder::ServerProtocol::sendResponse(client_fd, resp);
  }

  long enTime = getTimeInMicroseconds();
  long duration = enTime - stTime;

  std::ostringstream ostream;
  ostream << "echo '" << duration << "' >> logs/responseTime.txt";
  system(ostream.str().c_str());

  close(client_fd);
  std::cout << "================================================\n";
  pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
  signal(SIGSERV, serverStateSignalHandler);

  check_error(argc == 2, "Usage: ./server <port>");
  short portno = atoi(argv[1]);

  ServerState* serverState = new ServerState();

  serverStateHandler = [&](int sig) -> void {
    std::cout << "Received signal: " << sig << std::endl;

    if (!serverState->startedRecording) {
      std::string numClientsStr = readFileFromPath("./numClients.txt");
      serverState->numClients = atoi(numClientsStr.c_str());
      serverState->serverStartTime = getTimeInMicroseconds();
      serverState->startedRecording = true;
    } else {
      serverState->serverEndTime = getTimeInMicroseconds();
      time_t duration =
          (serverState->serverEndTime - serverState->serverStartTime);
      float throughput = ((double)serverState->numSuccessfulReqResp * 1000000) /
                         (double)duration;

      std::ostringstream ostream;
      ostream << "echo '" << serverState->numClients << "," << throughput
              << "' >> serverrun.log";
      system(ostream.str().c_str());
      serverState->reset();
    }
    std::cout << "Processed signal: " << sig << std::endl;
  };

  char cwd[BUFF_SIZE];
  getcwd(cwd, BUFF_SIZE);
  std::cout << "WORKING DIR: " << cwd << std::endl;

  app = new Application(
      new autograder::Grader(GRADER_ROOT_DIR, IDLE_OUT_FILE_PATH));

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

    pthread_t worker;
    pthread_create(&worker, NULL, &handler, (void*)&client_fd);
  }
}