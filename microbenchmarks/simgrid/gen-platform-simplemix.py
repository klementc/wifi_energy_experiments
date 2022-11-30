#!/usr/bin/python

import sys, os
wai=os.path.dirname(os.path.abspath(__file__))

if len(sys.argv) != 5:
    print("Usage: ./"+os.path.basename(__file__)+" <nSTAPerAP> <bandwidths> <factor> <cancelidleEnergy>")
    exit(1)
else:
    nSTAPerAP=int(sys.argv[1])
    bandwidth=sys.argv[2]
    factor=float(sys.argv[3])
    cancelIDLEEnergy=int(sys.argv[4])
    if(cancelIDLEEnergy):
      idleE=0
    else:
      idleE=0.82
def pwrite(text,override=False):
    print(text+"\n")



header="""<?xml version='1.0'?>

<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
  <zone id="world" routing="Full">

"""
footer="""
    <!-- Links -->
    <link id="Collector1" sharing_policy="SHARED" bandwidth="10Gbps" latency="0ms" />

    <!-- Route between wlans -->
    <!-- wlan1 <-> wlan2 -->
    <zoneRoute src="wlan1" dst="wlan2" gw_src="WIFIrouter1" gw_dst="WIFIrouter2">
      <link_ctn id="Collector1"/>
    </zoneRoute>
  </zone>
</platform>
"""

def buildT():

    for i in range(1,3):
        pwrite("""    <zone id="wlan{}" routing="Wifi">
        <!-- First declare the Access Point (ie, the wifi media) -->
        <prop id="access_point" value="WIFIrouter{}" />
""".format(i,i))
        pwrite("""    <!-- Declare the stations of this wifi zone -->""")
        for j in range(nSTAPerAP):
            pwrite("""    <host id="STA{}" speed="100.0Mf,50.0Mf,20.0Mf"></host>""".format((i-1)*nSTAPerAP+j))
        pwrite("""         <link id="AP{}" sharing_policy="WIFI" bandwidth="{}"  latency="0ms">
                  <prop id = "control_duration" value = "{}"/>
                  <prop id = "wifi_watt_values" value = "{}:1.14:0.94:0.10"/>
        </link>
        <router id="WIFIrouter{}"/>
    </zone>""".format(i,bandwidth,factor,idleE,i))


pwrite(header,True)
buildT()
pwrite(footer)