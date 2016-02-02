#ifndef HYRAX_EXPERIMENT_APP_WIFIDIRECT_H
#define HYRAX_EXPERIMENT_APP_WIFIDIRECT_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "TdlsManager.h"

class HyraxExperimentAppWifiDirect : public ns3::Application
{
public:
	HyraxExperimentAppWifiDirect();
	virtual ~HyraxExperimentAppWifiDirect();

	void Setup(std::string type, uint32_t NNodes, uint32_t NServers, uint32_t Scenario, uint32_t FileSize, bool Debug, bool ShowPackages, bool ShowData, bool exclusive, TdlsManager * tdlsman);

private:
	virtual void StartApplication (void);
  virtual void StopApplication (void);
  void RunSimulation(void);

  // Socket related stuff
  void NormalClose(ns3::Ptr<ns3::Socket> socket);
  void ErrorClose(ns3::Ptr<ns3::Socket> socket);
  bool ConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  void AcceptConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  void ConnectSuccess(ns3::Ptr<ns3::Socket> socket);
  void ConnectFail(ns3::Ptr<ns3::Socket> socket);
  void ReceivePacket(ns3::Ptr<ns3::Socket> socket);
  void Send(ns3::Ptr<ns3::Socket> socket, std::string data);
  void ReadData(ns3::Ptr<ns3::Socket> socket, ns3::Address from, std::string data);
  ns3::Ptr<ns3::Socket> CreateSocket(void);
  std::map<ns3::Ptr<ns3::Socket>, void *> socket_data;
  std::map<ns3::Ptr<ns3::Socket>, uint32_t> current_socket_data;


  std::string m_type; // Slave or GO
  ns3::Ipv4Address m_address;
  ns3::Ptr<ns3::Socket> GlobalSocket;
  ns3::Ptr<ns3::Socket> SocketListener;
  uint32_t packet_size = 2900000;
  std::map<ns3::Ptr<ns3::Socket>, std::stringstream> SocketData = {};


	//Experiments
	void Scenario_4_a(void);
  void Scenario_4_b(void);
  void Scenario_4_c(void);
	void Scenario_5_a(void);
  void Scenario_5_b(void);
	void Scenario_6(void);
  uint32_t scenario_id;

  // WifiDirect
  void ConnectSocket(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data);
  void CompleteConnectSocket(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data);
  void ConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data, ns3::Ipv4Address Adhoc_ip, uint32_t Adhoc_port);
  void CompleteConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port);
  void CompleteAdHocConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data);

  // General data
  uint32_t n_nodes;
  uint32_t files_to_fetch = 10;
  uint32_t files_fetched = 0;
  double fetch_init_time;
  uint32_t n_servers = 1;
  bool exclusive_servers = false;
  void genServerList(void);
  std::vector<ns3::Ipv4Address> ServerList;
  std::vector<ns3::Ipv4Address> AdHocServerList;

  ns3::Ptr<ns3::UniformRandomVariable> randomGen = ns3::CreateObject<ns3::UniformRandomVariable>();


	// Debug
	bool debug = false;
	bool show_packages = false;
  bool show_data = false;
	uint32_t c_index = 0;
	uint32_t total_data = 0;
	uint32_t total_sent_data = 0;
};

#endif