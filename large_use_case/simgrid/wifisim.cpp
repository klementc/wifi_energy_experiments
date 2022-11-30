//
// Created by clement on 20/02/2020.
//


#include "simgrid/s4u.hpp"
#include "xbt/log.h"
#include "simgrid/msg.h"
#include "simgrid/s4u/Activity.hpp"
#include "src/surf/network_cm02.hpp"
/*

#include "src/kernel/activity/CommImpl.hpp"
#include "simgrid/s4u/Link.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/network_wifi.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
*/
#include "simgrid/s4u.hpp"
#include "simgrid/engine.h"
#include "src/kernel/resource/WifiLinkImpl.hpp"
#include "simgrid/plugins/energy.h"

#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <fstream>
#include <boost/algorithm/string.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(experience, "Wifi exp");

// used to have the same duration of the experiment between ns3 and simgrid
double DELAYMAX;
double totData;

std::vector<std::pair<simgrid::s4u::CommPtr,size_t>> recPtr;

// associative array keeping size of data already sent for a given flow (required for interleaved actions)
std::map<simgrid::kernel::resource::WifiLinkAction*, std::pair<int, double>> flowTmp{};
// keep track of data sent in the current time slot
double overallData = 0;
double prev_update = 0;


static void sender(std::vector<std::string> args);
static void receiver(std::vector<std::string> args);
static void monitor();
static void energyMon();
static void killer ();
void updateAllLinksTh();
void updateThroughput(const simgrid::kernel::resource::NetworkAction&);


int main(int argc, char** argv)
{

  // engine
  simgrid::s4u::Engine engine(&argc, argv);

  if(argv[1] == "--help" || argc < 4) {
    std::cout << "Usage: "<<argv[0] << " <platform-file> <comm-file> <durationMax>"<<std::endl;
    return 1;
  }

  DELAYMAX = std::stod(argv[3]);

  engine.register_function("sender", &sender);
  engine.register_function("receiver", &receiver);

  engine.load_platform(argv[1]);

  XBT_INFO("Launching the experiment");

  std::ifstream stream(argv[2]);

  std::string line;
  while(std::getline(stream, line)){
    // parse line
    std::vector<std::string> fields;
    boost::algorithm::split(fields, line, boost::is_any_of(","));
    std::vector<std::string> args;

    std::string src = fields[0];
    std::string dst = fields[1];
    int size = std::stoi(fields[2]);
    double start = std::stod(fields[3]);

    // set bandwidth on station
    std::stringstream apName;
    apName << "AP_" << simgrid::s4u::Host::by_name("STA_"+src)->get_englobing_zone()->get_cname();
    simgrid::kernel::resource::WifiLinkImpl* l= (simgrid::kernel::resource::WifiLinkImpl *)simgrid::s4u::Link::by_name(apName.str())->get_impl();
    l->set_host_rate(simgrid::s4u::Host::by_name("STA_"+src), 0);


    // create actors
    XBT_INFO("%s -> %s : %dB starting at %lf s", src.c_str(), dst.c_str(), size, start);
    args.push_back(src+"_mb"); // mailbox name (unique name to not interfere)
    args.push_back(fields[2]);
    args.push_back(fields[3]);
    simgrid::s4u::Actor::create("SND:"+src+"-"+dst, simgrid::s4u::Host::by_name("STA_"+src), &sender, args);
    std::vector<std::string> argsRcv;
    argsRcv.push_back(src+"_mb"); // receiver's mailbox (unique name to not interfere)
    simgrid::s4u::Actor::create("RCV:"+src+"-"+dst, simgrid::s4u::Host::by_name(std::string("OUT")), &receiver, argsRcv);
  }
  simgrid::s4u::Link::on_communication_state_change_cb([](simgrid::kernel::resource::NetworkAction const& action,
         simgrid::kernel::resource::Action::State /* previous */) {
           updateThroughput(action);}
);
  /*simgrid::s4u::Link::on_communicate_cb.connect([](const simgrid::kernel::resource::NetworkAction& action) {
           updateThroughput(action);}
);*/
  simgrid::s4u::Actor::create("killer", simgrid::s4u::Host::by_name(std::string("OUT")), &killer);
  simgrid::s4u::Actor::create("monitor", simgrid::s4u::Host::by_name(std::string("OUT")), &monitor);
  simgrid::s4u::Actor::create("monirotEn",simgrid::s4u::Host::by_name(std::string("OUT")), &energyMon);
  engine.run();
  return 0;
}

static void killer () {
  simgrid::s4u::this_actor::sleep_until(DELAYMAX);
  XBT_INFO("killing everyone");
  simgrid::s4u::Actor::kill_all();
}

static void energyMon() {
  int sec = 0;
  while(true) {
    int secPrev = sec;
    sec+=10;
    simgrid::s4u::this_actor::sleep_until(sec);
    double enTot = 0;
    auto linkList = simgrid::s4u::Engine::get_instance()->get_all_links();
    for(auto link : linkList) {
      if(link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI) {
        enTot += sg_wifi_link_get_consumed_energy(link);
      }
    }
    XBT_INFO("Tot energy at %lf : %lf J", simgrid::s4u::Engine::get_clock(),enTot);
  //sg_wifi_link_get_consumed_energy()
  }
}

