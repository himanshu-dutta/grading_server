#! /bin/bash

numClients=4

for idx in {1..20}
do
    currClients=$(( numClients*idx ))
    ./build/perfclient 0.0.0.0:8000 $currClients 5 logs/client_perf.log build/client 0.0.0.0:5005 samples/correct.c 3
done