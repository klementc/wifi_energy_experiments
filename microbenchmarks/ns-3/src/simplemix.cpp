#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/energy-module.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/log.h"
#include <boost/algorithm/string.hpp>

#include "modules.hpp"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPlatform");

bool cancelIDLEEnergy = false;
std::string mcs;
double simTime = 60;


NetDeviceContainer addLink(Ptr<Node> n1, Ptr<Node> n2)
{
  NodeContainer nc (n1, n2);
  PointToPointHelper ptph;
  ptph.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
  ptph.SetChannelAttribute("Delay", StringValue("0ms"));

  return ptph.Install(nc);
}

int main(int argc, char** argv)
{
  int verbose = 0;
  int nNodesPerWLan = 4;
  double startTime = 10;
  double seed = 0;
  double flowSize = 0;

  CommandLine cmd;
  cmd.AddValue ("cancelIDLEEnergy", "account or not for the idle energy consumption", cancelIDLEEnergy);
  cmd.AddValue ("seed", "Simulator seed", seed);
  cmd.AddValue ("verbose", "Show sinks logs or not (significant size when true for large experiments", verbose);
  cmd.AddValue ("mcs", "MCS to use for data and control", mcs);
  cmd.AddValue ("simTime", "Communication duration", simTime);
  cmd.AddValue ("nNodesPerWLan", "Number of stations per Wi-Fi lan (tot number of STA = 4*nNodesPerWLan)", nNodesPerWLan);
  cmd.AddValue ("flowSize", "Size of each flow in bytes", flowSize);



  cmd.Parse (argc, argv);

  if(verbose) {
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  ns3::RngSeedManager::SetSeed (seed);

  // bone network, add wifi nodes later
  NodeContainer backbone;
  backbone.Create(2);

  InternetStackHelper stackHelper;
  stackHelper.Install(backbone);

  Ipv4AddressHelper ipv4addr;
  // backbone
  ipv4addr.SetBase("10.1.1.0", "255.255.255.0");
  NetDeviceContainer backboneD = addLink(backbone.Get(0), backbone.Get(1));
  Ipv4InterfaceContainer backboneInt = ipv4addr.Assign(backboneD);

  std::vector<DeviceEnergyModelContainer> energyModels;
  //container for all sta nodes (reused for communications later)
  NodeContainer stas;
  NodeContainer aps;

  //add wifi to extremity nodes
  for(int i=0;i<2;i++){
    NodeContainer wifiStaNodes;
    NodeContainer wifiAPNode;
    wifiStaNodes.Create(nNodesPerWLan);
    if(i==0)
      wifiAPNode = backbone.Get(0);
    if(i==1)
      wifiAPNode = backbone.Get(1);

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel (wifiChannel.Create ());
    // physical parameters: Single antennas, 5GHz
    wifiPhy.Set ("Antennas", UintegerValue (1));
    wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (1));
    wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (1));
    wifiPhy.Set ("ChannelWidth", UintegerValue(40));
    wifiPhy.Set ("TxPowerStart", DoubleValue (20)); // dBm  (1.26 mW)
    wifiPhy.Set ("TxPowerEnd", DoubleValue (20));
    wifiPhy.Set ("ChannelSettings", StringValue (std::string ("{0, 40, BAND_5GHZ, 0}")));

    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_STANDARD_80211n);
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode", StringValue (mcs), // Before was HtMcs7
                                    "ControlMode", StringValue (mcs));

    WifiMacHelper mac;
    std::ostringstream ss1;
    ss1 <<"wlan"<<i;
    Ssid ssid = Ssid (ss1.str().c_str());
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (wifiPhy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifiHelper.Install (wifiPhy, mac, wifiAPNode);

    MobilityHelper mobilitySTA;
    mobilitySTA.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
    mobilitySTA.SetPositionAllocator("ns3::UniformDiscPositionAllocator");
    mobilitySTA.Install(wifiStaNodes);
    // Ap on the center
    MobilityHelper mobilityAP;
    mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
    mobilityAP.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                  "rho",DoubleValue(10));
    mobilityAP.Install(wifiAPNode);

    // energy model
    BasicEnergySourceHelper energySource;
    // put very high value to have "non-battery" nodes..
    energySource.Set("BasicEnergySourceInitialEnergyJ", DoubleValue (10000000));

    EnergySourceContainer batteryAP = energySource.Install(wifiAPNode);
    EnergySourceContainer batterySTA = energySource.Install(wifiStaNodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
   if (cancelIDLEEnergy)
    radioEnergyHelper.Set("IdleCurrentA", DoubleValue (0));

    DeviceEnergyModelContainer consumedEnergyAP = radioEnergyHelper.Install(apDevices, batteryAP);
    DeviceEnergyModelContainer consumedEnergySTA =  radioEnergyHelper.Install(staDevices, batterySTA);
    energyModels.push_back(consumedEnergyAP);
    energyModels.push_back(consumedEnergySTA);

    stackHelper.Install(wifiStaNodes);

    std::ostringstream ss;
    ss <<"11.1."<<4+i<<".0";
    ipv4addr.SetBase(ss.str().c_str(), "255.255.255.0");
    ipv4addr.Assign(apDevices);
    ipv4addr.Assign(staDevices);

    stas.Add(wifiStaNodes);
    aps.Add(wifiAPNode);
  }

  // create flows on port 80
  PacketSinkHelper STASink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), 80));

  // install sink on every station
  for(int i=0;i<stas.GetN();i++) {
    STASink.Install(stas.Get(i));
  }

  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (false));
  // callbacks
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback(&MacRxDropCallback));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRx));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/State", MakeCallback(&PhyStateTrace));


  for(int i=0; i< stas.GetN()/2; i++){

    int src = i;
    int dst = i+(stas.GetN()/2);

    if (flowSize == 0) {
      std::cout << "No active flows (flowSize=0 specified)" << std::endl;
    } else {
      std::cout << src<< "->"<<dst<<":"<<flowSize<<std::endl;

      Ipv4Address dstAddr = (stas.Get(dst)->GetObject<Ipv4>())->GetAddress(1,0).GetLocal();
      BulkSendHelper echoHelper ("ns3::TcpSocketFactory", InetSocketAddress (dstAddr, 80));
      echoHelper.SetAttribute("MaxBytes", UintegerValue(flowSize));
      echoHelper.SetAttribute("StartTime", TimeValue(Seconds(startTime)));
      NS_LOG_UNCOND("Experiment: comm  size: " << 10 << " "<<stas.Get(src)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<"->"<<dstAddr);

      echoHelper.Install(stas.Get(src)).Start(Seconds(startTime));
    }
  }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (simTime));


  Simulator::Run();

  NS_LOG_UNCOND("Time= "<<Simulator::GetImplementation()->Now().GetSeconds()<<" seconds");

  // log results
  double totEnergy = 0;
  for(int i=0; i<energyModels.size(); i++)
  {
    DeviceEnergyModelContainer::Iterator energyIt = energyModels.at(i).Begin();
    while(energyIt!= energyModels.at(i).End()) {
      NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J" << i);
      totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
      energyIt++;
    }
  }

  std::cout << "Tot energy: "<<totEnergy<< std::endl;
  NS_LOG_UNCOND("MacTxDrop: " << MacTxDrop);
  NS_LOG_UNCOND("MacRxDrop: " << MacRxDrop);
  NS_LOG_UNCOND("PhyTxDrop: " << PhyTxDrop);
  NS_LOG_UNCOND("PhyRxDrop: " << PhyRxDrop);

  float totDurIDLE = 0, totDurRx = 0, totDurTx = 0, totDurBusy = 0, totDurSleep = 0, totDurSwitching = 0, totDurOff = 0;

  for (auto e : nodeTimers) {
    totDurIDLE += e.second.second[1];
    totDurRx += e.second.second[2];
    totDurTx += e.second.second[3];
    totDurBusy += e.second.second[4];
    totDurSleep += e.second.second[5];
    totDurSwitching += e.second.second[6];
    totDurOff += e.second.second[7];

    std::cout << "Node timers: "<< e.first << ","<< e.second.second[1]<<","<<
      e.second.second[2]<<","<<
      e.second.second[3]<<","<<
      e.second.second[4]<<","<<
      e.second.second[5]<<","<<
      e.second.second[6]<<","<<
      e.second.second[7]<<std::endl;
  }
  std::cout << "Overall timers: "<<totDurIDLE<<" "<<
      totDurRx<<" "<<
      totDurTx<<" "<<
      totDurBusy<<" "<<
      totDurSleep<<" "<<
      totDurSwitching<<" "<<
      totDurOff<<std::endl;

  Simulator::Destroy();
  return 0;
}
