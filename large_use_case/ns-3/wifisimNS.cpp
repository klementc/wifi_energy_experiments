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
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-net-device.h"
#include <boost/algorithm/string.hpp>
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPlatform");

FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor;
std::map<ns3::FlowId, long> flowToThroughput;
std::map<ns3::FlowId, long> flowToNbPackets;
double prevOverallThMeasure = 0;
double prevOverallPcktMeasure = 0;
double prevOverallThTS = 0;
int nbOverallThMetrics = 0;
double overallThInterval = 10;
bool verbose_flow_th = true;
double sinkRx = 0;
double sinkRx2 = 0;
ns3::ApplicationContainer sinks;
std::vector<ns3::DeviceEnergyModelContainer> consumedEnergyAP;
std::vector<ns3::DeviceEnergyModelContainer> consumedEnergySTA;
typedef std::pair<NetDeviceContainer,NetDeviceContainer> WifiNetDev;

/**
 * Set up energy measurement capacity
 * (provision 'infinite' batteries and put them on the devices)
 */
void setupEnergyMetrics(NodeContainer aps, NodeContainer stas, WifiNetDev netDev) {
    // energy model
   BasicEnergySourceHelper energySource;
   // put very high value to have "non-battery" nodes..
   energySource.Set("BasicEnergySourceInitialEnergyJ", DoubleValue (10000000));

   EnergySourceContainer batteryAP = energySource.Install(aps);
   EnergySourceContainer batterySTA = energySource.Install(stas);

   WifiRadioEnergyModelHelper radioEnergyHelper;
   radioEnergyHelper.Set("IdleCurrentA", DoubleValue (0));

   consumedEnergyAP.push_back(radioEnergyHelper.Install(netDev.first, batteryAP));
   consumedEnergySTA.push_back(radioEnergyHelper.Install(netDev.second, batterySTA));
}


NetDeviceContainer addLink(Ptr<Node> n1, Ptr<Node> n2)
{
  NodeContainer nc (n1, n2);
  PointToPointHelper ptph;
  ptph.SetDeviceAttribute("DataRate", StringValue("1000Gbps"));
  ptph.SetChannelAttribute("Delay", StringValue("0ms"));

  return ptph.Install(nc);
}

