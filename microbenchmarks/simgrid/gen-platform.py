#!/usr/bin/python

import sys, os
wai=os.path.dirname(os.path.abspath(__file__))

if len(sys.argv) != 7:
    print("Usage: ./"+os.path.basename(__file__)+" <topo> <nNodePair> <nNode> <bitRate> <correction-factor> <cancelIdleEnergy>")
    exit(1)
else:
    topo=int(sys.argv[1])
    nNodePair=int(sys.argv[2])
    nNode=int(sys.argv[3])
    WIFI_BW="{}".format(sys.argv[4])
    corrF=float(sys.argv[5])
    cancelIDLEEnergy=int(sys.argv[6])
    if(cancelIDLEEnergy):
      idleE=0
    else:
      idleE=0.82
def pwrite(text,override=False):
    print(text+"\n")



header="""<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
<platform version="4.1">
<zone id="world" routing="Full">
  <zone id="WIFI zone" routing="Wifi">
"""
footer="""  </zone>

    <!-- outNode AS -->
    <zone id="Wired zone" routing="Full">
      <host id="outNode" speed="100.0Mf,50.0Mf,20.0Mf" />
    </zone>


    <!-- AS Routing -->
    <link id="Collector" sharing_policy="SHARED" bandwidth="100Mbps" latency="0ms" />
    <zoneRoute src="WIFI zone" dst="Wired zone" gw_src="WIFI router" gw_dst="outNode">
      <link_ctn id="Collector" />
    </zoneRoute>
</zone>
</platform>"""

def buildT1():
    pwrite("""    <prop id="access_point" value="WIFI router" />""")
    pwrite("""    <!-- Declare the stations of this wifi zone -->""")

    for i in range(0,2*nNodePair):
        pwrite("""    <host id="STA{}" speed="100.0Mf,50.0Mf,20.0Mf"></host>""".format(i))

    pwrite("""    <!-- Declare the wifi media (after hosts because our parser is sometimes annoying) -->""")
    pwrite("""    <link id="AP1" sharing_policy="WIFI" bandwidth="{}"  latency="0ms">
                  <prop id = "control_duration" value = "{}"/>
                  <prop id = "wifi_watt_values" value = "{}:1.14:0.94:0.10"/>
                </link>""".format(WIFI_BW, corrF,idleE))
    pwrite("""    <router id="WIFI router"/>""")

def buildT2():
    pwrite("""    <prop id="access_point" value="WIFI router" />""")
    pwrite("""    <!-- Declare the stations of this wifi zone -->""")

    for i in range(0,nNode):
        pwrite("""    <host id="STA{}" speed="100.0Mf,50.0Mf,20.0Mf"></host>""".format(i))

    pwrite("""    <!-- Declare the wifi media (after hosts because our parser is sometimes annoying) -->""")
    pwrite("""    <link id="AP1" sharing_policy="WIFI" bandwidth="{}"  latency="0ms">
                  <prop id = "control_duration" value = "{}"/>
                  <prop id = "wifi_watt_values" value = "{}:1.14:0.94:0.10"/>
                  </link>""".format(WIFI_BW, corrF,idleE))
    pwrite("""    <router id="WIFI router"/>""")


if topo==1:
    pwrite(header,True)
    buildT1()
    pwrite(footer)
elif topo==2:
    pwrite(header,True)
    buildT2()
    pwrite(footer)
else:
    print("Topology unknown.")

