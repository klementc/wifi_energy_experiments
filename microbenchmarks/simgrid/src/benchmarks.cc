/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/log.h"
#include "simgrid/msg.h"
#include "src/surf/network_cm02.hpp"
//#include "src/surf/network_wifi.hpp"
//#include "src/kernel/ressource/WifiLinkImpl.hpp"
#include <simgrid/s4u/Link.hpp>
#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[wifi_usage] 1STA-1LINK-1NODE-CT");

size_t SEND_SIZE = 5500000.*150;
double simTime = 60;
double START_TIME = 10;

void setup_simulation(short topo, int upDown, int biDirection);
/**
 * @brief Function used as a sending actor
 */
static void send(std::vector<std::string> args);
/**
 * @brief Function used as a receiving actor
 */
static void rcv(std::vector<std::string> args);
/**
 * @brief Set rate of a station on a wifi link
 * @param ap_name Name of the access point (WIFI link)
 * @param node_name Name of the station you want to set the rate
 * @param rate The rate id
 */
void set_rate(std::string ap_name, std::string node_name, short rate);


std::vector<simgrid::s4u::CommPtr> recPtr;
std::vector<simgrid::s4u::CommPtr> sndPtr;
void stoper(double timeToStop) {

  simgrid::s4u::this_actor::sleep_until(timeToStop);

  for (simgrid::s4u::CommPtr v : recPtr){
    XBT_INFO("Sent: %lf Received: %lf", (SEND_SIZE-v->get_remaining()));
  }
  simgrid::s4u::Actor::kill_all();
}

/**
 * Usage ./simulator <platformFile> <topo> <dataSize> <flowConf> <useDecayModel>
 * All arguments are mandatory.
 */
int main(int argc, char **argv) {

  // Build engine
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);

  // Parse arguments
  short topo=std::stoi(argv[2]);
  SEND_SIZE =std::stoi(argv[3]);
  simTime = std::stoi(argv[4]);
  int upDown = std::stoi(argv[5]);
  int biDirection = std::stoi(argv[6]);

  XBT_INFO("topo=%d dataSize=%d upDown=%d biDirection=%d",topo,SEND_SIZE,upDown, biDirection);

  // Setup/Run simulation
  setup_simulation(topo,upDown, biDirection);


  simgrid::s4u::Actor::create("stopper", engine.get_all_hosts().at(0),
                              stoper, simTime);
  engine.run();
  XBT_INFO("Simulation took %fs", simgrid::s4u::Engine::get_clock());

  return (0);
}

void setup_simulation(short topo, int upDown, int biDirection) {
  auto dataSize=SEND_SIZE;
  std::vector<std::string> noArgs;
  noArgs.push_back(std::to_string(dataSize));
  // topology with node pairs
  if(topo==1){
    int nHost= sg_host_count()-1; // remove outnode (and router if in ns3)
    XBT_INFO("%d hosts on the platform", nHost);
    for(int i=0;i<nHost;i+=2){
      std::ostringstream ss;
      ss<<"STA"<<i;
      std::string STA_A=ss.str();
      ss.str("");
      ss<<"STA"<<i+1;
      std::string STA_B=ss.str();

      // flow A->B
      if(biDirection == 0) {
        std::vector<std::string> args_STA_A;
        args_STA_A.push_back(STA_B);
        args_STA_A.push_back(std::to_string(dataSize));

        simgrid::s4u::Actor::create(STA_A, simgrid::s4u::Host::by_name(STA_A),
                                    send, args_STA_A);
        simgrid::s4u::Actor::create(STA_B, simgrid::s4u::Host::by_name(STA_B),
                                    rcv, noArgs);
      }
      // flow A<->B
      else if(biDirection==1) {
        // --------------------------- 1
        // Send to B
        std::vector<std::string> args_STA_A;
        args_STA_A.push_back(STA_B);
        args_STA_A.push_back(std::to_string(dataSize));
        simgrid::s4u::Actor::create(STA_A+"Send", simgrid::s4u::Host::by_name(STA_A),
                                    send, args_STA_A);
        // Receive from A
        simgrid::s4u::Actor::create(STA_B, simgrid::s4u::Host::by_name(STA_B),
                                    rcv, noArgs);
        // --------------------------- 2
        // Send to A
        std::vector<std::string> args_STA_B;
        args_STA_B.push_back(STA_A);
        args_STA_B.push_back(std::to_string(dataSize));
        simgrid::s4u::Actor::create(STA_B+"Send", simgrid::s4u::Host::by_name(STA_B),
                                    send, args_STA_B);
        // Receive from B
        simgrid::s4u::Actor::create(STA_A, simgrid::s4u::Host::by_name(STA_A),
                                    rcv, noArgs);
      }
        set_rate("AP1",STA_A,0);
        set_rate("AP1",STA_B,0);
    }
  }
  else if(topo==2){
    int nHost= sg_host_count(); // remove router (which is a host) if using ns3
    XBT_INFO("%d hosts", nHost);

    for(int i=0;i<nHost-1;i+=1){ // -1 because of the AP
      // no flows created, to measure only beacons
      if(upDown == 2) {
        std::ostringstream ss;
        ss<<"STA"<<i;
        std::string STA=ss.str();
        std::vector<std::string> args_STA;
        args_STA.push_back("outNode");
        args_STA.push_back(std::to_string(0));

        simgrid::s4u::Actor::create(STA, simgrid::s4u::Host::by_name(STA),
                                    send, args_STA);
        set_rate("AP1",STA,0);
        // One receiver for each flow
        simgrid::s4u::Actor::create("outNode", simgrid::s4u::Host::by_name("outNode"), rcv, noArgs);
      } else if (upDown == 1) {
        std::ostringstream ss;
        ss<<"STA"<<i;
        std::string STA=ss.str();
        std::vector<std::string> args_STA;
        args_STA.push_back("outNode");
        args_STA.push_back(std::to_string(dataSize));

        simgrid::s4u::Actor::create(STA, simgrid::s4u::Host::by_name(STA),
                                    send, args_STA);
        set_rate("AP1",STA,0);
        // One receiver for each flow
        simgrid::s4u::Actor::create("outNode", simgrid::s4u::Host::by_name("outNode"), rcv, noArgs);
      } else if (upDown == 0) {
        std::ostringstream ss;
        ss<<"STA"<<i;
        std::string STA=ss.str();
        std::vector<std::string> args_STA;
        args_STA.push_back(STA);
        args_STA.push_back(std::to_string(dataSize));

        simgrid::s4u::Actor::create("outNode", simgrid::s4u::Host::by_name("outNode"), send, args_STA);
        set_rate("AP1",STA,0);
        // One receiver for each flow
        simgrid::s4u::Actor::create(STA, simgrid::s4u::Host::by_name(STA), rcv, noArgs);
      }
    }
  }
}

