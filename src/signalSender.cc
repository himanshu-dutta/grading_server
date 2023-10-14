#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include "utils.h"

#define BACKLOG 8

int main(int argc, char* argv[]) {
  check_error(argc == 3, "Usage: ./server <serverIP:port> <numClients>");
  auto [host, port] = splitHostPort(argv[1]);
  int numClients = atoi(argv[2]);
  short portno = atoi(port.data());

  int conn_fd;
  sockaddr_in server_addr;
  check_error((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "error at socket creation");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portno);
  server_addr.sin_addr.s_addr = inet_addr(host.data());

  check_error(connect(conn_fd, (struct sockaddr*)&server_addr,
                      sizeof(server_addr)) >= 0,
              "error at socket connect");
  write(conn_fd, &numClients, sizeof(numClients));
  close(conn_fd);
}