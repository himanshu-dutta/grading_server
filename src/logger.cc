#include "logger.h"

#include "utils.h"

namespace autograder {
FileLogger::FileLogger(std::string filePath, FileLogger::LoggerLevel level) {
  this->filePath = filePath;
  this->logLevel = level;
  pthread_mutex_init(&this->mu, NULL);

  pthread_mutex_lock(&this->mu);
  this->file = new std::ofstream(this->filePath);
  pthread_mutex_unlock(&this->mu);
  check_error(this->file->is_open(), "logger error: file coulnd't be opened");
}

void FileLogger::writeLog(std::string log, FileLogger::LoggerLevel level) {
  if (level >= this->logLevel) {
    pthread_mutex_lock(&this->mu);
    (*this->file) << log << std::endl;
    pthread_mutex_unlock(&this->mu);
  }
}

void FileLogger::trace(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::TRACE);
}

void FileLogger::debug(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::DEBUG);
}

void FileLogger::info(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::INFO);
}

void FileLogger::warn(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::WARN);
}

void FileLogger::error(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::ERROR);
}

void FileLogger::fatal(std::string log) {
  return this->writeLog(log, FileLogger::LoggerLevel::FATAL);
}

}  // namespace autograder