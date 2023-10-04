#pragma once
#include <string>
#include <utility>

#include "protocol.h"

namespace autograder {
class Grader {
 public:
  Grader(std::string rootDir, std::string idleOutFilePath);
  Response* operator()(Request* req);

 private:
  // grades, returns if the submission passed, and logs for the  grading
  std::pair<bool, std::string> grade(std::string fileContents);

  // saves file to the disk and returns filename, and path with that filename as
  // string
  std::pair<std::string, std::string> saveFileToDisk(std::string dest,
                                                     std::string fileContents,
                                                     std::string ext);

  // runs command and saves it's logs in the provided paths. Returns the return
  // code of the command, along with the saved logs
  std::pair<int, std::pair<std::string, std::string>> runCmd(
      std::string cmd, std::string outLogPath = "",
      std::string errLogPath = "");

 private:
  std::string rootDir;
  std::string idleOutFilePath;
};

// returns filename with extension
std::string fileWithExt(std::string fname, std::string ext);
}  // namespace autograder