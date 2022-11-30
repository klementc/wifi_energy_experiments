#!/usr/bin/awk -f

BEGIN {
  print("timestamp,flow,throughput")
}

/ThroughInfo/ {
    print($3","$5","$7)
}
