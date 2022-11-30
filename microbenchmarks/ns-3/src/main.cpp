#include "modules.hpp"

// command line variables
bool pcapCapture;
double simTime;
double seed;
std::string mcs;
int threshRTSCTS;
ApplicationContainer Sinks;
bool cancelIDLEEnergy = false;

// for energy measurements
DeviceEnergyModelContainer consumedEnergyAP;
DeviceEnergyModelContainer consumedEnergySTA;


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
   if (cancelIDLEEnergy)
    radioEnergyHelper.Set("IdleCurrentA", DoubleValue (0));

   consumedEnergyAP = radioEnergyHelper.Install(netDev.first, batteryAP);
   consumedEnergySTA =  radioEnergyHelper.Install(netDev.second, batterySTA);
}


/**
 * @brief Make build application
 * @param dst Destination IP address
 * @param port Communication port
 * @param dataSize Amount of data to send
 * @param nodes Node to install the application
 */
void txApp(Ipv4Address dst, int port,int dataSize, NodeContainer nodes, bool limitSize=true){
  for(int i=0; i< nodes.GetN(); i++)
    NS_LOG_UNCOND("Creating sender on Node "<<nodes.Get(i)->GetId());

    BulkSendHelper echoClientHelper ("ns3::TcpSocketFactory",InetSocketAddress (dst, port));
    if(limitSize == true)
      echoClientHelper.SetAttribute ("MaxBytes", UintegerValue (dataSize)); //remove maxbytes to observe an amount of bytes over duration instead of the duration over an amount of bytes sent

    Ptr<UniformRandomVariable> trafficStart = CreateObject<UniformRandomVariable> ();
    double trafStar = trafficStart->GetValue(10.0, 10.0 + 0.01);
    ApplicationContainer apps = echoClientHelper.Install(nodes);
    apps.Start (Seconds (trafStar)); // App start somewhere at 10s !!
    apps.Stop (Seconds (simTime+1));
}


void CreateFlowsPairs(NodeContainer AP,NodeContainer STA, NetDeviceContainer APNET,NetDeviceContainer STANET, bool biDirection, int dataSize, short nNodePair)
{
  InternetStackHelper internet;
  internet.Install (NodeContainer(AP,STA));

  Ipv4InterfaceContainer interfaces;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces=ipv4.Assign(NetDeviceContainer(APNET,STANET));

  int port=80;
  PacketSinkHelper STASink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));

  for(int i=0;i<(nNodePair*2-1);i+=2)
  {
    Ipv4Address IPnode1,IPnode2; // Will be fill in the following for loop
    IPnode1=interfaces.GetAddress(i+1); // Note first interface == AP!!!
    IPnode2=interfaces.GetAddress(i+2);

    if(biDirection==0) {
      txApp(IPnode2,port,dataSize,STA.Get(i));
      Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node
    }
    else {
      txApp(IPnode2,port,dataSize,STA.Get(i));
      Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node

      txApp(IPnode1,port,dataSize,STA.Get(i+1));
      Sinks.Add(STASink.Install(STA.Get(i))); // Install sink on the first node
    }
  }
}

/**
 * @brief Setup position/mobility on a wifi cell
 *
 * Node are spread homogeneously in circle around the access point.
 *
 * @param STA Stations in the wifi cell
 * @param AP Access Point of the wifi cell
 * @param The Stations circle radius
 */
void SetupMobility(NodeContainer STA, NodeContainer AP, double radius){
  // Setup AP position (on the origin)
  MobilityHelper mobilityAP;
  Ptr<ListPositionAllocator> posAP = CreateObject <ListPositionAllocator>();
  posAP ->Add(Vector(0, 0, 0));
  mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
  mobilityAP.SetPositionAllocator(posAP);
  mobilityAP.Install(NodeContainer(AP));

  // Set station positions homogeneously in circle around the origin
  MobilityHelper mobilitySTA;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  double step=2*M_PI/STA.GetN();
  double theta=0;

  for(int i=0;i<STA.GetN();i++){
    double r = radius * sqrt(((double) rand() / (RAND_MAX)));
    double theta = ((double) rand() / (RAND_MAX)) * 2 * M_PI;
    positionAlloc ->Add(Vector(cos(theta)*r,sin(theta)*r, 0)); //
  }
  mobilitySTA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySTA.SetPositionAllocator(positionAlloc);
  mobilitySTA.Install(STA);
}

