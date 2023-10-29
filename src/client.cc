#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "config.h"
#include "protocol.h"
#include "tcpsocket.h"
#include "types.h"
#include "utils.h"

pthread_t resp_th;
pthread_mutex_t resp_mu;
pthread_cond_t resp_cond;

void *GetResponseTimeOutCheck(void *connFd_ptr)
{
  pthread_mutex_lock(&resp_mu);
  int connFd = *(int *)connFd_ptr;
  autograder::Response *resp = autograder::ClientProtocol::parseResponse(connFd);
  pthread_cond_signal(&resp_cond);
  pthread_mutex_unlock(&resp_mu);
  pthread_exit((void *)resp);
}

int main(int argc, char *argv[])
{
  check_error(argc == 4,
              "Usage: ./client <serverIP:port> <sourceCodeFileTobeGraded> <time-out>");
  auto [host, port] = splitHostPort(argv[1]);
  std::string evaluationFilePath = argv[2];
  short portno = atoi(port.data());
  float timeOutVal = atof(argv[3]);
  struct timeval daytime;
  std::string data = readFileFromPath(evaluationFilePath);
  int numTry = 0;
  if (data != "")
  {

    while (numTry != 3)
    {
      int connFd;

      if ((connFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
        numTry++;
        continue;
      }

      sockaddr_in serverAddr = getSockaddrIn(host, portno);
      if (connect(connFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
      {
        numTry++;
        close(connFd);
        continue;
      }

      autograder::ClientProtocol::sendRequest(
          connFd, "CHECK",
          {
              {"Content-Length", std::to_string(data.length())},
          },
          data);

      std::cout << "Sent grading request to the server...\n";
      struct timespec time_out;
      if (timeOutVal < 1)
      {
        time_out.tv_nsec = timeOutVal * 10e8;
        time_out.tv_sec = 0;
      }
      else
      {
        time_out.tv_nsec = (timeOutVal - (int)timeOutVal) * 10e8;
        time_out.tv_sec = (int)timeOutVal;
      }
      gettimeofday(&daytime, NULL);
      time_out.tv_sec += daytime.tv_sec + (int)(time_out.tv_nsec + daytime.tv_usec * 1000) / 1e9;
      time_out.tv_nsec = (time_out.tv_nsec + daytime.tv_usec * 1000) % (int)1e9;
      void *resp = NULL;
      pthread_cond_init(&resp_cond, NULL);
      pthread_mutex_init(&resp_mu, NULL);

      while (numTry != 3)
      {
        pthread_mutex_lock(&resp_mu);
        int tc = pthread_create(&resp_th, NULL, &GetResponseTimeOutCheck, &connFd);
        if (tc)
        {
          numTry++;
          continue;
        }
        else
        {
          int n = pthread_cond_timedwait(&resp_cond, &resp_mu, &time_out);
          pthread_mutex_unlock(&resp_mu);
          std::cout << strerror(n) << std::endl;
          if (n == 0)
          {
            int err_no = pthread_join(resp_th, &resp);
            if (err_no != 0)
            {
              numTry++;
              continue;
            }
            else
            {
              std::cout << "Response from the server:\n"
                        << ((autograder::Response *)resp)->body << std::endl;
              return (((autograder::Response *)resp)->resp_type != "ERROR");
            }
            close(connFd);
          }
          else
          {
            pthread_kill(resp_th, 0);
            close(connFd);
            // timeout occured
            exit(2);
          }
        }
        break;
      }
      close(connFd);
    }
    if (numTry == 3)
    {
      // exit incase of error connecting
      exit(1);
    }
    else
    {
      // exit for successful response
      exit(0);
    }
  }
  else
  {
    std::cout << "Either empty file or file doesn't exists\n";
  }
}