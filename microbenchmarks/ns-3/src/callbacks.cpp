#include "modules.hpp"

using namespace ns3;

std::map<int, std::pair<int,std::vector<float>>> nodeTimers;

// for measurements
float TimeLastRxPacket=-1;
long APRx=0;
long MacTxDrop=0,PhyTxDrop=0;
long MacRxDrop=0,PhyRxDrop=0;


// https://stackoverflow.com/questions/2727749/c-split-string
std::vector<std::string> split(std::string text, std::string split_word) {
    std::vector<std::string> list;
    std::string word = "";
    int is_word_over = 0;

    for (int i = 0; i <= text.length(); i++) {
        if (i <= text.length() - split_word.length()) {
            if (text.substr(i, split_word.length()) == split_word) {
                list.insert(list.end(), word);
                word = "";
                is_word_over = 1;
            }
            //now we want that it jumps the rest of the split character
            else if (is_word_over >= 1) {
                if (is_word_over != split_word.length()) {
                    is_word_over += 1;
                    continue;
                }
                else {
                    word += text[i];
                    is_word_over = 0;
                }
            }
            else {
                word += text[i];
            }
        }
        else {
            word += text[i];
        }
    }
    list.insert(list.end(), word);
    return list;
}


void PhyStateTrace(std::string context, Time start, Time duration, enum WifiPhyState state)
{
  std::vector< std::string> sep = split(context, "/");
  float timeSecond = start.GetDouble()/1000000000;
  int idNode =  atoi(sep[2].c_str());

  if(nodeTimers.find(idNode) == nodeTimers.end()) {
    nodeTimers[idNode] = std::make_pair<int,std::vector<float>>(state, {timeSecond, 0,0,0,0,0,0,0});
  }
  else {
      switch (nodeTimers[idNode].first)
      {
      case IDLE:
        // m_idleStateFile << idNode << "," << lastUpdateStatus[idNode] << "," << timeSecond << std::endl;
        nodeTimers[idNode].second[1] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case RX:
        //m_rxStateFile << idNode << "," << lastUpdateStatus[idNode] << "," << timeSecond << std::endl;
        nodeTimers[idNode].second[2] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case TX:
        // m_txStateFile << idNode << "," << lastUpdateStatus[idNode] << "," << timeSecond << std::endl;
        nodeTimers[idNode].second[3] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case CCA_BUSY:
        // m_ccaStateFile << idNode << "," << lastUpdateStatus[idNode] << "," << timeSecond << std::endl;
        nodeTimers[idNode].second[4] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case SLEEP:
        //std::cout << context << " SLEEP " << std::endl;
        nodeTimers[idNode].second[5] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case SWITCHING:
        //std::cout << context << " SWITCHING " << std::endl;
        nodeTimers[idNode].second[6] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      case OFF:
        //std::cout << context << " OFF " << std::endl;
        nodeTimers[idNode].second[7] += timeSecond - nodeTimers[idNode].second[0];
        nodeTimers[idNode].second[0] = timeSecond;
        nodeTimers[idNode].first = state;
        break;
      default:
        //std::cout << context << "VALEUR ETAT " << nodeState[idNode] << " DEFAULT " << std::endl;
        break;
    }
  }
}


void PacketSinkRx(/*std::string path, */Ptr<const Packet> p, const Address &address)
{
	float simTime=Simulator::Now().GetSeconds();
	if(TimeLastRxPacket<simTime)
		TimeLastRxPacket=simTime;
	APRx+=p->GetSize();
}

void MacTxDropCallback(std::string context, Ptr<const Packet> p)
{
	MacTxDrop++;
}
void MacRxDropCallback(std::string context, Ptr<const Packet> p)
{
	MacRxDrop++;
}
void PhyTxDropCallback(std::string context,Ptr<const Packet> p)
{
	PhyTxDrop++;
}
void PhyRxDropCallback(std::string context,Ptr<const Packet> p,WifiPhyRxfailureReason r)
{
	PhyRxDrop++;
}

void PrintThroughput(){
	uint32_t simTime=Simulator::Now().GetSeconds();
  double throughput=APRx*8;
  NS_LOG_UNCOND("Overall APs throughput at " << simTime <<"s: " << throughput <<"bps");
  APRx=0;
	Simulator::Schedule (Seconds (1.0), &PrintThroughput);
}