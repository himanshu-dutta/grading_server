# Regular Client-Server operation

To start server: `./build/server <port> <threadpoolSize> <logFilePath>`

Example:

```
$ ./build/server 5005 32 logs/serv.log
```

To make client request: `./build/client <serverIP:port> <sourceCodeFileTobeGraded> <timeout>`

Example:

```
$ ./build/client 0.0.0.0:5005 samples/compile_error.c 2
```

# Performance Testing

Run: `./build/perfserver <port> <logFilePath> <serverBinPath> <serverPort> <serverThreadpoolSize>` to start the performance testing server.

Run: `./build/perfclient <perfserverIP:port> <numClients> <numRequests> <logFilePath> <clientBinPath> <serverIP:port> <sourceCodeFileToBeGraded> <clientTimeout>` to start the performance testing client for `numClients` clients.

Example:

```
$ ./build/perfserver 8000 logs/server_perf.log build/server 5005 8
```

```
$ ./build/perfclient 0.0.0.0:8000 32 5 logs/client_perf.log build/client 0.0.0.0:5005 samples/correct.c 2
```
