#include "Experiment.h"
#include "ExperimentApp.h"
#include "ExperimentAppWifiDirect.h"

NS_LOG_COMPONENT_DEFINE ("HyraxExperiment");

void
PhyRxOkTrace(ns3::Ptr<const ns3::Packet> pkt){
  std::cout << "wut" << std::endl;
}

int main(int argc, char *argv[]){
  LogComponentEnable("HyraxExperimentApp", ns3::LOG_LEVEL_INFO);

  ns3::CommandLine cmd;
  cmd.AddValue ("Nodes", "Number of Nodes to be used in the simulation", RemoteNodesN);
  cmd.AddValue ("Servers", "Number of Servers to be used in the simulation", MobileServersN);
  cmd.AddValue ("Scenario", "Scenario to run", Scenario);
  cmd.AddValue ("FileSize", "File Size to be shared", FileSize);
  cmd.AddValue ("Debug", "Debug socket callbacks", debug);
  cmd.AddValue ("ShowPackets", "Show every packet received", show_packages);
  cmd.AddValue ("ShowData", "Show Send/Receive instead of the time a transfer took", show_data);
  cmd.AddValue ("Seed", "Seed to be used", seed);
  cmd.AddValue ("ExclusiveServers", "Use Exclusive Server. (Server Dont act as Client)", exclusive);
  cmd.Parse (argc, argv);
  //if (Scenario == 42 && MobileServersN == 1) RemoteNodesN++;
  if (Scenario != 1 && Scenario != 41 && exclusive) RemoteNodesN += MobileServersN; // Except for scenario 1 and 4a
  wifiDirect = ((Scenario > 3) ? true : false);
  ns3::RngSeedManager::SetSeed(seed);

  if (!wifiDirect){
    pointToPoint.SetDeviceAttribute ("DataRate", ns3::StringValue ("1000Mbps")); // 1Gbps
    pointToPoint.SetChannelAttribute ("Delay", ns3::StringValue ("1ns"));

  	p2pNodes.Create (2);
    p2pDevices = pointToPoint.Install (p2pNodes);

    //p2pNodes_alt.Create (2);
    //p2pDevices = pointToPoint.Install (p2pNodes);
    
    wifiApNode = p2pNodes.Get(0);
    serverNode = p2pNodes.Get(1);
    //wifiApNode_alt = p2pNodes_alt.Get(0);
    //serverNode_alt = p2pNodes_alt.Get(1);

    remoteNodes.Create(RemoteNodesN);
    //remoteNodes_alt.Create(1);
  }

  if (wifiDirect){
    // Wifi Direct 1 GO and 2 Slaves
    
    //wifiDirectSlaves.Create(RemoteNodesN+1);
    //wifiDirectGO = wifiDirectSlaves.Get (0);
    wifiDirectGO.Create(1);
    wifiDirectSlaves.Create(RemoteNodesN);
  }

  ns3::YansWifiChannelHelper channel = ns3::YansWifiChannelHelper::Default ();
  ns3::YansWifiPhyHelper phy = ns3::YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  //ns3::YansWifiPhyHelper phy_alt = ns3::YansWifiPhyHelper::Default ();
  //phy_alt.SetChannel (channel.Create ());

  if (phy_type == "802.11n"){
    // Edited src/wifi/model/yans-wifi-phy.ccsrc/wifi/model/wifi-phy.cc
    // Edited src/wifi/model/yans-wifi-phy.cc
    // To make OfdmRate150MbpsBW40MHz available.
    //
    // This class can create MACs of type ns3::ApWifiMac, ns3::StaWifiMac, and, ns3::AdhocWifiMac, with QosSupported 
    // and HTSupported attributes set to True.
    //
    //phy.Set ("ChannelBonding", ns3::BooleanValue(true));
    //phy.Set ("ChannelWidth", ns3::UintegerValue(40));
    //phy.Set ("ShortGuardEnabled", ns3::BooleanValue(true)); //Increase Throughput by reducing Word Guard time
    //phy.Set ("GreenfieldEnabled", ns3::BooleanValue(true)); //Enable HT
    // phy_alt.Set ("ChannelBonding", ns3::BooleanValue(true));
    // phy_alt.Set ("ShortGuardEnabled", ns3::BooleanValue(true)); //Increase Throughput by reducing Word Guard time
    // phy_alt.Set ("GreenfieldEnabled", ns3::BooleanValue(true)); //Enable HT
    // mac.SetMsduAggregatorForAc (ns3::AC_VO, "ns3::MsduStandardAggregator",
    //                             "MaxAmsduSize", ns3::UintegerValue (3839));
    // mac.SetBlockAckThresholdForAc (ns3::AC_BE, 10);
    // mac.SetBlockAckInactivityTimeoutForAc (ns3::AC_BE, 5);
    // 135mbps also works.
    wifi.SetStandard(ns3::WIFI_PHY_STANDARD_80211n_5GHZ);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                  "DataMode", ns3::StringValue("HtMcs7"),
                                  "ControlMode", ns3::StringValue("HtMcs0"));
    // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
    //                               "DataMode", ns3::StringValue("OfdmRate65MbpsBW20MHz"), 
    //                               "ControlMode", ns3::StringValue("OfdmRate6_5MbpsBW20MHz"));
  }else if(phy_type == "802.11ac"){
    //ChannelWidth
    //phy.SetStandard("ChannelWidth", ns3::UintegerValue(160));
    phy.Set ("ShortGuardEnabled", ns3::BooleanValue(true)); //Increase Throughput by reducing Word Guard time
    wifi.SetStandard(ns3::WIFI_PHY_STANDARD_80211ac);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                  "DataMode", ns3::StringValue("VhtMcs9"), 
                                  "ControlMode", ns3::StringValue("VhtMcs0"));
    // !!! Isto tem de ficar noutro sitio
    ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(160)); //Isto deve ser chamado no final
  }else if(phy_type == "802.11g-54mbps"){
    // Working - src/wifi/model/wifi-phy.cc para mais modelos.
    // Good source - https://www.nsnam.org/docs/models/html/wifi.html
    wifi.SetStandard (ns3::WIFI_PHY_STANDARD_80211g);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode", ns3::StringValue ("ErpOfdmRate54Mbps"),
                                  "ControlMode", ns3::StringValue ("ErpOfdmRate54Mbps"));
  }else{
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  }
  
  ns3::Ssid ssid = ns3::Ssid ("ns3-wifi");

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", ns3::SsidValue (ssid),
               "ActiveProbing", ns3::BooleanValue (false));

  //ns3::NetDeviceContainer staDevices;
  
  staDevices = wifi.Install (phy, mac, remoteNodes);

  // mac.SetType ("ns3::StaWifiMac",
  //              "Ssid", ns3::SsidValue (ns3::Ssid ("ns3-wifi-alt")),
  //              "ActiveProbing", ns3::BooleanValue (false));
  //staDevices_alt = wifi.Install (phy_alt, mac, remoteNodes_alt);

  if (wifiDirect){
    // Wifi Direct slaves are RS
    wifiDirectSlaveDevices = wifi.Install(phy, mac, wifiDirectSlaves);
  }

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", ns3::SsidValue (ssid));

  apDevices = wifi.Install (phy, mac, wifiApNode);

  // mac.SetType ("ns3::ApWifiMac",
  //              "Ssid", ns3::SsidValue (ns3::Ssid ("ns3-wifi-alt")));

  // apDevices_alt = wifi.Install (phy_alt, mac, wifiApNode_alt);

  if (wifiDirect){
    // Wifi Direct Group Owner acts as an AP
    WifiDirectGODevices = wifi.Install(phy, mac, wifiDirectGO);
  }

  mac.SetType ("ns3::AdhocWifiMac",
               "Ssid", ns3::SsidValue (ssid));
  adhocDevices = wifi.Install (phy, mac, remoteNodes);

  if (wifiDirect){
    WifiDirectadhocGODevices = wifi.Install (phy, mac, wifiDirectGO);  
    WifiDirectadhocDevices = wifi.Install (phy, mac, wifiDirectSlaves);
  }

  // Mobility
  ns3::MobilityHelper mobility;
  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", ns3::DoubleValue (0.0),
  //                                "MinY", ns3::DoubleValue (0.0),
  //                                "DeltaX", ns3::DoubleValue (5.0),
  //                                "DeltaY", ns3::DoubleValue (10.0),
  //                                "GridWidth", ns3::UintegerValue (3),
  //                                "LayoutType", ns3::StringValue ("RowFirst"));

  // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                            "Bounds", ns3::RectangleValue (ns3::Rectangle (-5, 5, -5, 5)));
  ns3::Ptr<ns3::ListPositionAllocator> positionAlloc = ns3::CreateObject<ns3::ListPositionAllocator> ();
  positionAlloc->Add (ns3::Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (ns3::Vector (1.0, 0.0, 0.0));
  positionAlloc->Add (ns3::Vector (2.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (remoteNodes);
  //mobility.Install (remoteNodes_alt);

  if (wifiDirect){
    // Wifi Direct Nodes can move freely
    ns3::MobilityHelper WifiDirectMobility;
    WifiDirectMobility.Install (wifiDirectGO);
    WifiDirectMobility.Install (wifiDirectSlaves);
  }

  // AP and Server are at a fixed location
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  //mobility.Install (wifiApNode_alt);
  mobility.Install (serverNode);

  //list.Add (olsr, 10);

  //stack.SetRoutingHelper (olsr);  
  stack.SetTcp("ns3::TcpL4Protocol");
  ns3::Config::SetDefault("ns3::TcpSocket::SndBufSize", ns3::UintegerValue (52428800));
  ns3::Config::SetDefault("ns3::TcpSocket::RcvBufSize", ns3::UintegerValue (52428800));
  ns3::Config::SetDefault("ns3::TcpSocket::SegmentSize", ns3::UintegerValue (10000)); // Afecta a velocidade
  stack.Install (serverNode);
  stack.Install (wifiApNode);
  //stack.Install (wifiApNode_alt);
  stack.Install (remoteNodes);
  //stack.Install (remoteNodes_alt);
  
  if (phy_type == "802.11n" || phy_type == "802.11ac"){
    if (phy_type == "802.11n")
      ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(40));
    else
      ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(160));
    ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ShortGuardEnabled", ns3::BooleanValue(true));
  }
  

  // olsr.AssignStreams (p2pNodes, 0);
  // olsr.AssignStreams (remoteNodes, 1);

  if (wifiDirect){
    // Wifi Direct Internet Stack
    stack.Install (wifiDirectGO);
    stack.Install (wifiDirectSlaves);
  }

	address.SetBase ("10.1.1.0", "255.255.255.0");
	address.Assign (p2pDevices);
  //address.Assign (apDevices_alt);

	address.SetBase ("10.1.2.0", "255.255.255.0");
	address.Assign (staDevices);
  address.Assign (apDevices);

  //ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // address.SetBase ("10.1.10.0", "255.255.255.0");
  // address.Assign (p2pDevices_alt);

  // address.SetBase ("10.1.11.0", "255.255.255.0");
  // address.Assign (staDevices_alt);
  // address.Assign (apDevices_alt);

  // Populate Routing tables for the base network
  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Attribute ips to the secondary interface AdHoc
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign(adhocDevices);

  if (wifiDirect){
    // Wifi Direct Nodes will use a 10.1.4.0 ip base
    address.SetBase ("10.1.4.0", "255.255.255.0");
    address.Assign (WifiDirectGODevices);
    address.Assign (wifiDirectSlaveDevices);

    address.SetBase ("10.1.5.0", "255.255.255.0");
    address.Assign (WifiDirectadhocGODevices);
    address.Assign (WifiDirectadhocDevices);
  }

  if (!wifiDirect){
    PlaceApplication<HyraxExperimentApp>(remoteNodes.Begin(), remoteNodes.End(), "RemoteStation");
    //PlaceApplication<HyraxExperimentApp>(remoteNodes_alt.Begin(), remoteNodes_alt.End(), "RemoteStation");
    PlaceApplication<HyraxExperimentApp>(serverNode.Begin(), serverNode.End(), "Server");
    PlaceApplication<HyraxExperimentApp>(wifiApNode.Begin(), wifiApNode.End(), "AccessPoint");
    //PlaceApplication<HyraxExperimentApp>(wifiApNode_alt.Begin(), wifiApNode_alt.End(), "RemoteStation");
  }else{
    ns3::NodeContainer::Iterator it = wifiDirectGO.Begin();
    PlaceApplication<HyraxExperimentAppWifiDirect>(it, std::next(it,1), "WD_GO");
    //it = wifiDirectSlaves.Begin();
    //std::advance(it,1);
    PlaceApplication<HyraxExperimentAppWifiDirect>(wifiDirectSlaves.Begin(), wifiDirectSlaves.End(), "WD_Slave");
  }

  std::cout << 
    "===================" << std::endl <<
    "Simulation Duration: " << TotalSimulationTime << " s" << std::endl <<
    "Remote Nodes: " << RemoteNodesN << std::endl << 
    "Number of Mobile Servers: " << MobileServersN << std::endl << 
    "Scenario " << experiment_scenarios[Scenario] << std::endl << 
    "File Size: " << FileSize << " bytes" << std::endl << 
    "Use Wifi-Direct: " << (wifiDirect ? "yes" : "no") << std::endl <<
    "Using Exclusive Servers: " << (exclusive ? "yes" : "no") << std::endl <<
    "Seed: " << seed << std::endl <<
    "===================" << std::endl;

  //ns3::Config::Connect ("/NodeList/2/DeviceList/*/Phy/State/RxOk",MakeCallback(&PhyRxOkTrace));

  ns3::Simulator::Stop(ns3::MilliSeconds(TotalSimulationTime));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy ();

	return EXIT_SUCCESS;
}



template <typename T>
void PlaceApplication(ns3::NodeContainer::Iterator begin, ns3::NodeContainer::Iterator end, std::string role){

  for (ns3::NodeContainer::Iterator it = begin; it != end; it++){
    AddApplication<T>((*it), role);
  }
}

template <typename T>
void AddApplication(ns3::Ptr<ns3::Node> node, std::string role){
    ns3::Ptr<T> App = ns3::CreateObject<T> ();
    App->Setup(role, RemoteNodesN, MobileServersN, Scenario, FileSize, debug, show_packages, show_data, exclusive);
    node->AddApplication(App);
    App->SetStartTime (ns3::MilliSeconds (1000));
    App->SetStopTime (ns3::MilliSeconds (AppSimulationTime));
}