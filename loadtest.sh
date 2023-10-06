#! /bin/bash

#getting the arguments
numClients=$1
loopNum=$2
sleepTimeSeconds=$3

#running number of clients
for i in $(seq 1 $numClients)
do
    ./build/loadtestclient 0.0.0.0:5005 build/source.c $loopNum $sleepTimeSeconds > "/build/client${i}.txt" &
    pids[${i}]=$!
done

#waiting for all the client to done all request and get corresponding responses
for pid in ${pids[*]}
do
    wait $pid
done

#giving path to the file going to be created
tpfile="/build/throughput.csv"
rtfile="/build/response.csv"

#adding header initially file will not be there
if [! -e $tpfile] 
then
    echo "M,Overall_throughput" > /build/throughput.csv
fi

if [! -e $tpfile] 
then
    echo "M,Avg_Response_time" > /build/responsetime.csv
fi

#read individual throughput, add them and calculate partial response time
Overall_throughput=0
pResponsetime=0
for file in /build/client*.c
do
    ART=grep "ART" $file | cut -d ',' -f 1 | cut -d ': ' -f 2
    NSR=grep "ART" $file | cut -d ',' -f 2 | cut -d ': ' -f 2
    LT=grep "ART" $file | cut -d ',' -f 3 | cut -d ': ' -f 2
    throughput_i=${NSR}/${LT}

    let Overall_throughput=$(($Overall_throughput + $throughput_i))
    temp=(($ART * $loopNum))
    let pResponsetime=(($pResponsetime + $temp))
done
#calculate final response time
total_req=$(($numClients * $loopNum))
Avg_Response_time=$(($pResponsetime / $total_req))

#sending rows to respective files
echo "$M,$Overall_throughput" > /build/throughput.csv
echo "$M,$Avg_Response_time" > /build/responsetime.csv

