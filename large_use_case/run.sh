#!/bin/bash
set -e -o pipefail
set -x
########## Pay attention: Please install R lib in personal directory before running this script

wai=$(dirname $(readlink -f $0))
[ -e "/tmp/args.sh" ] && onG5K=1 || onG5K=0
[ $onG5K -eq 1 ] && date=$(cat "~/ts.txt") || date=$(date "+%s")

sg="${wai}/simgrid/"
ns="${wai}/ns-3/"
[ $onG5K -eq 1 ] && logs="/tmp/logs/" || logs="${wai}/logs/$date/"
rm -rf "$logs" && mkdir -p "$logs"
[ $onG5K -eq 1 ] && csv="/tmp/results.csv" || csv="${logs}/results.csv"
nSub=16 # Number of parrallel simulations

init_csv(){
        header="simulator,seedP,seedNS,nbCells,minSize,maxSize,startTime,endTime,minSTAPerCell,maxSTAPerCell,AP_BW,nbSTATot,platformFileNS,platformFileSG,flowsFile,simTime,swappedCount,cpuTimeU,cpuTimeS,wallClock,peakMemUsage,minNbMsg,maxNbMsg,meanSize,devSize,avgNbMsg,devNbMsg,meanIntervalBetweenMsg,minIntervalBetweenMsg,devIntervalBetweenMsg,minIntervalBetweenMsg,maxIntervalBetweenMsg,meanStaPerCell,devStaPerCell,type,thByPartFile,enByPartFile,totEnergy,energyDyn,energyStat,corrF" && echo "$header" > "$csv"
}

