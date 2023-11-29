# Running instructions

Run
```sh
$ ./build/server 5005
```

Run
```sh
$ ps -e | grep "./build/server 5005"
```

and note down the pid of the server process.
Use this pid and run
```sh
$ ./build/signalListener 8000 <pid>
```
This scrpit would allow the server to make recordings for the throughput, by signaling the start and end time for the throughput recording.
