#include "tcpsocket.h"

#include <unistd.h>

#include "config.h"
#include "utils.h"

namespace autograder {
int setupTCPSocket(std::string ip, short port, int backlog, bool setReuseAddr) {
  int listenerFd;
  sockaddr_in serverAddr = getSockaddrIn(ip, port);
  check_error((listenerFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0,
              "tcpsocket::setupTCPSocket - error at socket creation");

  if (setReuseAddr) {
    const int32_t enable = 1;
    check_error((setsockopt(listenerFd, SOL_SOCKET, SO_REUSEADDR, &enable,
                            sizeof(int))) >= 0,
                "tcpsocket::setupTCPSocket - setsockopt reuseaddr error");
  }

  check_error(
      (bind(listenerFd, (sockaddr*)&serverAddr, sizeof(serverAddr))) >= 0,
      "tcpsocket::setupTCPSocket - bind error");
  check_error((listen(listenerFd, backlog)) >= 0,
              "tcpsocket::setupTCPSocket - listen error");

  return listenerFd;
}

std::pair<bytes, bool> readFromSocket(int fd, int num_bytes) {
  bytes data;
  byte buffer[BUFF_SIZE];
  int n = 0;

  if (num_bytes > 0 &&
      (n = read(fd, buffer, std::min(BUFF_SIZE, num_bytes))) > 0) {
    data.insert(data.end(), buffer, buffer + n);
    num_bytes -= n;
  }
  return {data, n >= 0};
}
bool writeToSocket(int fd, bytes data) {
  return (write(fd, data.data(), data.size()) == data.size());
}
}  // namespace autograder