run_once() {

    args="${date}_${seedP}_${seedNS}_${nbCells}_${minSTAPerCell}_${maxSTAPerCell}_${minNbMsg}_${maxNbMsg}_${meanSize}_${devSize}_${meanStaPerCell}_${devStaPerCell}_${type}"
    nsout="${logs}/NS3_${args}.log"
    sgout="${logs}/SG_${args}.log"

    # generate platforms and flows first
    [ "$type" = "rand" ] && { python platform_generator.py rand ${nbCells} ${minSTAPerCell} ${maxSTAPerCell} ${AP_BW} ${seedP} ${logs}/${args} ${corrF}; }
    [ "$type" = "norm" ] && { python platform_generator.py norm ${nbCells} ${meanStaPerCell} ${devStaPerCell} ${AP_BW} ${seedP} ${logs}/${args} ${corrF}; }
    # obtain number of stas from platform
    nbSTATot=`awk -f getNBSta.awk ${logs}/${args}_ns_platf.xml`
    [ "$type" = "rand" ] && { python flow_generator.py rand ${nbSTATot} ${minSize} ${maxSize} ${startTime} ${endTime} ${simTime} ${minNbMsg} ${maxNbMsg} ${minIntervalBetweenMsg} ${maxIntervalBetweenMsg} ${seedP} > ${logs}/flows_${args}.csv; }
    [ "$type" = "norm" ] && { python flow_generator.py norm ${nbSTATot} ${meanSize} ${devSize} ${startTime} ${endTime} ${simTime} ${avgNbMsg} ${devNbMsg} ${meanIntervalBetweenMsg} ${devIntervalBetweenMsg} ${seedP} > ${logs}/flows_${args}.csv; }

    echo -e '\e[1;32m---------- RUN ---------\e[0m' >&2
    # Run ns-3 simulations
    tStart=$(date "+%s")
    [ $useNS3 -eq 1 ] && { /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $ns ARGS="--flowsFile=${logs}/flows_${args}.csv --platformFile=${logs}/${args}_ns_platf.xml --verbose_flow_th=${verbose_flow_th} -seed=${seedNS} --duration=${simTime} --csvFlowsName=${logs}/${args}_flowData_ns3.csv" run |& tee $nsout >&2; }
    tNS3=$(( $(date "+%s") - tStart))


    # Run simgrid simulations
    tStart=$(date "+%s")
    /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $sg ARGS="${logs}/${args}_sg_platf.xml ${logs}/flows_${args}.csv ${simTime} --cfg=plugin:link_energy_wifi" run |& tee $sgout >&2
    tSG=$(( $(date "+%s") - tStart))
    echo -e '\e[1;32m---------- RUN END ---------\e[0m\n' >&2

    [ ! -e "$csv" ] && init_csv;

    ##### Gen CSV #####
    exec 100>>"$csv"
    flock -x 100 # Lock the file

    [ $useNS3 -eq 1 ] && { awk -f ${ns}/parseThroughputs.awk ${nsout} > ${logs}/throughputs_${args}.csv; }
    [ $useNS3 -eq 1 ] && { ns_bash=$(awk -f parsePerf.awk $nsout); }
    [ $useNS3 -eq 1 ] && { eval "$ns_bash"; }
    [ $useNS3 -eq 1 ] && { ns_bash2=$(awk -f ns-3/parseEnergy.awk $nsout); }
    [ $useNS3 -eq 1 ] && { eval "$ns_bash2"; }
    [ $useNS3 -eq 1 ] && { echo "ns3,${seedP},${seedNS},${nbCells},${minSize},${maxSize},${startTime},${endTime},${minSTAPerCell},${maxSTAPerCell},${AP_BW},${nbSTATot},${args}_ns_platf.xml,${args}_sg_platf.xml,flows_${args}.csv,${simTime},${swappedCount},${cpuTimeU},${cpuTimeS},${wallClock},${peakMemUsage},${minNbMsg},${maxNbMsg},${meanSize},${devSize},${avgNbMsg},${devNbMsg},${meanIntervalBetweenMsg},${minIntervalBetweenMsg},${devIntervalBetweenMsg},${minIntervalBetweenMsg},${maxIntervalBetweenMsg},${meanStaPerCell},${devStaPerCell},${type},${args}_throughput_bypart_all.csv,${args}_energy_bypart_all.csv,${totEnergyConsumption},${energyDyn},${energyStat},NA" >&100; }

    awk -f ${sg}/parse.awk ${sgout} > ${logs}/${args}_flowData_sg.csv
    sg_bash=$(awk -f parsePerf.awk $sgout)
    eval "$sg_bash"
    sg_bash2=$(awk -f simgrid/parseEnergy.awk $sgout)
    eval "$sg_bash2"
    echo "sg,${seedP},${seedNS},${nbCells},${minSize},${maxSize},${startTime},${endTime},${minSTAPerCell},${maxSTAPerCell},${AP_BW},${nbSTATot},${args}_ns_platf.xml,${args}_sg_platf.xml,flows_${args}.csv,${simTime},${swappedCount},${cpuTimeU},${cpuTimeS},${wallClock},${peakMemUsage},${minNbMsg},${maxNbMsg},${meanSize},${devSize},${avgNbMsg},${devNbMsg},${meanIntervalBetweenMsg},${minIntervalBetweenMsg},${devIntervalBetweenMsg},${minIntervalBetweenMsg},${maxIntervalBetweenMsg},${meanStaPerCell},${devStaPerCell},${type},${args}_throughput_bypart_all.csv,${args}_energy_bypart_all.csv, ${totEnergyConsumption},${energyDyn},${energyStat},${corrF}" >&100

    awk -f ${sg}/parseThroughputByPart.awk ${sgout} > ${logs}/${args}_throughput_bypart_sg.csv
    awk -f ${sg}/parseEnergyByPart.awk ${sgout} > ${logs}/${args}_energy_bypart_sg.csv
    [ $useNS3 -eq 1 ] && { awk -f ${ns}/parseThroughputByPart.awk ${nsout} > ${logs}/${args}_throughput_bypart_ns.csv; }
    [ $useNS3 -eq 1 ] && { awk -f ${ns}/parseEnergyByPart.awk ${nsout} >> ${logs}/${args}_energy_bypart_ns.csv; }

    cat ${logs}/${args}_throughput_bypart_sg.csv >> ${logs}/${args}_throughput_bypart_all.csv
    [ $useNS3 -eq 1 ] && { tail -n +2 ${logs}/${args}_throughput_bypart_ns.csv >> ${logs}/${args}_throughput_bypart_all.csv; }


    cat ${logs}/${args}_energy_bypart_sg.csv >> ${logs}/${args}_energy_bypart_all.csv
    [ $useNS3 -eq 1 ] && { tail -n +2 ${logs}/${args}_energy_bypart_ns.csv >> ${logs}/${args}_energy_bypart_all.csv; }

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
    # Do data analysis
    echo -e '\n\e[1;32m---------- Running R data analysis ---------\e[0m'
    R --no-save -q -e 'source("'${wai}/analysis.R'")'
    echo -e '\e[1;32m---------- Done -----------\e[0m'

    # Generate pdf
    echo -e '\n\e[1;32m---------- Generating PDF ---------\e[0m'
    ${wai}/gen-pdf.sh
    pkill -SIGHUP mupdf # Refresh pdf viewer
    echo -e '\e[1;32m---------- Done -----------\e[0m'
else
    touch /tmp/finish.txt
fi
