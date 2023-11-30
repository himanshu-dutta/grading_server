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

// #############################################################################
// SHUTDOWN SIGNAL HANDLER
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

// #############################################################################

// #############################################################################
// SERVER STATE
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

ServerState* serverState;
// #############################################################################

// #############################################################################
// THREAD CLEANUP
std::vector<pthread_t> cleanupThreadIds;
pthread_mutex_t mu;
void* threadCleaner(void* arg) {
  sigset_t set;
  sigemptyset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (true) {
    pthread_mutex_lock(&mu);
    int n = 0;
    for (auto thId : cleanupThreadIds) {
      pthread_join(thId, NULL);
      n++;
    }
    std::cerr << "\033[96m"
              << "Cleaned " << n << " threads\n"
              << "\033[0m";
    cleanupThreadIds.clear();
    pthread_mutex_unlock(&mu);
    sleep(2);
  }
}

// #############################################################################

// #############################################################################
// APPLICATION
class Application {
 public:
  autograder::Grader* grader;
  Application(autograder::Grader* grader) : grader(grader) {}
};

Application* app;
pthread_mutex_t appMu;

void* handler(void* client_fd_ptr) {
  sigset_t set;
  sigemptyset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  int client_fd = *(int*)client_fd_ptr;
  std::cout << "================================================\n";
  std::cout << "Connected to a client...\n";

  while (true) {
    autograder::Request* req =
        autograder::ServerProtocol::parseRequest(client_fd);
    if (req->req_type == "ERROR") break;
    pthread_mutex_lock(&appMu);
    autograder::Response* resp = (*app->grader)(req);
    pthread_mutex_unlock(&appMu);
    bool sentSuccessfully =
        autograder::ServerProtocol::sendResponse(client_fd, resp);
    if (sentSuccessfully) serverState->numSuccessfulReqResp++;
  }

  close(client_fd);
  std::cout << "================================================\n";

  pthread_mutex_lock(&mu);
  cleanupThreadIds.push_back(pthread_self());
  pthread_mutex_unlock(&mu);

  pthread_exit(NULL);
}
// #############################################################################

// #############################################################################
// MAIN EXECUTION LOOP
int main(int argc, char* argv[]) {
  // registering signal handlers
  signal(SIGINT, shutdownSignalHandler);
  signal(SIGKILL, shutdownSignalHandler);
  signal(SIGSERV, serverStateSignalHandler);

  // parsing the arguments
  check_error(argc == 2, "Usage: ./server <port>");
  short portno = atoi(argv[1]);

  serverState = new ServerState();

  serverStateHandler = [&](int sig) -> void {
    std::cerr << "\033[92m"
              << "Received signal: " << sig << std::endl
              << "\033[0m";
    // differentiating between server started taking load or not!
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
    std::cerr << "\033[92m"
              << "Processed signal: " << sig << std::endl
              << "\033[0m";
  };

  // Logging the working directory
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
  check_error(
    (listen(listener_fd, BACKLOG)) >= 0, 
    "listen error");

  shutdownHandler = [&](int sig) -> void {
    std::cerr << "Shutting the server down, yo!\n";
    close(listener_fd);
  };

  pthread_mutex_init(&mu, NULL);
  pthread_mutex_init(&appMu, NULL);
  pthread_t cleanerThread;
  pthread_create(&cleanerThread, NULL, &threadCleaner, NULL);

  while (true) {
    socklen_t client_addr_len = sizeof(client_addr);
    std::cerr << "\033[94m"
              << "Main Thread: START\n"
              << "\033[0m";
    int client_fd =
        accept(listener_fd, (sockaddr*)&client_addr, &client_addr_len);
    check_error(client_fd >= 0, "accept error");
    std::cerr << "\033[94m"
              << "Main Thread: END\n"
              << "\033[0m";
    pthread_t worker;
    int* client_fd_copy = new int;
    *client_fd_copy = client_fd;
    pthread_create(&worker, NULL, &handler, (void*)client_fd_copy);

    std::cerr << "\033[94m"
              << "Main Thread: AFTER THREAD\n"
              << "\033[0m";
  }
}
// #############################################################################