#include "TdlsManager.h"

NS_LOG_COMPONENT_DEFINE ("TdlsManager");

//Default constructor
TdlsManager::TdlsManager(){
	NS_LOG_DEBUG("Init TdlsManager");
	m_n_servers = 0;
}

TdlsManager::TdlsManager(uint32_t NServers){
	NS_LOG_DEBUG("Init TdlsManager");
	m_n_servers = NServers;
}

TdlsManager::~TdlsManager(){
	NS_LOG_DEBUG("Destroy TdlsManager");
}

void TdlsManager::AddNode(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, ns3::Ipv4Address iface0Address, ns3::Ipv4Address iface1Address, iface iface, bool use_tdls){
	ServerStatus[serverAddress][NodeAddress].status = run;
	if (use_tdls){
		ServerStatus[serverAddress][NodeAddress].connection = tdls;
		ServerTDLSUsers[serverAddress]++;
	}
	else{
		ServerStatus[serverAddress][NodeAddress].connection = regular;
	}
	ServerStatus[serverAddress][NodeAddress].iface = iface;

	ServerUsers[serverAddress]++;

	// tirar esta interface da lista de interfaces disponiveis.
	//openIfaces.erase(std::find(openIfaces.begin(), openIfaces.end(), iface));

	ServersIps[serverAddress][iface0] = iface0Address;
	ServersIps[serverAddress][iface1] = iface1Address;
}

ns3::Ipv4Address TdlsManager::RequestIP(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress){
	iface tmp_iface = ap;
	if (ServerUsers[serverAddress] < 3){ //Vais usar TDLS e tem duas interfaces abertas
		for (auto it = ServerStatus[serverAddress].begin(); it != ServerStatus[serverAddress].end(); it++){
			if (std::get<1>((*it)).status == run && std::get<1>((*it)).iface == iface0)
				tmp_iface = iface1;
			else if (std::get<1>((*it)).status == run && std::get<1>((*it)).iface == iface1)
				tmp_iface = iface0;
		}
	}else if (ServerTDLSUsers[serverAddress] == 0){ // Vais usar TDLS e tem 1 iface aberta.
		tmp_iface = iface0;
	}
	if (tmp_iface != ap){
		ServerStatus[serverAddress][NodeAddress].connection = tdls;
		ServerStatus[serverAddress][NodeAddress].iface = tmp_iface;
		ServerTDLSUsers[serverAddress]++;
		return ServersIps[serverAddress][tmp_iface];
	}
	// Vais usar o AP.
	return serverAddress;
}


void TdlsManager::UpdateStatusDone(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress, bool using_tdls){
	ServerStatus[serverAddress][NodeAddress].status = stopped;
	ServerUsers[serverAddress]--;
	if (using_tdls){
		ServerTDLSUsers[serverAddress]--;
	}
	//std::cout << "updatestatus" << ServerTDLSUsers[serverAddress] << std::endl;
}

// void TdlsManager::UpdateStatusDoneTDLS(ns3::Ipv4Address serverAddress, ns3::Ipv4Address NodeAddress){
// 	ServerStatus[serverAddress][NodeAddress].status = stopped;
// 	ServerUsers[serverAddress]--;
// 	ServerTDLSUsers[serverAddress]--;
// 	//openIfaces.emplace(openIfaces.end(), ServerStatus[serverAddress][NodeAddress].iface);
// }