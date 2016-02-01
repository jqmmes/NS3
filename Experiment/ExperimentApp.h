#ifndef HYRAX_EXPERIMENT_APP_H
#define HYRAX_EXPERIMENT_APP_H


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include <vector>

struct TDLS
{
	ns3::Ptr<ns3::Socket> socket;//"] = socket;
	std::string data;//"] = data;
	bool delivered;// = false;
};


class HyraxExperimentApp : public ns3::Application
{
public:
	HyraxExperimentApp();
	virtual ~HyraxExperimentApp();

	void Setup(std::string type, uint32_t NNodes, uint32_t NServers, uint32_t Scenario, uint32_t FileSize, bool Debug, bool ShowPackages, bool ShowData, bool exclusive);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void RunSimulation(void);

  // Socket related stuff
  void NormalClose(ns3::Ptr<ns3::Socket> socket);
  void ErrorClose(ns3::Ptr<ns3::Socket> socket);
  void TDLSNormalClose(ns3::Ptr<ns3::Socket> socket);
  void TDLSErrorClose(ns3::Ptr<ns3::Socket> socket);
  bool ConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  bool TDLSConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  void AcceptConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  void AcceptTDLSConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from);
  void ConnectSuccess(ns3::Ptr<ns3::Socket> socket);
  void ConnectFail(ns3::Ptr<ns3::Socket> socket);
  void ReceivePacket(ns3::Ptr<ns3::Socket> socket);
  void Send(ns3::Ptr<ns3::Socket> socket, std::string data);
  void ReadData(ns3::Ptr<ns3::Socket> socket, ns3::Address from, std::string data);
  ns3::Ptr<ns3::Socket> CreateSocket(void);
  ns3::Ptr<ns3::Socket> CreateTDLSSocket(void);
  std::map<ns3::Ptr<ns3::Socket>, void *> socket_data;
  std::map<ns3::Ptr<ns3::Socket>, uint32_t> current_socket_data;


  ns3::Ptr<ns3::Socket> Listen;
  ns3::Ptr<ns3::Socket> TDLSListen;
  std::string m_type;
  ns3::Ipv4Address m_address;
  ns3::Ptr<ns3::Socket> socket;
  uint32_t packet_size = 2900000;
  std::map<ns3::Ptr<ns3::Socket>, std::stringstream> SocketData;

  ns3::Time first_receive = ns3::Time(0);
	ns3::Time last_receive = ns3::Time(0);

	//Experiments
	void Scenario_1(void);
	void Scenario_2(void);
	void Scenario_3(void);
	void Stress_Server(void);
	uint32_t scenario_id = 1;

	// General data
	uint32_t m_n_nodes;
	uint32_t files_to_fetch = 10;
	uint32_t files_fetched = 0;
	uint32_t server_files_fetched = 0;
	double fetch_init_time;
	uint32_t m_n_servers = 1;
	bool exclusive_servers = false;
	void genServerList(void);
	std::vector<ns3::Ipv4Address> ServerList;
	std::vector<ns3::Ipv4Address> AdHocServerList;

	ns3::Ptr<ns3::UniformRandomVariable> randomGen = ns3::CreateObject<ns3::UniformRandomVariable>();


	bool new_sim = false;
	bool measure_only_server = false;
	bool measure_data = false;

	// Debug
	bool debug = false;
	bool show_packages = false;
	bool show_data = false;
	uint32_t c_index = 0;
	uint32_t total_data = 0;
	uint32_t total_sent_data = 0;


	/**
	* TDLS Data
	**/

	void SendTDLS(ns3::Ptr<ns3::Socket> socket, std::string data, ns3::Ipv4Address to);
	void CheckTDLS(ns3::Ptr<ns3::Socket> socket, std::string data, ns3::Ipv4Address to);
	bool TDLS_enabled = true;
	uint32_t m_ActiveTDLSCons = 0;
	uint32_t m_MaxTDLSCons = 1;
	uint32_t m_TDLSTimeout = 200; /*< 100ms */
	ns3::Ptr<ns3::Socket> tdlsSock;

	TDLS m_TDLSData;


	/**
	* Temporary for TDLS experiment.
	**/
	void genServerListSpecial(void);
	bool m_tdls_active = false;


};

#endif