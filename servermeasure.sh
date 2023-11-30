#! /bin/bash

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    MSYS_NT*)   machine=Git;;
    *)          machine="UNKNOWN:${unameOut}"
esac
echo ${machine}


fl="logs/server_stats.csv"
# putting header into the file
:>$fl
echo "num_threads,cpu_usage" >> $fl

serverProcId=$1

while true
do
    if [[ $machine == "Mac" ]]
    then
        numThreads=$(ps -M $serverProcId | grep -v USER | wc -l)
        cpuUsage=$(top -l  2 | grep -E "^CPU" | tail -1 | awk '{ print $3 + $5 }')
    else
        numThreads=$(ps -o nlwp --no-header $serverProcId)
        cpuUsage=$(echo $[100- $(vmstat 1 2| tail -1 | awk '{print $15}')])
    fi

    echo "$numThreads,$cpuUsage" >> $fl
    sleep 10
done