NetDeviceContainer addCSMA(NodeContainer nodes) {
  NodeContainer csmaNodes (nodes);
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("1000Gbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (0)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  return csmaDevices;
}

void PacketSinkRx(/*std::string path, */Ptr<const Packet> p, const Address &address){
	float simTime=Simulator::Now().GetSeconds();
	sinkRx+=p->GetSize();
}

void PrintThroughput(){
	  // https://groups.google.com/g/ns-3-users/c/-BnPRmJwcGs
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  for(auto &flow: monitor->GetFlowStats ()){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (flow.first);
    std::ostringstream ssrc;
    t.sourceAddress.Print(ssrc);
    std::ostringstream sdst;
    t.destinationAddress.Print(sdst);

    ns3::FlowId id = flow.first;
    if(! flowToThroughput.count(id)) {
      flowToThroughput[id] = 0;
      flowToNbPackets[id] = 0;
    }

    double throughput = flow.second.rxBytes-flowToThroughput.find(id)->second;
    double nbPackets = flow.second.rxPackets-flowToNbPackets.find(id)->second;
    prevOverallThMeasure += flow.second.rxBytes-flowToThroughput.find(id)->second;
    prevOverallPcktMeasure += flow.second.rxPackets-flowToNbPackets.find(id)->second;

    nbOverallThMetrics++;

    flowToThroughput.find(id)->second=flow.second.rxBytes;
    flowToNbPackets.find(id)->second=flow.second.rxPackets;


	  uint32_t simTime=Simulator::Now().GetSeconds();
    if(verbose_flow_th) {
      NS_LOG_UNCOND("ThroughInfo At "<<simTime<<" Flow "<< flow.first << " Throughput: " << throughput << " nbPackets: "<<nbPackets);
    }
  }
  if(Simulator::Now().GetSeconds()-prevOverallThTS >= overallThInterval) {
    PacketSink& ps=dynamic_cast<PacketSink&>(*(sinks.Get(0)));
    NS_LOG_UNCOND("Overall throughput for "<< prevOverallThTS <<" -> " << Simulator::Now().GetSeconds() << " "<<prevOverallThMeasure<< " "<<prevOverallPcktMeasure<< " sinkRx: "<<sinkRx<< " "<<ps.GetTotalRx()-sinkRx2);
    prevOverallThMeasure = 0;
    nbOverallThMetrics = 0;
    sinkRx=0;
    sinkRx2 = ps.GetTotalRx();
    prevOverallPcktMeasure = 0;
    prevOverallThTS = floor(Simulator::Now().GetSeconds());

    double totEnergy = 0;
    for(auto it : consumedEnergyAP) {
      DeviceEnergyModelContainer::Iterator energyIt = it.Begin();
      while(energyIt!= it.End()) {
        NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (AP)");
        totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
        energyIt++;
      }
    }
    for(auto it : consumedEnergySTA) {
      DeviceEnergyModelContainer::Iterator energyIt = it.Begin();
      while(energyIt!= it.End()) {
        NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (STA)");
        totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
        energyIt++;
      }
    }

    std::cout << "Energy at time "<< Simulator::Now().GetSeconds() <<" : "<<totEnergy<< std::endl;
  }

	Simulator::Schedule (Seconds (1.0), &PrintThroughput);
}

int main(int argc, char** argv)
{
  int verbose=0;
  double duration=200.;
  int seed = 1;
  std::string flowsFile;
  std::string platformFile;
  // used to obtain the STA ID from its adress (and then compare to sg)
  std::map<std::string, std::string> addrToName;
  std::string csvFlowsFName = "default_ns3.csv";
  CommandLine cmd;
  cmd.AddValue ("verbose", "Show sinks logs or not (significant size when true for large experiments", verbose);
  cmd.AddValue("flowsFile", "file containing flow scenario", flowsFile);
  cmd.AddValue("platformFile", "file containing platform information (nb of stations on each AP)", platformFile);
  cmd.AddValue("duration", "simulated time", duration);
  cmd.AddValue("csvFlowsName", "output csv file for flow data", csvFlowsFName);
  cmd.AddValue("overallThInterval", "interval between two overall throughput measurements", overallThInterval);
  cmd.AddValue("seed","seed", seed);
  cmd.AddValue("verbose_flow_th", "Log single flow throughputs over time", verbose_flow_th);
  cmd.Parse (argc, argv);

  ns3::RngSeedManager::SetSeed(seed);
  if(verbose) {
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  std::ofstream outCSV;
  outCSV.open (csvFlowsFName);
  outCSV << "simulator,src,dst,startRx,startTx,endRx,endTx,rcvSize,txSize\n";


  //int payloadSize = 1448; //bytes
  //Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1500));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
  // Disable Short Guard Interval: Add ghost signal to the carrier in order to reduce the collision probability with the data (multipath fading etc..)
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (false));

  NodeContainer outside;
  outside.Create(2);

  InternetStackHelper stackHelper;
  stackHelper.Install(outside);

  Ipv4AddressHelper ipv4addr;

  // container for all wifi nodes
  NodeContainer stas;
  NodeContainer aps;

  // create wifi cells from platform file
  std::ifstream stream(platformFile);
  std::string line;
  int nbSTAsAdded = 0;
  for (std::string line; std::getline(stream, line); ) {
    std::istringstream in(line);
    int apNumber, nbSTAs;
    in >> apNumber >> nbSTAs;

    NS_LOG_UNCOND("Create wifi cell for AP_"+std::to_string(apNumber)+" with "+std::to_string(nbSTAs)+" stations");

    // create nodes
    NodeContainer wlanSTAs;
    NodeContainer wlanAP;
    wlanSTAs.Create(nbSTAs);
    wlanAP.Create(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());
    // physical parameters: Single antennas, 5GHz
    phy.Set ("Antennas", UintegerValue (1));
    phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (1));
    phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (1));
    phy.Set ("ChannelWidth", UintegerValue(40));
    phy.Set ("TxPowerStart", DoubleValue (20)); // dBm  (1.26 mW)
    phy.Set ("TxPowerEnd", DoubleValue (20));
    phy.Set ("ChannelSettings", StringValue (std::string ("{0, 40, BAND_5GHZ, 0}")));

    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_STANDARD_80211n);

    // We setup this according to:
    // this examples/wireless/wifi-spectrum-per-example.cc
    // and https://en.wikipedia.org/wiki/IEEE_802.11n-2009
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode", StringValue ("HtMcs3"), // Before was HtMcs7
                                        "ControlMode", StringValue ("HtMcs3"));

    WifiMacHelper mac;
    std::ostringstream ss1;
    ss1 <<"lan"<<apNumber;
    Ssid ssid = Ssid (ss1.str().c_str());
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (phy, mac, wlanSTAs);

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices;
    apDevices = wifiHelper.Install (phy, mac, wlanAP);

    setupEnergyMetrics(wlanAP, wlanSTAs, WifiNetDev(apDevices, staDevices));
