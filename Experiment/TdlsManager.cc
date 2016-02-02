#include "TdlsManager.h"

NS_LOG_COMPONENT_DEFINE ("TdlsManager");

TdlsManager::TdlsManager(){
	NS_LOG_DEBUG("Init TdlsManager");
}

TdlsManager::~TdlsManager(){
	NS_LOG_DEBUG("Destroy TdlsManager");
}

void TdlsManager::AddNode(ns3::Ptr<ns3::Node> node, ns3::Ipv4Address iface0, ns3::Ipv4Address iface1, ns3::Ipv4Address iface2){
	// if (network.find(node) == network.end()){
	// 	TDLS a;
	// 	a.iface0 = iface0;
	// 	a.iface1 = iface1;
	// 	a.iface2 = iface2;
	// 	network.emplace(std::make_pair(node, a));
	// }
}