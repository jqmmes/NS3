#ifndef BTINTERFACE_H
#define BTINTERFACE_H


#include "VirtualDiscovery.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

using namespace std;
using namespace ns3;

typedef uint32_t BTMac; // Index in NodeList for now

class BTInterface
{
public:
  
  BTInterface(Ptr<Node> node, VirtualDiscovery * discovery);
  virtual ~BTInterface(void);

  void turnOn(void);
  void turnOff(void);
  void setDiscoverable(bool choice);
  vector<BTMac> Discover(void);
  BTMac getMac(void);
  void Send(string data, BTMac dest);
  vector<BTMac> getMyPiconet(void);
  vector<vector<BTMac>> getMyPiconets(void);

private:
	vector<tuple<BTMac, uint16_t>> searchArea(void);
	bool isReachable(BTMac mac);
	uint16_t getDistTo(BTMac mac);
	

	Ptr<Node> m_node;
	BTMac m_mac;
	bool m_discoverable = false;
	bool m_isOn = false;
	uint16_t m_max_range = 15; //15m max range
	uint8_t m_max_neighours = 6; // 6 slaves + 1 master
	string m_max_datarate = "3Mbps"; // Max transmission speed 3Mb/s



};

#endif