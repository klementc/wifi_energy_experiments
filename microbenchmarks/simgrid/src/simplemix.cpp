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
#include <fstream>
#include <boost/algorithm/string.hpp>

#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[wifi_usage] 1STA-1LINK-1NODE-CT");

size_t SEND_SIZE = 5500000.*150;
double simTime = 60;
double START_TIME = 10;

void setup_simulation();
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
  SEND_SIZE =std::stoi(argv[2]);
  simTime = std::stoi(argv[3]);

  XBT_INFO("dataSize=%d simTime=%d",SEND_SIZE, simTime);

  // Setup/Run simulation
  setup_simulation();


  simgrid::s4u::Actor::create("stopper", engine.get_all_hosts().at(0),
                              stoper, simTime);
  engine.run();
  XBT_INFO("Simulation took %fs", simgrid::s4u::Engine::get_clock());

  return (0);
}

void setup_simulation() {
  auto dataSize=SEND_SIZE;
  std::vector<std::string> noArgs;
  noArgs.push_back(std::to_string(dataSize));

  int nHost= sg_host_count();
  XBT_INFO("%d hosts on the platform", nHost);

  for(int i=0; i<nHost/2; i++){

    int src = i;
    int dst = i+(nHost/2);

    std::ostringstream ss;
    ss<<"STA"<<src;
    std::string STA_A=ss.str();
    ss.str("");
    ss<<"STA"<<dst;
    std::string STA_B=ss.str();
    ss.str("");
    ss<<"AP"<<1+((src)/(nHost/2));
    std::string AP_s=ss.str();

    // flow A->B
    std::vector<std::string> args_STA_A;
    args_STA_A.push_back(STA_B);
    args_STA_A.push_back(std::to_string(dataSize));

    simgrid::s4u::Actor::create(STA_A, simgrid::s4u::Host::by_name(STA_A),
                                send, args_STA_A);
    simgrid::s4u::Actor::create(STA_B, simgrid::s4u::Host::by_name(STA_B),
                                rcv, noArgs);
  }

  for (int i=0;i<nHost;i++) {
        std::ostringstream ss;
        ss<<"STA"<<i;
        std::string STA_A=ss.str();
        ss.str("");
        ss<<"AP"<<1+((i)/(nHost/2));
        std::string AP_s=ss.str();

        XBT_INFO("coucou set rate %s %s %d %d\n", STA_A.c_str(), AP_s.c_str(), i, nHost);
        set_rate(AP_s,STA_A,0);
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

  int dataSize = std::atoi(args.at(1).c_str());

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
