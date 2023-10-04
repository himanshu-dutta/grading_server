#include "utils.h"

#include <errno.h>
#include <string.h>

void check_error(bool cond, std::string msg) {
  if (!cond) {
    std::cout << msg << ": " << strerror(errno) << std::endl;
    exit(1);
  }
}

std::pair<std::string, std::string> splitHostPort(std::string hp) {
  int delPos = hp.find(":");
  std::string host = hp.substr(0, delPos);
  std::string port = hp.substr(delPos + 1);
  return {host, port};
}

std::string generateRandomString(int len) {
  static const char alphanum[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";

  std::string randStr;
  randStr.reserve(len);

  for (int i = 0; i < len; ++i) {
    randStr += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return randStr;
}

std::string readFileFromPath(std::string filePath) {
  std::ifstream f(filePath);
  std::string data;
  if (f.is_open()) {
    std::ostringstream ostream;
    ostream << f.rdbuf();
    data = ostream.str();
  }
  return data;
}
