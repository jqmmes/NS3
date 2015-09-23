#include "BTInterface.h"

NS_LOG_COMPONENT_DEFINE ("BTInterface");

BTInterface::BTInterface(Ptr<Node> node, VirtualDiscovery * discovery)
{
	m_node = node;
  srand(time(0));
  NS_LOG_DEBUG("Init BTInterface");
}

BTInterface::~BTInterface(void)
{
  NS_LOG_DEBUG("Destroy BTInterface"); 
}

void BTInterface::turnOn(void){
	m_isOn = true;
}

void BTInterface::turnOff(void){
	m_isOn = false;
}

void BTInterface::setDiscoverable(bool choice){
	m_discoverable = choice;
}

vector<BTMac> BTInterface::Discover(void){
	uint32_t mac, nnods =  NodeList::GetNNodes();
	uint16_t dist;
	TimeValue start, stop;
	vector<BTMac> nl;
	UintegerValue myId;
	m_node->GetAttribute("Id", myId);
	for(mac = 1; mac < nnods; mac++){ // Starting on 1, as the first node is allways a server without BT
		if (myId.Get() != mac){
			NodeList::GetNode(mac)->GetApplication(0)->GetAttribute("StartTime", start);
			NodeList::GetNode(mac)->GetApplication(0)->GetAttribute("StopTime", stop);
			if (start.Get() <=  Simulator::Now() && Simulator::Now() <= stop.Get()){
				if (isReachable(mac)){
					nl.emplace(nl.end(), mac);
				}
			}
		}
	}
	return nl;
}


void BTInterface::Send(string data, BTMac dest){

}

vector<tuple<BTMac, uint16_t>> BTInterface::searchArea(void){
	uint32_t mac, nnods =  NodeList::GetNNodes();
	vector<tuple<BTMac, uint16_t>> nl;
	for(mac = 1; mac < nnods; mac++){
		nl.emplace(nl.end(), make_tuple(mac, isReachable(mac)));
	}
	return nl;
}

bool BTInterface::isReachable(BTMac mac){
	return (getDistTo(mac) < m_max_range);
}

uint16_t BTInterface::getDistTo(BTMac mac){
	Ptr<MobilityModel> mob = m_node->GetObject<MobilityModel>();
	Ptr<MobilityModel> other = NodeList::GetNode(mac)->GetObject<MobilityModel>();
	return (uint16_t)floor(mob->GetDistanceFrom(other)); // Aprox. Distance
}



BTMac BTInterface::getMac(void){
	return m_mac;
}

