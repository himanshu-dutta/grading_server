#! /bin/bash

numClients=$1
loopNum=$2
sleepTimeSeconds=$3

for i in $(seq 1 $numClients)
do
    ./build/loadtestclient 0.0.0.0:5005 build/source.c $loopNum $sleepTimeSeconds > "client${i}.txt" &
    pids[${i}]=$!
done

for pid in ${pids[*]}
do
    wait $pid
done