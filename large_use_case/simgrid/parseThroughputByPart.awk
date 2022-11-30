#!/usr/bin/awk -f

BEGIN {
  print("simulator, start, end, size,nbPckts")
}

/Totdatav2 between/ {
  start=$6
  end=$8
  size=$10
  print("sg,"start","end","size",0")
}