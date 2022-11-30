#!/usr/bin/awk -f

BEGIN {
  print("simulator, start, end, size,nbPckts")
}

/Overall throughput for/ {
  start=$4
  end=$6
  size=$11
  oSize=$10 #$7 $8
  print("ns,"start","end","size","oSize)
}