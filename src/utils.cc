#include "utils.h"

#include <errno.h>
#include <string.h>

void check_error(bool cond, std::string msg, bool exitOnError) {
  if (!cond) {
    std::cout << msg << ": " << strerror(errno) << std::endl;
    if (exitOnError) exit(1);
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

int randInt(int mn, int mx) {
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> uni(mn, mx);
  auto random_integer = uni(rng);
  return random_integer;
}

long getTimeInMicroseconds() {
  timeval timestamp;
  gettimeofday(&timestamp, NULL);
  long tm = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
  return tm;
}

sockaddr_in getSockaddrIn(std::string ip, short port) {
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.data());
  return addr;
}

void logCWD() {
  const int BUFF_SIZE = 1024;
  char* cwd = new char[BUFF_SIZE];
  getcwd(cwd, BUFF_SIZE);
  std::cout << "WORKING DIR: " << cwd << std::endl;
  delete[] cwd;
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string generate_uuid_v4() {
  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4";
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  ss << dis2(gen);
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 12; i++) {
    ss << dis(gen);
  };
  return ss.str();
}