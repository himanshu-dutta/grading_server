#!/bin/bash

function killer() {
    pids=$(ps -e | grep './build/loadtestclient' | awk '{print $1}')
    for pid in $pids; do
        kill -9 $pid
    done
}


threshold=5
consecutive_count=0

while true; do
    count=$(ps -e | grep './build/loadtestclient' | wc -l)
    echo "Current count $count"

    if [ $count -lt $threshold ]; then
        ((consecutive_count++))
    else
        consecutive_count=0
    fi

    if [ $consecutive_count -eq 2 ]; then
        echo "Initiating client destruction; count $count"
        pids=$(ps -e | grep './build/loadtestclient' | awk '{print $1}')
        for pid in $pids; do
            kill -9 $pid
        done
        consecutive_count=0
        sleep 2
    else
        sleep 1
    fi
done
