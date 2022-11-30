#!/usr/bin/awk -f

BEGIN {
  print("simulator, src, dst, start, end, size")
}

/finished task/ {
  src=$7
  dst=$9
  start=$11
  end=$13
  size=$15
  print("sg,"src","dst","start","end","size)
}

