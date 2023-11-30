#! /bin/bash


timesDropped=0
numClients=8
clientIncr=8
DESIREDDROPS=4

while [[ $timesDropped -lt $DESIREDDROPS ]]
do
    echo "============================================================"
    # send signal to notify server that load is going to come
    ./build/signalSender 0.0.0.0:8000 $numClients
    sleep 2
    ./loadtest.sh $numClients 5 0
    numDropped=$?
    sleep 2
    ./build/signalSender 0.0.0.0:8000 $numClients
     # send signal to notify server thatload has ended
    if [[ numDropped -gt 0 ]]
    then
        timesDropped=$(($timesDropped + 1))
        echo "Times Dropped Increased To: $timesDropped"
    fi
    # increament the number of client
    numClients=$(($numClients + $clientIncr))
    echo "============================================================"
    sleep 5
done