void set_rate(std::string ap_name, std::string node_name, short rate) {
  auto *l =
      simgrid::s4u::Link::by_name(
          ap_name)
          ->get_impl();


  simgrid::s4u::Link *ll = simgrid::s4u::Link::by_name(ap_name);
  ll->set_host_wifi_rate(simgrid::s4u::Host::by_name(node_name), rate);

  /*if(useDecayModel){
    bool enabled=l->toggle_decay_model();
    if(!enabled)
      l->toggle_decay_model();
  }*/
}

static void rcv(std::vector<std::string> args) {
  int dataSize = std::atoi(args.at(0).c_str());
  std::string selfName = simgrid::s4u::this_actor::get_host()->get_name();
  simgrid::s4u::Mailbox *selfMailbox = simgrid::s4u::Mailbox::by_name(
      simgrid::s4u::this_actor::get_host()->get_name());


      std::string* msg_content;
      recPtr.push_back(selfMailbox->get_async(&msg_content));

       //XBT_INFO("Init %f %f", recPtr.at(recPtr.size()-1)->get_dst_data_size(), recPtr.at(recPtr.size()-1)->get_remaining());
      recPtr.at(recPtr.size()-1)->wait();

      //selfMailbox->get();

      /*double receivedTime = simgrid::s4u::Engine::get_clock();
      XBT_INFO("Sending check: %s received %d bytes at %f",
               selfName.c_str(), dataSize, receivedTime);*/
}

static void send(std::vector<std::string> args) {
  std::string selfName = simgrid::s4u::this_actor::get_host()->get_name();
  simgrid::s4u::Mailbox *selfMailbox = simgrid::s4u::Mailbox::by_name(
      simgrid::s4u::this_actor::get_host()->get_name());

  simgrid::s4u::Mailbox *dstMailbox =
    simgrid::s4u::Mailbox::by_name(args.at(0));

  int dataSize = SEND_SIZE;

  simgrid::s4u::this_actor::sleep_until(START_TIME);

  double comStartTime = simgrid::s4u::Engine::get_clock();


  std::string msg_content = std::string("Message ");
  auto* payload = new std::string(msg_content);

  sndPtr.push_back(dstMailbox->put_async(payload, dataSize));
  sndPtr.at(sndPtr.size()-1)->wait();

  double comEndTime = simgrid::s4u::Engine::get_clock();
  XBT_INFO("%s sent %d bytes to %s in %f seconds from %f to %f",
           selfName.c_str(), dataSize, args.at(0).c_str(),
           comEndTime - comStartTime, comStartTime, comEndTime);
}