/**
 * @brief Create a Flows between stations and an ap
 * @param upDown true = STA->AP, false = ap->STA
 * @param nNodes
 */
void CreateFlowsStaAp(NodeContainer AP,NodeContainer STA, NetDeviceContainer APNET,NetDeviceContainer STANET, bool upDown, int dataSize, short nNodes)
{
  // Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (NodeContainer(AP,STA));
  Ipv4InterfaceContainer interfaces;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces=ipv4.Assign(NetDeviceContainer(APNET,STANET));

  Ipv4Address IPAP; // Will be fill in the following for loop
  IPAP=interfaces.GetAddress(0);
  int port=80;

  /* true = STA->AP*/
  if (upDown) {
    txApp(IPAP,port,dataSize,STA,true);
    PacketSinkHelper apSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
    Sinks.Add(apSink.Install(AP.Get(0))); // Install sink on AP
  } /* false = AP->STA*/
  else {
    for (int i=0; i < STA.GetN(); i++)
    {
      // create app on AP to send to sink on STA (downlink communication)
      txApp(interfaces.GetAddress(1+i),port, dataSize, AP.Get(0));
      PacketSinkHelper staSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
      Sinks.Add(staSink.Install(STA.Get(i))); // Install sink on AP
    }
  }
}

/**
 * @brief Setup a 802.11n stack on STA and AP.
 * @param STA The cell's stations
 * @param AP the cell's Acces Point
 * @return The wifi cell net devices
 */
WifiNetDev SetupWifi(NodeContainer STA, NodeContainer AP){
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
  WifiMacHelper wifiMac;
  Ssid ssid = Ssid ("net1");
  /* Configure STA */
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer sensorsNetDevices;
  sensorsNetDevices = wifiHelper.Install (/*wifiPhy*/wifiPhy, wifiMac, STA);
  /* Configure AP */
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apNetDevice;
  apNetDevice = wifiHelper.Install (/*wifiPhy*/wifiPhy, wifiMac, AP);

  if(pcapCapture){
    wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
    //wifiPhy.EnablePcap("AP_capture_s"+std::to_string(seed)+"_n"+std::to_string(nNode)+"_p"+std::to_string(nNodePair), AP,true);
  }
    return(std::make_pair(apNetDevice, sensorsNetDevices));
}


