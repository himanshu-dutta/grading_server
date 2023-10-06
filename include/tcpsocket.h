#pragma once
#include <utility>

#include "types.h"

namespace autograder {
std::pair<bytes, bool> readFromSocket(int, int);
bool writeToSocket(int, bytes);
}  // namespace autograder