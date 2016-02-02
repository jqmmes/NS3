#ifndef TDLS_MANAGER_H
#define TDLS_MANAGER_H

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
#include <array>


struct TdlsIP
{
	ns3::Ipv4Address address = ns3::Ipv4Address("0.0.0.0");
	bool status = false;
};

enum status {run, stopped};
enum iface {iface0, iface1, ap};
enum connection {regular, tdls};

class TdlsStatus{
public:
	status m_status;
	iface m_iface;
	connection m_connection;
};

class TdlsManager
{
public:
  
  TdlsManager();
  TdlsManager(uint32_t);
  virtual ~TdlsManager();
  void AddNode(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, ns3::Ipv4Address iface0, ns3::Ipv4Address iface1, iface iface, bool tdls);
  ns3::Ipv4Address RequestIP(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress);
  //void UpdateStatusDoneTDLS(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress); // Ao acabar o tdls marcar-se como feito.
  void UpdateStatusDone(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, bool using_tdls=false); // Ao acabar sem tdls marcar-se como feito.



 //  void add(Ipv4Address ip, uint16_t port);
 //  void remove(Ipv4Address ip, uint16_t port);
 //  tuple<Ipv4Address,uint16_t> discover(void);
  
 //  vector<tuple<Ipv4Address,uint16_t>> getAll(void);
 //  int Random(int i);

	// uint32_t GetN(void);

private:
	ns3::Ptr<ns3::UniformRandomVariable> randomGen = ns3::CreateObject<ns3::UniformRandomVariable>();
	
	uint32_t m_n_servers = 0;
	//uint32_t m_tlds_cons = 0; // Active TDLS connections
	//uint32_t m_max_tdls = 12; // Maximum TDLS connections
	std::map<ns3::Ipv4Address, std::map<iface, ns3::Ipv4Address>> ServersIps;
	//std::map<ns3::Ipv4Address, std::array<ns3::Ipv4Address, 2>> client_server;
	std::map<ns3::Ipv4Address, std::map<ns3::Ipv4Address, TdlsStatus>> ServerStatus;

	std::map<ns3::Ipv4Address, uint32_t> ServerUsers;

	std::map<ns3::Ipv4Address, uint32_t> ServerTDLSUsers;

	//std::map<ns3::Ipv4Address, std::list<iface>> openIfaces = {iface0, iface1}; //interfaces livres para serem usadas
};

#endif