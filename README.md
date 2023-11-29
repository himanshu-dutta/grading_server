# Regular Client-Server operation

To start server: `./build/server <port>`

Example:

```sh
$ ./build/server 5005
```

To start client: `./client <serverIP:port> <sourceCodeFileTobeGraded>`

Example:

```sh
$ ./build/client 0.0.0.0:5005 source.c
```

# Performance Testing

Run the server, and note down the pid of the server process.
Use this pid and run

```sh
$ ./build/signalListener <signalListenerPort> <serverPid>
```

Then run the `wrapperloadtest.sh` file after configuring the `signalListenerPort` in the same, configuring the source file path as well. You may have to change the server port in `loadtest.sh` as well.

The client side `throughput.csv` and `responsetime.csv` would be stored in `./build/` subdirectory, and `serverrun.log` in the current directory itself.
