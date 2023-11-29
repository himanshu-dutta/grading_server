#! /bin/bash


timesDropped=0
numClients=4
clientIncr=4
DESIREDDROPS=4

while [[ $timesDropped -lt $DESIREDDROPS ]]
do
    echo "============================================================"
    ./build/signalSender 0.0.0.0:8000 $numClients
    sleep 2
    ./loadtest.sh $numClients 5 0
    numDropped=$?
    sleep 2
    ./build/signalSender 0.0.0.0:8000 $numClients
    if [[ numDropped -gt 0 ]]
    then
        timesDropped=$(($timesDropped + 1))
        echo "Times Dropped Increased To: $timesDropped"
    fi
    
    numClients=$(($numClients + $clientIncr))
    echo "============================================================"
    sleep 5
done