#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "utils.h"

int main(int argc, char* argv[]) {
  // pass the process id as argument
  int portno = atoi(argv[1]);
  int serverProcId = atoi(argv[2]);

  int listener_fd;
  sockaddr_in server_addr, client_addr;
  check_error((listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "error at socket creation");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portno);
  server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

  check_error(
      (bind(listener_fd, (sockaddr*)&server_addr, sizeof(server_addr))) >= 0,
      "bind error");
  check_error((listen(listener_fd, 1)) >= 0, "listen error");

  while (1) {
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(listener_fd, (sockaddr*)&client_addr, &client_addr_len);
    check_error(client_fd >= 0, "accept error");

    int numClients;
    int nb = read(client_fd, &numClients, sizeof(numClients));
    std::ostringstream ostream;
    ostream << "echo '" << numClients << " tp' > numClients.txt";
    system(ostream.str().c_str());

    int retVal = kill(serverProcId, 10);
    std::cout << "Sent kill signal and got retVal: " << retVal << std::endl;
    close(client_fd);
  }

  close(listener_fd);
}
