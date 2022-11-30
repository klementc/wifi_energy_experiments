#!/usr/bin/awk -f

BEGIN {
  print("simulator, timestamp, energy")
}

/Tot energy at/ {
  timestamp=$7
  en=$9
  print("sg,"timestamp","en)
}
