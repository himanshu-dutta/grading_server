# Regular Client-Server operation

To start server: `./build/server <port> <threadpoolSize> <logFilePath>`

Example:

```
$ ./build/server 5005 32 logs/serv.log
```

To make client request: `./client <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID>`

Example:

```
$ ./build/client new 0.0.0.0:5005 samples/compile_error.c

$ ./build/client status 0.0.0.0:5005 67dad33a-e7ef-4abe-a756-e20471cc158a
```

# Performance Testing

Run: `./build/perfserver <port> <logFilePath> <serverBinPath> <serverPort> <serverThreadpoolSize>` to start the performance testing server.

Run: `./build/perfclient <perfserverIP:port> <numClients> <numRequests> <logFilePath> <clientBinPath> <serverIP:port> <sourceCodeFileToBeGraded> <clientPollTime>` to start the performance testing client for `numClients` clients.

Example:

```
$ ./build/perfserver 8000 logs/server_perf.log build/server 5005 8
```

```
$ ./build/perfclient 0.0.0.0:8000 32 5 logs/client_perf.log build/client 0.0.0.0:5005 samples/correct.c 2
```
