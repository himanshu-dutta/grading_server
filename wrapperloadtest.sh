#! /bin/bash


timesDropped=0
numClients=4
clientIncr=4
DESIREDDROPS=4

while [[ $timesDropped -lt $DESIREDDROPS ]]
do
    echo "============================================================"
    ./build/signalSender 0.0.0.0:8000 $numClients
    ./loadtest.sh $numClients 5 0
    ./build/signalSender 0.0.0.0:8000 $numClients
    numDropped=$?
    if [[ numDropped -gt 0 ]]
    then
        timesDropped=$(($timesDropped + 1))
        echo "Times Dropped Increased To: $timesDropped"
    fi
    
    numClients=$(($numClients + $clientIncr))
    echo "============================================================"
    sleep 5
done