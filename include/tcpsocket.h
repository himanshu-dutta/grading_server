#pragma once
#include <string>
#include <utility>

#include "types.h"

namespace autograder {
// sets up tcp socket by performing socket, bind and listen calls
int setupTCPSocket(std::string ip, short port, int backlog,
                   bool setReuseAddr = true);
std::pair<bytes, bool> readFromSocket(int, int);
bool writeToSocket(int, bytes);
}  // namespace autograder