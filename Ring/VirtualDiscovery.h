#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include <mutex>
#include <climits>

using namespace ns3;
using namespace std;

class VirtualDiscovery
{
public:
  
  VirtualDiscovery(Ptr<UniformRandomVariable> rand);
  virtual ~VirtualDiscovery();

  void add(Ipv4Address ip, uint16_t port);
  void remove(Ipv4Address ip, uint16_t port);
  tuple<Ipv4Address,uint16_t> discover(void);
  
  vector<tuple<Ipv4Address,uint16_t>> getAll(void);


	uint32_t GetN(void);

private:
	vector<tuple<Ipv4Address,uint16_t>> m_ips;
	mutex i_mutex;
	mutex o_mutex;

  Ptr<UniformRandomVariable> randomGen;
};

#endif