#pragma once
#include "types.h"

namespace autograder {
bytes readFromSocket(int, int);
bool writeToSocket(int, bytes);
}  // namespace autograder