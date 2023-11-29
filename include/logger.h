#pragma once
#include <pthread.h>

#include <fstream>
#include <string>

namespace autograder {

// thread-safe file logger, to write application logs to some file
class FileLogger {
 public:
  enum class LoggerLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

  FileLogger(std::string filePath, LoggerLevel level = LoggerLevel::INFO);
  void trace(std::string log);
  void debug(std::string log);
  void info(std::string log);
  void warn(std::string log);
  void error(std::string log);
  void fatal(std::string log);

 private:
  void writeLog(std::string log, LoggerLevel level);

 private:
  pthread_mutex_t mu;
  LoggerLevel logLevel;
  std::string filePath;
  std::ofstream *file;
};
}  // namespace autograder