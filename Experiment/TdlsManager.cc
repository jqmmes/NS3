#include "TdlsManager.h"

NS_LOG_COMPONENT_DEFINE ("TdlsManager");

//Default constructor
TdlsManager::TdlsManager(){
	NS_LOG_DEBUG("Init TdlsManager");
	m_n_servers = 0;
}

TdlsManager::TdlsManager(uint32_t NServers, uint32_t SuccPerc){
	NS_LOG_DEBUG("Init TdlsManager");
	m_n_servers = NServers;
	m_perc = SuccPerc;
}

TdlsManager::~TdlsManager(){
	NS_LOG_DEBUG("Destroy TdlsManager");
}

void TdlsManager::AddNode(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, ns3::Ipv4Address iface0Address, ns3::Ipv4Address iface1Address, iface iface, bool use_tdls){
	ServerStatus[serverAddress][NodeAddress].m_status = run;
	if (use_tdls){
		ServerStatus[serverAddress][NodeAddress].m_connection = tdls;
		ServerTDLSUsers[serverAddress]++;
	}
	else{
		ServerStatus[serverAddress][NodeAddress].m_connection = regular;
	}
	ServerStatus[serverAddress][NodeAddress].m_iface = iface;

	ServerUsers[serverAddress]++;

	ServersIps[serverAddress][iface0] = iface0Address;
	ServersIps[serverAddress][iface1] = iface1Address;
}

ns3::Ipv4Address TdlsManager::RequestIP(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress){
	iface tmp_iface = ap;
	//std::cout << ServerUsers[serverAddress] << std::endl;
	// std::cout << m_perc << std::endl;
	// roll a die
	if (randomGen->GetInteger(0, 100) > m_perc) return serverAddress;

	if (ServerUsers[serverAddress] < 3){ //Vais usar TDLS e tem duas interfaces abertas
		for (auto it = ServerStatus[serverAddress].begin(); it != ServerStatus[serverAddress].end(); it++){
			if (std::get<1>((*it)).m_status == run && std::get<1>((*it)).m_iface == iface0)
				tmp_iface = iface1;
			else if (std::get<1>((*it)).m_status == run && std::get<1>((*it)).m_iface == iface1)
				tmp_iface = iface0;
		}
		if (tmp_iface == ap)
			tmp_iface = iface0;
	}else if (ServerTDLSUsers[serverAddress] == 0){ // Vais usar TDLS e tem 1 iface aberta.
		tmp_iface = iface0;
	}
	if (tmp_iface != ap){
		ServerStatus[serverAddress][NodeAddress].m_connection = tdls;
		ServerStatus[serverAddress][NodeAddress].m_iface = tmp_iface;
		ServerTDLSUsers[serverAddress]++;
		return ServersIps[serverAddress][tmp_iface];
	}
	// Vais usar o AP.
	return serverAddress;
}


void TdlsManager::UpdateStatusDone(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, bool using_tdls){
	ServerStatus[serverAddress][NodeAddress].m_status = stopped;
	ServerUsers[serverAddress]--;
	if (using_tdls){
		ServerTDLSUsers[serverAddress]--;
	}
}