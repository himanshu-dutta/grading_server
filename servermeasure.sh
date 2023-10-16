#! /bin/bash

fl="server_stats.csv"

:>$fl
echo "num_threads,cpu_usage" >> $fl

while true
do

    numThreads=$(ps huH p 1 | wc -l)
    cpuUsage=$(vmstat -w | awk 'FNR>2 {print $(NF-3)}')
    echo "$numThreads,$cpuUsage" >> $fl
    sleep 10
done