/**
 * @brief Build Topology 1
 * @param nNodePair Number of pair of nodes which will communicate together
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT1(short nNodePair, int biDirection, int dataSize, double radius)
{
  NS_LOG_UNCOND("Creating topology with flow pairs: "<<nNodePair<<" pairs, bidirectional="<<biDirection<<", dataSize="<<dataSize);

  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STA;
  STA.Create(2*nNodePair);

  NS_LOG_UNCOND("created nodes, call setupwifi");
  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  NS_LOG_UNCOND("setupwifi finished, call setupmobility");
  SetupMobility(STA, AP, radius);

  NS_LOG_UNCOND("Setup energy measurement");
  setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  NS_LOG_UNCOND("call setflowconf");
  CreateFlowsPairs(AP,STA,netDev.first,netDev.second,biDirection,dataSize,nNodePair);

}


/**
 * @brief Build Topology 2
 * @param nNodePair Number of nodes which will communicate together
 * @param flowConf if 1 unidirectional and if 2 bidirectional
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT2(short nNode, int upDown, int dataSize, double radius)
{
  NS_LOG_UNCOND("Creating topology with single STAs: "<<nNode<<" stations, upDown="<<upDown<<", dataSize="<<dataSize);

  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STA;
  STA.Create(nNode);

  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  SetupMobility(STA, AP,radius);
  setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  if(upDown == 2) // updown==2 when no flows are created
  {} else {
    CreateFlowsStaAp(AP,STA,netDev.first,netDev.second,upDown, dataSize, STA.GetN());
  }
}


int main(int argc, char* argv[])
{
  pcapCapture = false;
  mcs = "HtMcs3";
  simTime = 60;
  threshRTSCTS = 100;
  int topo = 1;
  double radius = 15;
  int dataSize = 1000;
  int nNodePair = 1;
  int nNode = 1;
  int upDown=1, biDirection=1;

  CommandLine cmd;
  cmd.AddValue ("topo", "Which topology to use: 1=pairs of STAs, 2=single STAs", topo);
  cmd.AddValue ("dataSize", "Packet data size sended by senders", dataSize);
  cmd.AddValue ("nNodePair", "Number of sender/receiver pairs", nNodePair);
  cmd.AddValue ("nNode", "Number of node", nNode);
  cmd.AddValue ("radius", "Radius between STA and AP", radius);
  cmd.AddValue ("threshRTSCTS", "Set the RTS threshold", threshRTSCTS);
  cmd.AddValue ("seed", "Simulator seed", seed);
  cmd.AddValue ("pcap", "Generate PCAP", pcapCapture);
  cmd.AddValue ("simTime", "Communication duration", simTime);
  cmd.AddValue ("mcs", "MCS to use for data and control", mcs);
  cmd.AddValue ("upDown", "topo 1 only: 2=no flows, 1=STA->AP, 0=AP->STA", upDown);
  cmd.AddValue ("biDirection", "topo 2 only: 1=flows in STA1<->STA2, 0=flows in STA1->STA2 for each pair", biDirection);
  cmd.AddValue ("cancelIDLEEnergy", "account or not for the idle energy consumption", cancelIDLEEnergy);

  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(threshRTSCTS)); // Enable RTS/CTS

  ns3::RngSeedManager::SetSeed (seed);
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1500));

  // ---------- Setup Simulations ----------
  if(topo==1)
    buildT1(nNodePair,biDirection,dataSize,radius);
  else if(topo==2)
    buildT2(nNode,upDown,dataSize,radius);


  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (false));

  // callbacks
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback(&MacRxDropCallback));
  if(upDown!=2) // no sink in case of config 2 and single STAs
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRx));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/State/State", MakeCallback(&PhyStateTrace));

  // Setup routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Enable flow monitoring
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  // Un comment to save flows into xml
  // monitor->SerializeToXmlFile("NameOfFile.xml", true, true);

  // Run Simulations
  Simulator::Stop (Seconds (simTime));

  Simulator::Schedule (Seconds (10.0), &PrintThroughput);

  Simulator::Run ();

  // log stats
  NS_LOG_UNCOND("Time= "<<Simulator::GetImplementation()->Now().GetSeconds()<<" seconds");
  long totalRx=0;
  int nbSinkTot=0;
  int i=0;

  for(ApplicationContainer::Iterator n = Sinks.Begin (); n != Sinks.End (); n++)
  {
    ns3::Ptr<Application> app=*n;
    PacketSink& ps=dynamic_cast<PacketSink&>(*app);
    NS_LOG_UNCOND("Sink "<< i << " received " << ps.GetTotalRx() << " bytes");
    i++;
    totalRx+=ps.GetTotalRx();
    nbSinkTot++;
  }
  NS_LOG_UNCOND("Sink amount: " << Sinks.GetN());
  NS_LOG_UNCOND("Average throughput: " << (totalRx*8)/((TimeLastRxPacket-10)));

  double totEnergy = 0;

  DeviceEnergyModelContainer::Iterator energyIt = consumedEnergyAP.Begin();
  while(energyIt!= consumedEnergyAP.End()) {
    NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (AP)");
    totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
    energyIt++;
  }
  energyIt = consumedEnergySTA.Begin();
  while(energyIt!= consumedEnergySTA.End()) {
    NS_LOG_UNCOND("Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " J (STA)");
    totEnergy+=(*energyIt)->GetTotalEnergyConsumption();
    energyIt++;
  }

  std::cout << "Tot energy: "<<totEnergy<< std::endl;

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
  // Finish
  Simulator::Destroy ();
  return 0;
}