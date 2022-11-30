#!/usr/bin/awk -f

BEGIN{
    dataSent=0
    dataReceived=0
    swappedCount=-1
    cpuTimeU=-1
    cpuTimeS=-1
    wallClock=-1
    peakMemUsage=-1
    totEnergyConsumption=0
    energyDyn=0
    energyStat=0
    durIDLE=0
    durRXTX=0
}

/consumed:/ {
    totEnergyConsumption+=$7
    energyDyn+=$10
    energyStat+=$12
    durIDLE+=$14
    durRXTX+=$16
}

/bytes to / { # /Simulation took/ {
    simTime=$11+0 #$5+0
}

#/STA[0-9]+ sent/{
#    dataSent+=$6
#}

/Sent:/ {
    dataSent+=$5
    dataReceived+=$7
}

/Sending check:/ {
    dataReceived+=$8
}
/swappedCount/ {
    split($1,a,":")
    swappedCount=a[2]+0
    split($2,a,":")
    cpuTimeU=a[2]+0
    split($3,a,":")
    cpuTimeS=a[2]+0
    split($4,a,":")
    wallClock=a[2]+0
    split($5,a,":")
    peakMemUsage=a[2]+0
}

END {
    print("dataSent="dataSent";dataReceived="dataReceived";swappedCount="swappedCount";cpuTimeU="cpuTimeU\
        ";cpuTimeS="cpuTimeS";wallClock="wallClock";peakMemUsage="peakMemUsage";totEnergyConsumption="totEnergyConsumption";energyDyn="energyDyn";energyStat="energyStat";durIDLE="durIDLE";durRXTX="durRXTX)
}