#!/usr/bin/awk -f

BEGIN {

    totEnergyConsumption=0
}


/Tot energy:/ {
    totEnergyConsumption+=$3
    energyDyn="NA"
    energyStat="NA"
}

END {
    print("totEnergyConsumption="totEnergyConsumption";energyDyn="energyDyn";energyStat="energyStat)
}