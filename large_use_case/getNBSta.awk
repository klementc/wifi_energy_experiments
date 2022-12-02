#!/usr/bin/awk -f


BEGIN {
  nbSTA=0
}

{
  nbSTA+=$2
}

END {
  print(nbSTA)
}