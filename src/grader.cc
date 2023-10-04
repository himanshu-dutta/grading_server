#include "grader.h"

#include <stdio.h>

#include <fstream>
#include <sstream>

#include "config.h"
#include "protocol.h"
#include "types.h"
#include "utils.h"

namespace autograder {

Grader::Grader(std::string rootDir, std::string idleOutFilePath)
    : rootDir(rootDir), idleOutFilePath(idleOutFilePath){};

Response* Grader::operator()(Request* req) {
  Response* resp;
  if (req->req_type == "CHECK") {
    auto [gradingRes, gradingLog] = grade(req->body);
    std::cout << "===Got Grader Response===\n";
    resp = ServerProtocol::generateResponse(
        req->req_type,
        {
            {"content-length", std::to_string(gradingLog.length())},
        },
        gradingLog);
  }

  return resp;
}

std::pair<bool, std::string> Grader::grade(std::string fileContents) {
  std::ostringstream ostream;

  auto resetOStream = [&]() {
    ostream.str("");
    ostream.clear();
  };

  auto generateExecutablePath = [&](std::string fileName) {
    ostream << rootDir << "/" << fileName;
    std::string executablePath = ostream.str();
    resetOStream();
    return executablePath;
  };

  auto [fileName, filePath] = saveFileToDisk(rootDir, fileContents, "c");
  auto compileErrLogFPath = fileWithExt(filePath, "c.log");
  auto runOutLogFPath = fileWithExt(filePath, "o.log");
  auto runErrLogFPath = fileWithExt(filePath, "e.log");
  auto diffOutLogFPath = fileWithExt(filePath, "d.log");
  auto executablePath = generateExecutablePath(fileName);

  auto cleanupRoutine = [&]() {
    std::remove(compileErrLogFPath.c_str());
    std::remove(runOutLogFPath.c_str());
    std::remove(runErrLogFPath.c_str());
    std::remove(diffOutLogFPath.c_str());
    std::remove(diffOutLogFPath.c_str());
    std::remove(executablePath.c_str());
  };

  // compilation
  ostream << "gcc -o " << rootDir << "/" << fileName << " " << filePath;
  auto [compileRetCode, compileLogs] =
      runCmd(ostream.str(), "", compileErrLogFPath);
  auto [compileOutLog, compileErrLog] = compileLogs;
  resetOStream();
  if (compileRetCode != 0) {
    ostream << "COMPILER ERROR: ";
    ostream << compileErrLog;
    cleanupRoutine();
    return {false, ostream.str()};
  }
  std::cout << "===COMPILED===\n";

  // running
  ostream << "./" << executablePath;
  auto [runRetCode, runLogs] =
      runCmd(ostream.str(), runOutLogFPath, runErrLogFPath);
  auto [runOutLog, runErrLog] = runLogs;
  resetOStream();
  if (runRetCode != 0) {
    ostream << "RUNTIME ERROR: ";
    ostream << runErrLog;
    cleanupRoutine();
    return {false, ostream.str()};
  }
  std::cout << "===RAN===\n";

  // diffing
  ostream << "diff " << runOutLogFPath << " " << idleOutFilePath;
  auto [diffRetCode, diffLogs] = runCmd(ostream.str(), diffOutLogFPath, "");
  auto [diffOutLog, diffErrLog] = diffLogs;
  resetOStream();
  if (diffRetCode != 0) {
    ostream << "OUTPUT ERROR: ";
    ostream << diffOutLog << diffErrLog;
    cleanupRoutine();
    return {false, ostream.str()};
  }
  std::cout << "===PASSED===\n";

  ostream << "PASS";

  cleanupRoutine();
  return {true, ostream.str()};
}

std::pair<std::string, std::string> Grader::saveFileToDisk(
    std::string dir, std::string fileContents, std::string ext) {
  std::ostringstream ostream;
  std::string fileName = generateRandomString(FILENAME_LEN);
  ostream << dir << "/" << fileName << "." << ext;
  std::string filePath = ostream.str();
  ostream.str("");
  ostream.clear();

  std::ofstream fl;
  fl.open(filePath);
  fl << fileContents;
  fl.close();

  return {fileName, filePath};
}

std::pair<int, std::pair<std::string, std::string>> Grader::runCmd(
    std::string cmd, std::string outLogPath, std::string errLogPath) {
  std::ostringstream ostream;

  ostream << "sh -c \"{ ";

  ostream << cmd;
  if (outLogPath != "") ostream << " >" << outLogPath;
  ostream << "; }";
  if (errLogPath != "") ostream << " 2>" << errLogPath;
  ostream << "\"";

  std::cout << "Running: " << ostream.str() << std::endl;

  int retCode = system(ostream.str().c_str());

  std::string outLog;
  std::string errLog;
  if (outLogPath != "") outLog = readFileFromPath(outLogPath);
  if (errLogPath != "") errLog = readFileFromPath(errLogPath);

  return {retCode, {outLog, errLog}};
}

std::string fileWithExt(std::string fname, std::string ext) {
  std::ostringstream ostream;
  ostream << fname << "." << ext;
  return ostream.str();
}

}  // namespace autograder