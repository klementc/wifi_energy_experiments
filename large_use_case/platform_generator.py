#!/usr/bin/python

import random
import sys, os
import numpy

if len(sys.argv) < 7:
    print("Usage for rand values: ./"+os.path.basename(__file__)+" rand <nbCells> <minSTAPerCell> <maxSTAPerCell> <AP_BW> <seed> [filePrefix]")
    print("Usage for normal law: ./"+os.path.basename(__file__)+" norm <nbCells> <mean> <sd> <AP_BW> <seed> [filePrefix]")
    exit(1)

randomL = True
if(sys.argv[1] == "norm"):
  randomL = False

print("Use normal law: {}".format(not randomL))

# reproducibility
prefix = "DEFAULT"
if(len(sys.argv) > 8):
    prefix = sys.argv[7]

if (randomL):
  nbCells = int(sys.argv[2])
  minSTAPerCell = int(sys.argv[3])
  maxSTAPerCell = int(sys.argv[4])
  bw = sys.argv[5]
  random.seed(sys.argv[6])
  corrF = float(sys.argv[8])
else:
  nbCells = int(sys.argv[2])
  meanStaPerCell = int(sys.argv[3])
  dev = int(sys.argv[4])
  bw = sys.argv[5]
  numpy.random.seed(int(sys.argv[6]))
  random.seed(sys.argv[6])
  corrF = float(sys.argv[8])

fgraph = open(prefix+"_graphData.csv", "w")
fgraph.write("from, to\n")
platf_sg = open(prefix+"_sg_platf.xml", "w")
platf_ns = open(prefix+"_ns_platf.xml", "w")
print(prefix);


# header and wired zone
platf_sg.write("""<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">

<!--
Generated authomatically using:
{}
Seed: {}
-->
<platform version="4.1">
<zone id="world" routing="Full">
    <!-- Gateway zone (only one node, supposedly the rest of the world-->
    <zone id=\"gateway\" routing=\"Full\">
        <host id=\"OUT\" speed=\"100.0Mf,50.0Mf,20.0Mf\" />
    </zone>
""".format("./"+os.path.basename(__file__)+" "+" ".join(sys.argv), sys.argv[5]))


# generate all wifi cells
staCounter = 0
for i in range(nbCells):
  platf_sg.write("""    <zone id="{}" routing="Wifi">
        <prop id="access_point" value="router_{}" />\n""".format(i, i))
  if (randomL):
    staInCell = random.randint(minSTAPerCell, maxSTAPerCell)
  else:
    staInCell = int(numpy.random.normal(meanStaPerCell, dev, 1)[0])
  platf_ns.write(str(i)+" "+str(staInCell)+"\n")
  for j in range(staInCell):
    platf_sg.write("\t<host id=\"STA_{}\" speed=\"100.0Mf,50.0Mf,20.0Mf\" />\n".format(str(staCounter)))
    fgraph.write("STA_{},AP_{}\n".format(staCounter,i))
    staCounter += 1


  platf_sg.write("""        <!-- Declare the wifi media (after hosts because our parser is sometimes annoying) -->
        <link id="AP_{}" sharing_policy="WIFI" bandwidth="{}" latency="0ms">
                  <prop id = "control_duration" value = "{}"/>
                  <prop id = "wifi_watt_values" value = "0.82:1.14:0.94:0.10"/>
        </link>
        <router id="router_{}"/>
    </zone>\n""".format(i, bw, corrF, i))

# create the link between cells and the rest of the world (in practice we just use very high bandwidth links to avoir any side effect)
platf_sg.write("""<!-- AS Routing -->""")
for i in range(nbCells):
  platf_sg.write("""    <link id="Collector_{}" sharing_policy="SHARED" bandwidth="100Gbps" latency="0ms" />\n""".format(i))
for i in range(nbCells):
  platf_sg.write("""    <zoneRoute src="{}" dst="gateway" gw_src="router_{}" gw_dst="OUT">
      <link_ctn id="Collector_{}" />
    </zoneRoute>\n""".format(i,i,i))
  fgraph.write("AP_{},OUT\n".format(i))



# close opened tags
platf_sg.write("""</zone>
</platform>
""")

platf_ns.close()
platf_sg.close()
fgraph.close()
