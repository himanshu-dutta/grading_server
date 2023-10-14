#pragma once
#include <stdlib.h>
#include <sys/time.h>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>

void check_error(bool cond, std::string msg, bool exitOnError = true);
std::pair<std::string, std::string> splitHostPort(std::string hp);
std::string generateRandomString(int len);
std::string readFileFromPath(std::string);
int randInt(int mn, int mx);
std::time_t getTimeInMicroseconds();