#ifndef HYRAX_EXPERIMENT_H
#define HYRAX_EXPERIMENT_H

#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/wifi-phy-standard.h"
#include <ns3/olsr-helper.h>
#include <ns3/aodv-helper.h>
#include <ns3/rng-seed-manager.h>

#include <string>
#include <regex>
#include <utility>
#include <iterator>
#include <fstream>
#include <climits>
#include <array>
//#include <boost/algorithm/string.hpp>

// Default Network Topology
//
// Wifi 10.1.2.0
//  *    *    * point-to-point
//  |    |    |    10.1.1.0
// n2   n1   n0 -------------- server
//           AP

template <typename T>
void AddApplication(ns3::Ptr<ns3::Node> node, std::string role);

template <typename T>
void PlaceApplication(ns3::NodeContainer::Iterator begin, ns3::NodeContainer::Iterator end, std::string role);

ns3::WifiHelper wifi = ns3::WifiHelper::Default (); // Wifi Helper
ns3::PointToPointHelper pointToPoint; 							// PTP Helper
ns3::MobilityHelper mobility; 											// Mobility Helper
ns3::InternetStackHelper stack; 										// Internet Stack Helper
ns3::Ipv4AddressHelper address;											// IPv4 Address Helper

// Containers
ns3::NodeContainer p2pNodes;					// Server and AP
ns3::NodeContainer p2pNodes_alt;					// Server and AP
ns3::NodeContainer remoteNodes;				// Mobile devices
ns3::NodeContainer remoteNodes_alt;				// Mobile devices
ns3::NodeContainer serverNode;				// Server Node
ns3::NodeContainer serverNode_alt;				// Server Node
ns3::NodeContainer wifiApNode;				// AP Node (Will have two interfaces PTP & AP)  
ns3::NodeContainer wifiApNode_alt;				// AP Node (Will have two interfaces PTP & AP)  
ns3::NodeContainer wifiDirectGO;
ns3::NodeContainer wifiDirectSlaves;
ns3::NetDeviceContainer p2pDevices;		// Point-To-Point Net Devices Container
ns3::NetDeviceContainer p2pDevices_alt;		// Point-To-Point Net Devices Container
ns3::NetDeviceContainer staDevices;		// Remote Station Net Devices Container
ns3::NetDeviceContainer staDevices_alt;		// Remote Station Net Devices Container
ns3::NetDeviceContainer apDevices;		// AP Net Devices Container
ns3::NetDeviceContainer apDevices_alt;		// AP Net Devices Container
ns3::NetDeviceContainer adhocDevices; // Ad-Hoc Net Devices Container
ns3::NetDeviceContainer WifiDirectGODevices; // Wifi-Direct Group Owner Node
ns3::NetDeviceContainer wifiDirectSlaveDevices; // Wifi-Direct Slave Node
ns3::NetDeviceContainer WifiDirectadhocGODevices;
ns3::NetDeviceContainer WifiDirectadhocDevices;

// Yans Wifi Channel and Phy
ns3::YansWifiChannelHelper channel = ns3::YansWifiChannelHelper::Default ();
ns3::YansWifiPhyHelper phy = ns3::YansWifiPhyHelper::Default ();

//std::string phy_type = "802.11ac";
ns3::VhtWifiMacHelper VHTMac = ns3::VhtWifiMacHelper::Default ();

std::string phy_type = "802.11n";
ns3::HtWifiMacHelper HTMac = ns3::HtWifiMacHelper::Default ();

// Uncomment if not using 802.11n
// std::string phy_type = "802.11g-54mbps";
// ns3::NqosWifiMacHelper mac = ns3::NqosWifiMacHelper::Default ();


// Simulation data
uint32_t AppSimulationTime = 3500000;
uint32_t TotalSimulationTime = 3600000;
uint32_t RemoteNodesN = 4;
uint32_t MobileServersN = 0;
uint32_t Scenario = 1;
uint32_t FileSize = 2900000;
bool wifiDirect = false; // Do the simulations for wifiDirect or not.
bool debug = false; // debug
bool show_packages = false; // debug
bool show_data = false; // debug
uint32_t seed = 1893;
bool exclusive = false;

std::map<uint32_t, std::string> experiment_scenarios {
	{1, "1: 1 Server + AP + n Nodes"},
	{2, "2: AP + m Mobile Servers + n Nodes (m<=n)"},
	{3, "3: AP + TDLS + m Mobile Servers + n Nodes (m<=n)"},
	{41, "4a: WD + GO as Server + n Nodes"},
	{42, "4b: WD + GO + m Mobile Servers + n Nodes (m<=n)"},
	{43, "4c: WD + m Mobiles Servers + n Nodes (m<=n) No groups formed in the beggining"},
	{51, "5a: WD + Legacy AP as Server + n Nodes"},
	{52, "5b: WD + Legacy AP + m Mobile Servers + n Nodes (m<=n)"},
	{6, "6: WD + GO + TDLS + m Mobile Servers + n Nodes (m<=n)"}
};


#endif
