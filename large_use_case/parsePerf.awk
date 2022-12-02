#!/usr/bin/awk -f

BEGIN {
    swappedCount=-1
    cpuTimeU=-1
    cpuTimeS=-1
    wallClock=-1
    peakMemUsage=-1
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
    print("swappedCount="swappedCount";cpuTimeU="cpuTimeU";cpuTimeS="cpuTimeS";wallClock="wallClock";peakMemUsage="peakMemUsage)
}
