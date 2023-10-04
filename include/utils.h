#pragma once
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

void check_error(bool cond, std::string msg);
std::pair<std::string, std::string> splitHostPort(std::string hp);
std::string generateRandomString(int len);
std::string readFileFromPath(std::string);