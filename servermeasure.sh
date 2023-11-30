#! /bin/bash

fl="server_stats.csv"
# putting header into the file
:>$fl
echo "num_clients,num_threads,cpu_usage" >> $fl

serverProcId=$1

while true
do
    numClients=$(cat ./numClients.txt | cut -d' ' -f1)
    numThreads=$(ps -M $serverProcId | grep -v USER | wc -l)
    # numThreads=$(ps huH p 1 | wc -l)
    cpuUsage=$(top -l  2 | grep -E "^CPU" | tail -1 | awk '{ print $3 + $5 }')
    # cpuUsage=$(cpuUsage=$(vmstat -w | awk 'FNR>2 {print $(NF-3)}'))
    echo "$numClients,$numThreads,$cpuUsage" >> $fl
    sleep 10
done