#!/bin/bash
set -e -o pipefail
set -x

########## Pay attention: Please install R lib in personal directory before running this script

wai=$(dirname $(readlink -f $0))
[ -e "/tmp/args.sh" ] && onG5K=1 || onG5K=0
sg="${wai}/simgrid/"
ns="${wai}/ns-3/"
[ $onG5K -eq 1 ] && csv="/tmp/results.csv" || csv="${wai}/results_local.csv"
nSub=30 # Number of parrallel simulations

init_csv(){
        header="simulator,simTime,dataSize,nSTAPerAP,bitRate,date,seed,name,useNS3,duration,dataSent,dataReceived,appSimTime,swappedCount,cpuTimeU,cpuTimeS,wallClock,peakMemUsage,throughput,MacTxDrop,MacRxDrop,PhyTxDrop,PhyRxDrop,avgSignal,avgNoise,avgSNR,nbSinkTot,totEnergyConsumption,energyDyn,energyStat,durIDLE,durRx,durTx,corrF,mcs,cancelIDLEEnergy" && echo "$header" > "$csv"
}


[ $onG5K -eq 1 ] && date=$(cat "~/ts.txt") || date=$(date "+%s")
[ $onG5K -eq 1 ] && logs="/tmp/logs/" || logs="${wai}/logs/$date/"
rm -rf "$logs" && mkdir -p "$logs"

run_once() {
    args="dataSize${flowSize}_nSTAPerAP${nNodesPerWLan}_seed${seed}_simTime${simTime}"
    nsout="${logs}/NS3_${args}.log"
    sgout="${logs}/SG_${args}.log"

    flowFile="${logs}/coms_${args}.log"
    let nSTATot="2*${nNodesPerWLan}"

    echo -e '\e[1;32m---------- RUN ---------\e[0m' >&2
    # Run ns-3 simulations
    tStart=$(date "+%s")
    [ $useNS3 -eq 1 ] && { /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $ns ARGS="--cancelIDLEEnergy=$cancelIDLEEnergy --seed=${seed} --verbose=${verbose} --mcs=${mcs} --simTime=${simTime} --nNodesPerWLan=${nNodesPerWLan} --flowSize=${flowSize}" run_simplemix |& tee $nsout >&2; }
    tNS3=$(( $(date "+%s") - tStart))

    # Run simgrid simulations
    tStart=$(date "+%s")
    platform=/tmp/${args}_platform.xml
    python $sg/gen-platform-simplemix.py ${nNodesPerWLan} ${bandwidth} ${corrF} ${cancelIDLEEnergy} > ${platform}
    /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $sg ARGS="${platform} ${flowSize} ${simTime} --cfg=plugin:link_energy_wifi" run_simplemix |& tee $sgout >&2
    tSG=$(( $(date "+%s") - tStart))
    echo -e '\e[1;32m---------- RUN END ---------\e[0m\n' >&2

    # Parse logs
    #sg_bash=$(cat $sgout|$sg/parse.awk)
    #eval "$sg_bash"

    [ ! -e "$csv" ] && init_csv;

    ##### Gen CSV #####
    exec 100>>"$csv"
    flock -x 100 # Lock the file


    if [ $useNS3 -eq 1 ]
    then
        ns_bash=$(cat $nsout|$ns/parse.awk)
        eval "$ns_bash"
        echo "ns3,$simTime,$flowSize,$nNodesPerWLan,NA,$date,$seed,$name,$useNS3,${simTime},$dataSent,$dataReceived,$appSimTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,$throughput,$MacTxDrop,$MacRxDrop,$PhyTxDrop,$PhyRxDrop,$avgSignal,$avgNoise,$avgSNR,$nbSinkTot,$totEnergyConsumption,NA,NA,${durIdle},${durRx},${durTx},${corrF},${mcs},${cancelIDLEEnergy}" >&100
    fi
    sg_bash=$(cat $sgout|$sg/parse.awk)
    eval "$sg_bash"
    echo "sg,$simTime,$flowSize,${nNodesPerWLan},$bandwidth,$date,${seed},$name,$useNS3,$simTime,$dataSent,$dataReceived,$simTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,NA,NA,NA,NA,NA,NA,NA,NA,NA,$totEnergyConsumption,$energyDyn,$energyStat,$durIDLE,$durRXTX,${durRXTX},$corrF,${mcs},${cancelIDLEEnergy}" >&100

    flock -u 100 # Unlock the file
}


if [ $onG5K -eq 1 ]
then
    rm -rf /tmp/finish.txt
    rm -rf "$csv"
    #echo "$header" > "$csv"
    over=$(cat /tmp/args.sh|wc -l)
    i=0
    while IFS= read -r line
    do
        eval "$line"
        ( run_once; ) & # Launch as subprocess
        i=$(( i + 1 ))
        echo $i over $over > /tmp/avancement.txt
        while [ $(pgrep -P $$ | wc -l) -ge $nSub ] # Until we have the max of subprocess we wait
        do
            sleep 3
        done
    done < /tmp/args.sh
    wait # Be sure evething is done

else
    echo "Using args file: ${localOnlyArgsFile}"
    rm -rf /tmp/finish.txt
    rm -rf "$csv"
    #echo "$header" > "$csv"
    over=$(cat ${wai}/${localOnlyArgsFile}|wc -l)
    i=0
    while IFS= read -r line
    do
        eval "$line" #&& useNS3=0
        ( run_once; ) & # Launch as subprocess
        i=$(( i + 1 ))
        echo $i over $over > ${wai}/avancement.txt
        while [ $(pgrep -P $$ | wc -l) -ge $nSub ] # Until we have the max of subprocess we wait
        do
            sleep 3
        done
    done < ${wai}/${localOnlyArgsFile}
    wait # Be sure evething is done

fi

if [ $onG5K -ne 1 ]
then
    exit 0 # Skip this step

    echo -e '\e[1;32m---------- Done -----------\e[0m'
else
    touch /tmp/finish.txt
fi
