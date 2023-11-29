#pragma once
#include <string>
#include <unordered_map>
#include <utility>

#include "logger.h"
#include "protocol.h"

namespace autograder {

// ClientStatus
enum class ClientStatus { INQUEUE, COMPILED, RAN, DIFFED, DONE };
class ClientInfo {
 public:
  std::string token;
  std::string reqBody;
  ClientStatus status;
  std::string gradingLog;
  std::string statusStr;

  ClientInfo(std::string tok, std::string rb)
      : token(tok),
        reqBody(rb),
        status(ClientStatus::INQUEUE),
        statusStr("request in queue"){};
};

class Grader {
 public:
  Grader(std::string rootDir, std::string idleOutFilePath);
  Response* operator()(Request* req);

 public:
  // grades, returns if the submission passed, and logs for the  grading
  std::pair<bool, std::string> grade(std::string fileContents,
                                     std::string token);

  // saves file to the disk and returns filename, and path with that filename as
  // string
  std::pair<std::string, std::string> saveFileToDisk(std::string dest,
                                                     std::string fileContents,
                                                     std::string ext,
                                                     std::string fileName = "");

  // runs command and saves it's logs in the provided paths. Returns the return
  // code of the command, along with the saved logs
  std::pair<int, std::pair<std::string, std::string>> runCmd(
      std::string cmd, std::string outLogPath = "",
      std::string errLogPath = "");

 private:
  std::string rootDir;
  std::string idleOutFilePath;
  std::unordered_map<std::string, ClientInfo*>* clientInfo;
};

// returns filename with extension
std::string fileWithExt(std::string fname, std::string ext);
}  // namespace autograder