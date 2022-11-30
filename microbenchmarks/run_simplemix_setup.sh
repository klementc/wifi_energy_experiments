#!/bin/bash
set -e -o pipefail
wai=$(dirname $(readlink -f $0))
date=$(date "+%s")

if [[ -z ${hostFile} ]]
then
  echo "You must specify HostFile!"
  exit 1
fi
echo "Using hostFile: ${hostFile}"
#="${wai}/hosts.txt"

[ -z "${suffix}" ] && suffix="DEFAULT"
csv="${wai}/results_${date}_${suffix}.csv"
argsFile="${wai}/args_${date}_${suffix}.sh"

aHost=$(cat $hostFile|head -n1)
#bindLibPath="${wai}/../analysis/" && source "${bindLibPath}/Rbindings.sh"
g5k_user="ccourageuxsudan"

range(){
    min=$(( $1 * 1000000 ))
    max=$(( $2 * 1000000 ))
    count=$3
    range=$(( (max - min + 1) / count))
    seq $min $range $max
}

########## PARAMETERS ##########
echo "Reproduce Scenario: simplemix"

bandwidth="35Mbps" # bitRate in SimGrid in Mbps (obtained from the final plot o>
useNS3=1
local_only=0
name="simplemix"
useDecayModel=0
#threshRTSCTS=100 a rajouter
seeds="$(seq 1 1 40)"
simTimes="$(seq 700 200 1100)"
corrF="0.002"
cancelIDLEEnergy=1
mcs="HtMcs3"
verbose="0"

flowSizes=$(seq 3000000 1000000 20000000)
nNodesPerWLanS="$(seq 1 1 10)"

ECHO (){
    echo -e "\e[32m----------| $@ |----------\e[0m"
}

fetch_results() {
    ##### Wait for last exp finish
    ECHO "Waiting for the experiments to end..."
    finish=0
    while [ $finish -eq 0 ]
    do
        for host in $(cat $hostFile)
        do
            finish=0
            ssh ${g5k_user}@$host "test -e /tmp/finish.txt" && finish=1 || { finish=0; break; }
        done
        sleep 10
    done

    ##### Fetch results
    ECHO "Gathering results"
    for host in $(cat $hostFile)
    do
        echo "Fetch results from $host"
        {
            tmp=$(mktemp)
            rsync -qavh "${g5k_user}@$host:/tmp/results.csv" "$tmp"
            [ ! -e "$csv" ] && head -n1 "$tmp" > "$csv"
            tail -n +2 "$tmp" >> "$csv"
            rm "$tmp"
        } || {
            echo -e "\e[31mCan't fetch results from $host\e[0m"
        }
        {
            mkdir -p "${wai}/logs/$date/"
            rsync -qavh "${g5k_user}@$host:/tmp/logs/" "${wai}/logs/$date/"
        } || {
            echo -e "\e[31mCan't fetch log from $host\e[0m"
        }
    done
}

#[ "$1" == "fetch" ] && { fetch_results; exit 0; }

# First refresh simulator
if [ $local_only -ne 1 ]
then
    ECHO "Refreshing simulators"
    #rsync -avh simgrid --exclude simgrid/simgrid/ ${g5k_user}@$aHost:./project/
    #rsync -avh ns-3 --exclude ns-3/ns-3/ --exclude ns-3/ns-3/ns-333-nopatch/ ${g5k_user}@$aHost:./project/
    #ssh ${g5k_user}@$aHost "cd project/ns-3 && make clean &&  make"
    #ssh ${g5k_user}@$aHost "cd project/simgrid && make clean && make"
    ECHO "Refreshing R bindings"
    #rsync -avh --exclude "plots/" --exclude "decayVSnodecay" $(realpath -s ${bindLibPath}) ${g5k_user}@$aHost:./project/
    ECHO "Refreshing run.sh"
    rsync -avh run.sh ${g5k_user}@$aHost:./project/
fi

ECHO "Generating Arguments"
echo -en > $argsFile
for flowSize in ${flowSizes}
do
    for nNodesPerWLan in ${nNodesPerWLanS}
    do
        for bandwidth in ${bandwidth}
        do
            for seed in ${seeds}
            do
                for simTime in ${simTimes}
                do
                    echo "flowSize=$flowSize;nNodesPerWLan=$nNodesPerWLan;bandwidth=$bandwidth;useNS3=$useNS3;seed=$seed;name=$name;simTime=${simTime};corrF=${corrF};cancelIDLEEnergy=${cancelIDLEEnergy};mcs=${mcs};verbose=${verbose}" >> $argsFile
                done
            done
        done
    done
done

[ $local_only -eq 1 ] && exit 0

# Shuffle lines (more homogeneous load repartition)
tmp=$(mktemp)
cat $argsFile|shuf > $tmp
mv $tmp $argsFile

# Give the experiment timestamp
ECHO "Starting simulations $date"
ssh ${g5k_user}@$aHost "echo $date > ~/ts.txt"


nHost=$(cat $hostFile|wc -l)
nSim=$(cat $argsFile|wc -l)
chunk=$(( nSim / nHost ))

start=1
end=$chunk
for i in $(seq 1 $nHost)
do
    tmp=$(mktemp)
    if [ $i -ne $nHost ]
    then
        cat $argsFile | sed -n "${start},${end}p" > $tmp
    else
        cat $argsFile | tail --lines="+${start}" > $tmp
    fi
    start=$((start + chunk))
    end=$((end + chunk))

    host=$(cat $hostFile | sed -n "${i}p")
    rsync -qavh $tmp ${g5k_user}@$host:/tmp/args.sh
    ssh ${g5k_user}@$host "{ tmux kill-session -t simu 2>/dev/null && echo killing previous experiment on $host; } || true "
    echo "Launching simulation on host $host ($i/$nHost)"
    currDir=${PWD}
    ssh ${g5k_user}@$host tmux -l new-session -s "simu" -d "${currDir}/run_simplemix.sh"
    rm $tmp
done


fetch_results