//    phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
//    phy.EnablePcap("AP_capture_s"+std::to_string(apNumber)+"_d1", apDevices.Get(0),false);

    MobilityHelper mobilityAP;
    Ptr<ListPositionAllocator> posAP = CreateObject <ListPositionAllocator>();
    posAP ->Add(Vector(apNumber*100, 0, 0));
    NS_LOG_UNCOND("pos: "<<apNumber*100<<","<<0<<","<<0);
    mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
    mobilityAP.SetPositionAllocator(posAP);
    mobilityAP.Install(wlanAP);
    // Set station positions homogeneously in circle around the origin
    MobilityHelper mobilitySTA;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
    double step=2*M_PI/wlanSTAs.GetN();
    double theta=0;
    for(int i=0;i<wlanSTAs.GetN();i++){
      double r = 15 * sqrt(((double) rand() / (RAND_MAX)));
      double theta = ((double) rand() / (RAND_MAX)) * 2 * M_PI;
      positionAlloc ->Add(Vector((apNumber*100)+cos(theta)*r,sin(theta)*r, 0)); //
    }
    mobilitySTA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilitySTA.SetPositionAllocator(positionAlloc);
    mobilitySTA.Install(wlanSTAs);

    stackHelper.Install(wlanSTAs);
    stackHelper.Install(wlanAP);

    std::ostringstream ss;
    ss <<"11.2."<<4+apNumber<<".0";
    ipv4addr.SetBase(ss.str().c_str(), "255.255.255.0");
    Ipv4InterfaceContainer c = ipv4addr.Assign(staDevices);
    ipv4addr.Assign(apDevices);

    for(Ipv4InterfaceContainer::Iterator s = c.Begin (); s != c.End (); s++){
      ss.str(std::string());
      (*s).first->GetAddress(1,0).GetLocal().Print(ss);
      addrToName[ss.str()] = "STA"+std::to_string(nbSTAsAdded);
      ss.str(std::string());
      nbSTAsAdded++;
    }

      stas.Add(wlanSTAs);
      aps.Add(wlanAP);
  }
  NS_LOG_UNCOND("Finished creating wifi nodes");
  ipv4addr.SetBase("10.1.0.0", "255.255.0.0");

  // link AP to gateway
  NetDeviceContainer backboneD;
  for(uint32_t i=0;i<aps.GetN();i++) {
    ipv4addr.Assign(addLink(aps.Get(i), outside.Get(0)));
  }
  ipv4addr.Assign(addLink(outside.Get(0), outside.Get(1)));
  std::ostringstream ss;
  (outside.Get(1)->GetObject<Ipv4>())->GetAddress(1,0).GetLocal().Print(ss);
  addrToName[ss.str()] = "OUT";

  // create flows on port 80
  PacketSinkHelper STASink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), 80));

  sinks = STASink.Install(outside.Get(1));
  NS_LOG_UNCOND("Created Sinks");
  AddressValue a;
  sinks.Get(0)->GetAttribute("Local", a);
  std::ifstream stream2(flowsFile);


  while(std::getline(stream2, line)){

    std::vector<std::string> fields;
    boost::algorithm::split(fields, line, boost::is_any_of(","));

    int src = std::stoi(fields[0]);
    std::string dst = fields[1];
    int size = std::stoi(fields[2]);
    double start = std::stod(fields[3]);


    std::cout << src<< "->"<<dst<<":"<<size<<" "<<start<<"   sss "<<(outside.Get(1)->GetObject<Ipv4>())->GetNInterfaces()<<std::endl;

    Ipv4Address dstAddr = (outside.Get(1)->GetObject<Ipv4>())->GetAddress(1,0).GetLocal();
    BulkSendHelper echoHelper ("ns3::TcpSocketFactory", InetSocketAddress (dstAddr, 80));
    echoHelper.SetAttribute("MaxBytes", UintegerValue(size));

    echoHelper.SetAttribute("StartTime", TimeValue(Seconds(start)));
    NS_LOG_UNCOND("Experiment: start at "<<start<<" comm  size: " << size << " "<<stas.Get(src)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<"->"<<dstAddr);

    echoHelper.Install(stas.Get(src)).Start(Seconds(start));
  }
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRx));

  monitor = flowmon.InstallAll();
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (duration));
//AnimationInterface anim ("animation.xml");
  Simulator::Schedule (Seconds (10.0), &PrintThroughput);

  Simulator::Run();

  for(ApplicationContainer::Iterator n = sinks.Begin (); n != sinks.End (); n++){
    ns3::Ptr<Application> app=*n;
    PacketSink& ps=dynamic_cast<PacketSink&>(*app);
    NS_LOG_UNCOND("Sink " << " received " << ps.GetTotalRx() << " bytes");
  }

  // https://groups.google.com/g/ns-3-users/c/-BnPRmJwcGs
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  for(auto &flow: monitor->GetFlowStats ()){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (flow.first);
    std::ostringstream ssrc;
    t.sourceAddress.Print(ssrc);
    std::ostringstream sdst;
    t.destinationAddress.Print(sdst);

    NS_LOG_UNCOND("Flow "<< flow.first << " STA: " << addrToName.find(ssrc.str())->second <<  \
    		" ends at " << flow.second.timeLastRxPacket.GetSeconds() << "s" <<\
    		" first rx pkt " << flow.second.timeFirstTxPacket.GetSeconds() << "s" <<\
			" nb lost pkt " << flow.second.lostPackets << " rxBytes " << flow.second.rxBytes << " txBytes " << flow.second.txBytes );

    outCSV << "ns3,"<<addrToName.find(ssrc.str())->second<<","<<addrToName.find(sdst.str())->second<<","<<\
      flow.second.timeFirstRxPacket.GetSeconds()<<","<<flow.second.timeFirstTxPacket.GetSeconds()<<","<<flow.second.timeLastRxPacket.GetSeconds()<<","<<flow.second.timeLastTxPacket.GetSeconds()<<","<<\
      flow.second.rxBytes << "," << flow.second.txBytes<<"\n";

  }


  double totEnergy = 0;
  for(auto it : consumedEnergyAP) {
    DeviceEnergyModelContainer::Iterator energyIt = it.Begin();
    while(energyIt!= it.End()) {
      NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (AP)");
      totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
      energyIt++;
    }
  }
  for(auto it : consumedEnergySTA) {
    DeviceEnergyModelContainer::Iterator energyIt = it.Begin();
    while(energyIt!= it.End()) {
      NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (STA)");
      totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
      energyIt++;
    }
  }

  std::cout << "Tot energy: "<<totEnergy<< std::endl;


  //std::string fname = "NameOfFile.xml";
  //monitor->SerializeToXmlFile(fname, true, true);

  Simulator::Destroy();
  return 0;
}
