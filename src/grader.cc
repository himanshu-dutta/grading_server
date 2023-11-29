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
    : rootDir(rootDir), idleOutFilePath(idleOutFilePath) {
  clientInfo = new std::unordered_map<std::string, ClientInfo*>();
};

Response* Grader::operator()(Request* req) {
  Response* resp;
  if (req->req_type == "CHECK") {
    ClientInfo* ci = new ClientInfo(generate_uuid_v4(), req->body);
    clientInfo->insert_or_assign(ci->token, ci);
    std::string respText = ci->token + ": " + ci->statusStr;

    auto [fileName, filePath] =
        saveFileToDisk(rootDir, req->body, "c", ci->token);

    resp = ServerProtocol::generateResponse(
        req->req_type,
        {
            {"Content-Length", std::to_string(respText.length())},
        },
        respText);

  } else if (req->req_type == "STATUS") {
    std::string token = req->body;
    std::unordered_map<std::string, ClientInfo*>::const_iterator info =
        clientInfo->find(token);
    if (info == clientInfo->end()) {
      std::string respBody =
          token + ": ERROR :" + "no pending request for this token";
      resp = ServerProtocol::generateResponse(
          req->req_type,
          {
              {"Content-Length", std::to_string(respBody.length())},
          },
          respBody);
    } else {
      ClientInfo* ci = info->second;
      if (ci->status == ClientStatus::DONE) {
        // REQ DONE
        std::string respBody = token + ": DONE :" + ci->gradingLog;
        resp = ServerProtocol::generateResponse(
            req->req_type,
            {
                {"Content-Length", std::to_string(respBody.length())},
            },
            respBody);

        // DELETE INFO & LOG
        clientInfo->erase(ci->token);
      } else {
        // REQ IN PROGRESS
        std::string respBody = token + ": " + ci->statusStr;
        resp = ServerProtocol::generateResponse(
            req->req_type,
            {
                {"Content-Length", std::to_string(respBody.length())},
            },
            respBody);
      }
    }
  }

  return resp;
}

std::pair<bool, std::string> Grader::grade(std::string fileContents,
                                           std::string token) {
  std::ostringstream ostream;
  ClientInfo* ci = (*clientInfo)[token];

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

  auto [fileName, filePath] = saveFileToDisk(rootDir, fileContents, "c", token);
  auto sourceFilePath = filePath;
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
    std::remove(sourceFilePath.c_str());
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
    ci->status = ClientStatus::DONE;
    ci->gradingLog = ostream.str();
    return {false, ostream.str()};
  }
  std::cout << "===COMPILED===\n";
  ci->status = ClientStatus::COMPILED;
  ci->statusStr = "compiled successfully";

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
    ci->status = ClientStatus::DONE;
    ci->gradingLog = ostream.str();
    return {false, ostream.str()};
  }
  std::cout << "===RAN===\n";
  ci->status = ClientStatus::RAN;
  ci->statusStr = "ran successfully";

  // diffing
  ostream << "diff " << runOutLogFPath << " " << idleOutFilePath;
  auto [diffRetCode, diffLogs] = runCmd(ostream.str(), diffOutLogFPath, "");
  auto [diffOutLog, diffErrLog] = diffLogs;
  resetOStream();
  if (diffRetCode != 0) {
    ostream << "OUTPUT ERROR: ";
    ostream << diffOutLog << diffErrLog;
    cleanupRoutine();
    ci->status = ClientStatus::DONE;
    ci->gradingLog = ostream.str();
    return {false, ostream.str()};
  }
  std::cout << "===PASSED===\n";
  ci->status = ClientStatus::DIFFED;
  ci->statusStr = "diffed successfully";

  ostream << "PASS";

  cleanupRoutine();
  ci->status = ClientStatus::DONE;
  ci->gradingLog = ostream.str();
  return {true, ostream.str()};
}

std::pair<std::string, std::string> Grader::saveFileToDisk(
    std::string dir, std::string fileContents, std::string ext,
    std::string fileName) {
  std::ostringstream ostream;
  fileName = fileName != "" ? fileName : generateRandomString(FILENAME_LEN);
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