static void monitor () {
  int sec = 0;
  while(true) {
    int secPrev = sec;
    sec+=10;
    simgrid::s4u::this_actor::sleep_until(sec);
    size_t tot=0;
    for (auto c : recPtr) {
      //c.first->get_dst_data_size();
      //c.first->get_remaining();
      //XBT_INFO("tot: %lf rem: %lf, alr: %lf", c.first->get_dst_data_size(),c.first->get_remaining(),c.second);
      //tot+=c.first->get_dst_data_size() - c.first->get_remaining()- c.second;
      //c.second = c.first->get_dst_data_size() - c.first->get_remaining();
    }
    XBT_INFO("Totdata between %d and %d s: %lf bytes", secPrev, sec, totData); //tot);//totData);
    totData=0; // recompute throughput over the next time slot
    updateAllLinksTh();
    XBT_INFO("Totdatav2 between %d and %d s: %lf bytes", secPrev, sec, overallData);

    overallData = 0;
  }
}

static void sender(std::vector<std::string> args)
{
  simgrid::s4u::Actor::self()->set_kill_time(DELAYMAX);
  simgrid::s4u::Mailbox* dst = simgrid::s4u::Mailbox::by_name(args.at(0));
  int size = atoi(args.at(1).c_str());
  double start = std::stod(args.at(2).c_str());

  simgrid::s4u::this_actor::sleep_until(start);
  XBT_INFO("SENDING msg of size %d to %s, start at %f", size, args.at(0).c_str(), start);
  static std::string message = "message from "+args.at(0);
  auto comm = dst->put_async(&message, size);
  recPtr.push_back(std::make_pair(comm, 0));
  comm->wait();
  XBT_INFO("finished task  src: %s dst: %s start: %lf end: %lf size: %d",
    simgrid::s4u::this_actor::get_host()->get_cname(), "OUT", start, simgrid::s4u::Engine::get_clock(),size );
  totData += size;
}

static void receiver(std::vector<std::string> args)
{
  simgrid::s4u::Actor::self()->set_kill_time(DELAYMAX);
  XBT_INFO("RECEIVING");

  simgrid::s4u::Mailbox* mBox = simgrid::s4u::Mailbox::by_name(args.at(0));
  std::string s;
  mBox->get<std::string>();

  XBT_INFO("Received message at time %f", simgrid::s4u::Engine::get_clock());
}

void updateThroughput(const simgrid::kernel::resource::NetworkAction&)
{
  updateAllLinksTh();
}

/*compute data sent on updates, heavily inspired from the wifi energy plugin*/
void updateAllLinksTh() {
  auto* eng = simgrid::s4u::Engine::get_instance();
  double duration = eng->get_clock() - prev_update;
  prev_update    = eng->get_clock();

  XBT_INFO("update at %lf", eng->get_clock());
  // we don't update for null durations
  if(duration < 1e-6)
    return;

  // loop through all wifi links
  for (auto const* link : eng->get_all_links()){
    // check if it is a wifi link
    const simgrid::kernel::resource::WifiLinkImpl* wifi_link = nullptr;
    if(link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI) {
      wifi_link = static_cast<simgrid::kernel::resource::WifiLinkImpl*>(link->get_impl());
    }

    // skip if not wifi link
    if (wifi_link == nullptr)
      continue;

    // loop over all actions of the link and update the throughputs
    const simgrid::kernel::lmm::Element* elem = nullptr;
    while (const auto* var = wifi_link->get_constraint()->get_variable(&elem)) {
      auto* action = static_cast<simgrid::kernel::resource::WifiLinkAction*>(var->get_id());
      XBT_DEBUG("cost: %f action value: %f link rate 1: %f link rate 2: %f", action->get_cost(), action->get_rate(),
                wifi_link->get_host_rate(&action->get_src()), wifi_link->get_host_rate(&action->get_dst()));

      if (action->get_rate() != 0.0) {
        auto it = flowTmp.find(action);

        // if the flow has not been registered, initialize it: 0 bytes sent, and not updated since its creation timestamp
        if(it == flowTmp.end())
          flowTmp[action] = std::pair<int,double>(0, action->get_start_time());

        it = flowTmp.find(action);

        /**
         * The active duration of the link is equal to the amount of data it had to send divided by the bandwidth on the link.
         * If this is longer than the duration since the previous update, active duration = now - previous_update
         */
        double du = // durUsage on the current flow
            (action->get_cost() - it->second.first) / action->get_rate();

        if(du > eng->get_clock()-it->second.second)
          du = eng->get_clock()-it->second.second;

        // update the amount of data already sent by the flow
        it->second.first += du * action->get_rate();
        it->second.second =  eng->get_clock();

        //XBT_INFO("!!!!!!!!!!!!!!!!!!!!!!!! %lf , %lf, %lf",du,action->get_rate(),overallData);
        // update overall counter for monitor
        overallData += du * action->get_rate();

        // important: if the transmission finished, remove it (needed for performance and multi-message flows)
        /*if(it->second.first >= action->get_cost())
          flowTmp.erase (it);*/
      }
    }
  }
}