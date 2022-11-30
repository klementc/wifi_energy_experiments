#ifndef MODULES_HPP
#define MODULES_HPP

#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/ssid.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/wifi-standards.h"
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/rng-seed-manager.h"

using namespace ns3;

typedef std::pair<ns3::NetDeviceContainer,ns3::NetDeviceContainer> WifiNetDev;

extern std::map<int, std::pair<int,std::vector<float>>> nodeTimers;

// for measurements
extern float TimeLastRxPacket;
extern long APRx;
extern long MacTxDrop,PhyTxDrop;
extern long MacRxDrop,PhyRxDrop;


void PhyStateTrace(std::string context, Time start, Time duration, enum WifiPhyState state);

void PacketSinkRx(/*std::string path, */Ptr<const Packet> p, const Address &address);

void MacTxDropCallback(std::string context, Ptr<const Packet> p);

void MacRxDropCallback(std::string context, Ptr<const Packet> p);

void PhyTxDropCallback(std::string context,Ptr<const Packet> p);

void PhyRxDropCallback(std::string context,Ptr<const Packet> p,WifiPhyRxfailureReason r);

void PrintThroughput();

#endif