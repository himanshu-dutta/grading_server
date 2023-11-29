#pragma once
#include <string>
#include <utility>

#include "types.h"

namespace autograder {
int setupTCPSocket(std::string ip, short port, int backlog,
                   bool setReuseAddr = true);
std::pair<bytes, bool> readFromSocket(int, int);
bool writeToSocket(int, bytes);
}  // namespace autograder