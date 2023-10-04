#pragma once
#include <string>
#include <utility>
#include <vector>

#include "types.h"

/*
Here we define the format and actions of the messages that flow between the
server and the client.

Request Format:
================
REQ_TYPE\r\n
HEADER1\r\n
HEADER2\r\n
...
\r\n
REQ_BODY
\r\n
================

Respone Format:
================
RESP_TYPE\r\n
HEADER1\r\n
HEADER2\r\n
...
\r\n
RESP_BODY
\r\n
================


The only mandatory header for both request and response is Content-Length
REQ_TYPE: GET_GRADE

*/
namespace autograder {

class Request {
 public:
  Request(std::string rt, std::vector<std::pair<std::string, std::string>> hs,
          std::string b)
      : req_type(rt), headers(hs), body(b) {}

 public:
  std::string req_type;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

class Response {
 public:
  Response(std::string rt, std::vector<std::pair<std::string, std::string>> hs,
           std::string b)
      : resp_type(rt), headers(hs), body(b) {}

 public:
  std::string resp_type;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

class ServerProtocol {
 public:
  static Request* parseRequest(int);
  static Response* generateResponse(
      std::string resp_type,
      std::vector<std::pair<std::string, std::string>> headers,
      std::string body);
  static bool sendResponse(int, Response*);
};

class ClientProtocol {
 public:
  static bool sendRequest(
      int, std::string req_type,
      std::vector<std::pair<std::string, std::string>> headers,
      std::string body);
  static Response* parseResponse(int);

 private:
};
std::string parseRequestLine(bytes);
std::string parseResponseLine(bytes);
std::vector<std::pair<std::string, std::string>> parseHeaders(bytes);
std::string stringifyHeaders(
    std::vector<std::pair<std::string, std::string>> headersList);
}  // namespace autograder