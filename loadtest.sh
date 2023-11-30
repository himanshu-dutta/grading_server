#! /bin/bash

#getting the arguments
numClients=$1
loopNum=$2
sleepTimeSeconds=$3
timeout=1

#running number of clients
for i in $(seq 1 $numClients)
do
    ./build/loadtestclient 0.0.0.0:5005 samples/correct.c $loopNum $sleepTimeSeconds $timeout> "./build/client${i}.txt" &
    pids[${i}]=$!

done

#waiting for all the client to done all request and get corresponding responses
for i in $(seq 1 $numClients)
do
    wait ${pids[i]}
    # rm ./build/client${i}.txt
done

#giving path to the file going to be created
tpfile="./build/throughput.csv"
rtfile="./build/responsetime.csv"
ndfile="./build/numDropped.csv"
toutfile="./build/numTimeout.csv"

#adding header initially file will not be there
if [[ ! -f $tpfile ]] 
then
    echo "M,Overall_throughput(nsr/secs)" > ./build/throughput.csv
fi

if [[ ! -f $rtfile ]] 
then
    echo "M,Avg_Response_time(secs)" > ./build/responsetime.csv
fi

if [[ ! -f $ndfile ]] 
then
    echo "M,Num_Clients_Dropped" > ./build/numDropped.csv
fi

if [[ ! -f $toutfile ]] 
then
    echo "M,Num_timeout" > ./build/numTimeout.csv
fi

#read individual throughput, add them and calculate partial response time
Overall_throughput=0
pResponsetime=0
numDropped=0
Overall_NSR=0
Overall_time=0
AvgTout=0
for file in ./build/client*.txt
do
    echo "Looking at $file"
    ATiOUT=$(grep "ATiOUT" $file | cut -d ',' -f 1 | cut -d ':' -f 2)
    ART=$(grep "ATiOUT" $file | cut -d ',' -f 2 | cut -d ':' -f 2)
    NSR=$(grep "ATiOUT" $file | cut -d ',' -f 3 | cut -d ':' -f 2)
    LT=$(grep "ATiOUT" $file | cut -d ',' -f 4 | cut -d ':' -f 2)

    if [[ $ATiOUT == "" ]]
    then
        continue
    fi

    Overall_NSR=$(bc <<< "scale=3; ($Overall_NSR + $NSR)")
    Overall_time=$(bc <<< "scale=3; ($Overall_time + $LT / 1000000)")
    temp=$(bc <<< "scale=3; ($ART * $loopNum)")
    pResponsetime=$(bc <<< "scale=3; ($pResponsetime + $temp)")
    AvgTout=$(bc <<< "scale=3; ($AvgTout + $ATiOUT)")
done
#calculate final response time
total_req=$(bc <<< "scale=3; $numClients * $loopNum")
Avg_Response_time=$(bc <<< "scale=3; $pResponsetime / ($total_req*1000000)")
Overall_throughput=$(bc <<< "scale=3; $Overall_NSR / $Overall_time")

#sending rows to respective files
echo "$numClients,$Overall_throughput" >> ./build/throughput.csv
echo "$numClients,$Avg_Response_time" >> ./build/responsetime.csv
echo "$numClients,$numDropped" >> ./build/numDropped.csv
echo "$numClients,$AvgTout" >> ./build/numTimeout.csv

if [[ $numDropped -gt 0 ]]
then
    exit 1
fi
exit 0