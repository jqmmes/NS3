#ifndef TDLS_MANAGER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include <mutex>
#include <climits>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>


struct TdlsIP
{
	ns3::Ipv4Address address = ns3::Ipv4Address("0.0.0.0");
	bool status = false;
};

struct TDLS
{
	TdlsIP iface0;
	TdlsIP iface1;
	TdlsIP iface2;
};



class TdlsManager
{
public:
  
  TdlsManager();
  virtual ~TdlsManager();
  void AddNode(ns3::Ptr<ns3::Node>, ns3::Ipv4Address iface0 = ns3::Ipv4Address("0.0.0.0") , ns3::Ipv4Address iface1 = ns3::Ipv4Address("0.0.0.0"), ns3::Ipv4Address iface2 = ns3::Ipv4Address("0.0.0.0"));

 //  void add(Ipv4Address ip, uint16_t port);
 //  void remove(Ipv4Address ip, uint16_t port);
 //  tuple<Ipv4Address,uint16_t> discover(void);
  
 //  vector<tuple<Ipv4Address,uint16_t>> getAll(void);
 //  int Random(int i);

	// uint32_t GetN(void);

private:
	uint32_t tlds_cons = 0; // Active TDLS connections
	uint32_t max_tdls = 12; // Maximum TDLS connections
	std::map<ns3::Ptr<ns3::Node>, TDLS> network;
};

#endif