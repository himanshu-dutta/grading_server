#pragma once
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

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
long getTimeInMicroseconds();
sockaddr_in getSockaddrIn(std::string ip, short port);
void logCWD();
std::string
generate_uuid_v4();  // used from: https://stackoverflow.com/a/60198074