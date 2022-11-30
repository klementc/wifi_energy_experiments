#!/usr/bin/awk -f

BEGIN {
  print("simulator, timestamp, energy")
}

/Energy at time/ {
  timestamp=$4
  en=$6
  print("ns,"timestamp","en)
}
