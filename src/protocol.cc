#include "protocol.h"

#include <unistd.h>

#include "config.h"
#include "tcpsocket.h"
#include "types.h"
#include "utils.h"

namespace autograder {

Request* ServerProtocol::parseRequest(int fd) {
  bytes data;
  // looping for request line
  auto isRequestLineEnd = [&data](int32_t requestLinePtr) -> bool {
    return (data[requestLinePtr] == '\r' && data[requestLinePtr + 1] == '\n');
  };
  int32_t requestLinePtr = 0;
  while (true) {
    while (requestLinePtr < ((int32_t)data.size() - 1) &&
           !isRequestLineEnd(requestLinePtr))
      requestLinePtr++;
    if (requestLinePtr < ((int32_t)data.size() - 1) &&
        isRequestLineEnd(requestLinePtr))
      break;

    bytes buff = readFromSocket(fd, BUFF_SIZE);
    data.insert(data.end(), buff.begin(), buff.end());
  }
  // looping for headers
  auto isHeadersEnd = [&data](int32_t headerPtr) -> bool {
    return ((data[headerPtr] == '\r') && (data[headerPtr + 1] == '\n') &&
            (data[headerPtr + 2] == '\r') && (data[headerPtr + 3] == '\n'));
  };

  int32_t headersPtr = requestLinePtr + 2;
  while (true) {
    while (headersPtr < ((int32_t)data.size() - 3) && !isHeadersEnd(headersPtr))
      headersPtr++;
    if (headersPtr < ((int32_t)data.size() - 3) && isHeadersEnd(headersPtr))
      break;

    bytes buff = readFromSocket(fd, BUFF_SIZE);
    data.insert(data.end(), buff.begin(), buff.end());
  }

  bytes requestLine(data.begin(), data.begin() + requestLinePtr);
  bytes headers(data.begin() + requestLinePtr + 2,
                data.begin() + headersPtr + 2);
  bytes body(data.begin() + headersPtr + 4, data.end());
  std::vector<std::pair<std::string, std::string>> headersList =
      parseHeaders(headers);
  uint32_t bodySize = -1;
  for (auto header : headersList) {
    if (header.first == "content-length") bodySize = atoi(header.second.data());
  }

  check_error(bodySize >= 0, "content-length not provided in request msg");
  bodySize -= body.size();

  bytes buff = readFromSocket(fd, bodySize);
  body.insert(body.end(), buff.begin(), buff.end());

  Request* req = new Request(parseRequestLine(requestLine), headersList,
                             std::string(body.begin(), body.end()));

  return req;
}

Response* ServerProtocol::generateResponse(
    std::string resp_type,
    std::vector<std::pair<std::string, std::string>> headers,
    std::string body) {
  Response* resp = new Response(resp_type, headers, body);
  return resp;
}

bool ServerProtocol::sendResponse(int fd, Response* resp) {
  bytes buffer;
  buffer.insert(buffer.end(), resp->resp_type.begin(), resp->resp_type.end());
  buffer.push_back('\r');
  buffer.push_back('\n');

  std::string headers = stringifyHeaders(resp->headers);
  buffer.insert(buffer.end(), headers.begin(), headers.end());
  buffer.push_back('\r');
  buffer.push_back('\n');

  buffer.insert(buffer.end(), resp->body.begin(), resp->body.end());

  return writeToSocket(fd, buffer);
}

bool ClientProtocol::sendRequest(
    int fd, std::string req_type,
    std::vector<std::pair<std::string, std::string>> headers,
    std::string body) {
  bytes buffer;
  buffer.insert(buffer.end(), req_type.begin(), req_type.end());
  buffer.push_back('\r');
  buffer.push_back('\n');

  std::string headersStr = stringifyHeaders(headers);
  buffer.insert(buffer.end(), headersStr.begin(), headersStr.end());
  buffer.push_back('\r');
  buffer.push_back('\n');

  buffer.insert(buffer.end(), body.begin(), body.end());
  return writeToSocket(fd, buffer);
}

Response* ClientProtocol::parseResponse(int fd) {
  bytes data;
  // looping for request line
  auto isResponseLineEnd = [&data](int32_t responseLinePtr) -> bool {
    return (data[responseLinePtr] == '\r' && data[responseLinePtr + 1] == '\n');
  };
  int32_t responseLinePtr = 0;
  while (true) {
    while (responseLinePtr < ((int32_t)data.size() - 1) &&
           !isResponseLineEnd(responseLinePtr))
      responseLinePtr++;
    if (responseLinePtr < ((int32_t)data.size() - 1) &&
        isResponseLineEnd(responseLinePtr))
      break;

    bytes buff = readFromSocket(fd, BUFF_SIZE);
    data.insert(data.end(), buff.begin(), buff.end());
  }
  // looping for headers
  auto isHeadersEnd = [&data](int32_t headerPtr) -> bool {
    return ((data[headerPtr] == '\r') && (data[headerPtr + 1] == '\n') &&
            (data[headerPtr + 2] == '\r') && (data[headerPtr + 3] == '\n'));
  };

  int32_t headersPtr = responseLinePtr + 2;
  while (true) {
    while (headersPtr < ((int32_t)data.size() - 3) && !isHeadersEnd(headersPtr))
      headersPtr++;
    if (headersPtr < ((int32_t)data.size() - 3) && isHeadersEnd(headersPtr))
      break;

    bytes buff = readFromSocket(fd, BUFF_SIZE);
    data.insert(data.end(), buff.begin(), buff.end());
  }

  bytes responseLine(data.begin(), data.begin() + responseLinePtr);
  bytes headers(data.begin() + responseLinePtr + 2,
                data.begin() + headersPtr + 2);
  bytes body(data.begin() + headersPtr + 4, data.end());
  std::vector<std::pair<std::string, std::string>> headersList =
      parseHeaders(headers);
  uint32_t bodySize = -1;
  for (auto header : headersList) {
    if (header.first == "content-length") bodySize = atoi(header.second.data());
  }

  check_error(bodySize >= 0, "content-length not provided in response msg");
  bodySize -= body.size();

  bytes buff = readFromSocket(fd, bodySize);
  body.insert(body.end(), buff.begin(), buff.end());

  Response* resp = new Response(parseResponseLine(responseLine), headersList,
                                std::string(body.begin(), body.end()));

  return resp;
}

std::string parseRequestLine(bytes req_line) {
  return std::string(req_line.begin(), req_line.end());
}
std::string parseResponseLine(bytes req_line) {
  return std::string(req_line.begin(), req_line.end());
}

std::vector<std::pair<std::string, std::string>> parseHeaders(bytes headers) {
  std::vector<std::pair<std::string, std::string>> headersList;
  auto parseSingleHeader =
      [&headers](int32_t st,
                 int32_t en) -> std::pair<std::string, std::string> {
    int32_t idx = st;
    while (idx < en - 1 && !(headers[idx] == ':' && headers[idx + 1] == ' '))
      ++idx;
    std::string k(headers.begin() + st, headers.begin() + idx),
        v(headers.begin() + idx + 2, headers.begin() + en);

    return {k, v};
  };

  int32_t st = 0, en = 0;
  while (en < (int32_t)headers.size() - 1) {
    if (headers[en] == '\r' && headers[en + 1] == '\n') {
      headersList.push_back(parseSingleHeader(st, en));
      st = en = en + 2;

    } else {
      ++en;
    }
  }

  return headersList;
}

std::string stringifyHeaders(
    std::vector<std::pair<std::string, std::string>> headersList) {
  std::string headers;
  for (auto header : headersList) {
    headers += header.first;
    headers += ": ";
    headers += header.second;
    headers += "\r\n";
  }
  return headers;
}

}  // namespace autograder