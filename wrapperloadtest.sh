#! /bin/bash


timesDropped=0
numClients=8
clientIncr=8
DESIREDDROPS=4

while [[ $timesDropped -lt $DESIREDDROPS ]]
do
    echo "============================================================"
    ./loadtest.sh $numClients 5 0
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