#!/usr/bin/awk -f

BEGIN{
    totEnergyConsumption=0
    energyDyn=0
    energyStat=0
}

/consumed:/ {
    totEnergyConsumption+=$7
    energyDyn+=$10
    energyStat+=$12
}

END {
    print("totEnergyConsumption="totEnergyConsumption";energyDyn="energyDyn";energyStat="energyStat)
}