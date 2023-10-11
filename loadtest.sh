#! /bin/bash

#getting the arguments
numClients=$1
loopNum=$2
sleepTimeSeconds=$3

#running number of clients
for i in $(seq 1 $numClients)
do
    ./build/loadtestclient 0.0.0.0:5005 source.c $loopNum $sleepTimeSeconds > "./build/client${i}.txt" &
    pids[${i}]=$!
done

#waiting for all the client to done all request and get corresponding responses
for pid in ${pids[*]}
do
    wait $pid
done

#giving path to the file going to be created
tpfile="./build/throughput.csv"
rtfile="./build/responsetime.csv"

#adding header initially file will not be there
if [[ ! -f $tpfile ]] 
then
    echo "M,Overall_throughput(nsr/secs)" > ./build/throughput.csv
fi

if [[ ! -f $rtfile ]] 
then
    echo "M,Avg_Response_time(secs)" > ./build/responsetime.csv
fi

#read individual throughput, add them and calculate partial response time
Overall_throughput=0
pResponsetime=0
for file in ./build/client*.txt
do
    echo "Looking at $file"
    ART=$(grep "ART" $file | cut -d ',' -f 1 | cut -d ':' -f 2)
    NSR=$(grep "ART" $file | cut -d ',' -f 2 | cut -d ':' -f 2)
    LT=$(grep "ART" $file | cut -d ',' -f 3 | cut -d ':' -f 2)

    throughput_i=$(bc <<< "scale=3; ($NSR * 1000000) / $LT")
    Overall_throughput=$(bc <<< "scale=3; ($Overall_throughput + $throughput_i)")
    temp=$(bc <<< "scale=3; ($ART * $loopNum)")
    pResponsetime=$(bc <<< "scale=3; ($pResponsetime + $temp)")
done
#calculate final response time
total_req=$(($numClients * $loopNum))
Avg_Response_time=$(bc <<< "scale=3; $pResponsetime / ($total_req*1000000)")

#sending rows to respective files
echo "$numClients,$Overall_throughput" >> ./build/throughput.csv
echo "$numClients,$Avg_Response_time" >> ./build/responsetime.csv
