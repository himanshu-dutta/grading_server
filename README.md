To start server: ./server \<port\>

Example:

```
$ ./build/server 5005
```

Note: The `BACKLOG` parameter in src/server.c is the bottleneck parameter, on which the results of loadtesting depends. You may tweak that to see different levels of load tolerance.

To make client request: ./client \<serverIP:port\> \<sourceCodeFileTobeGraded\>

Example:

```
$ ./build/client 0.0.0.0:5005 samples/compile_error.c
```

To do loadtesting (client-side results only in this branch): ./wrapperloadtest

Note: The loadtest file needs to be configured before you run ./wrapperloadtest. In line 11, change the \<serverIP:port\> if necessary, and also the \<sourceCodeFileTobeGraded\>, if required.

The results would be available in: `build/throughput.csv`, `build/responsetime.csv` and `build/numDropped.csv`.
