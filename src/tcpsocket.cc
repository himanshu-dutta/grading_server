#include "tcpsocket.h"

#include <unistd.h>

#include "config.h"
#include "utils.h"

namespace autograder {
bytes readFromSocket(int fd, int num_bytes) {
  bytes data;
  byte buffer[BUFF_SIZE];
  int n = 0;

  if (num_bytes > 0 &&
      (n = read(fd, buffer, std::min(BUFF_SIZE, num_bytes))) > 0) {
    data.insert(data.end(), buffer, buffer + n);
    num_bytes -= n;
  }
  check_error(num_bytes == 0 || n >= 0, "error while reading from socket");
  return data;
}
bool writeToSocket(int fd, bytes data) {
  return (write(fd, data.data(), data.size()) == data.size());
}
}  // namespace autograder