#include "MyServer.h"
#include "VirtualDiscovery.h"
#include "SimpleRing.h"
#include "SimpleStar.h"
#include "PureP2P.h"
#include "WellFormed.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include <regex>
#include <utility>
#include <iterator>
#include <fstream>
#include <climits>
#include <boost/algorithm/string.hpp>
#include "ring.h"

NS_LOG_COMPONENT_DEFINE ("RingSimulator");

using namespace ns3;
using namespace std;

int 
main (int argc, char *argv[])
{

  // start
  srand(time(NULL));

  string phyMode ("DsssRate1Mbps");
	double rss = -80;  // -dBm
  string filename = "";

  CommandLine cmd;
  cmd.AddValue ("Conf", "Configuration File Location", filename);
  cmd.AddValue ("Seed", "Set Random Seed", random_seed);
  cmd.Parse (argc, argv);

  if (filename != "") if(!ConfigurationParser(filename)) return EXIT_FAILURE;
  
  RngSeedManager::SetSeed(random_seed);
  Ptr<UniformRandomVariable> randomGen = CreateObject<UniformRandomVariable>();

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  // config

  NodeContainer nodes;
  nodes.Create(node_number);

  PointToPointHelper BT;
  BT.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  BT.SetChannelAttribute("Delay", StringValue("100ms"));


  WifiHelper wifi;
  //wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  //NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  QosWifiMacHelper wifiMac = QosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (200.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, 5.0, 0.0));
  positionAlloc->Add (Vector (55.0, 5.0, 0.0));
  positionAlloc->Add (Vector (55.0, 5.0, 5.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  InternetStackHelper internet;
  internet.SetTcp("ns3::TcpL4Protocol");
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  
  VirtualDiscovery discovery(randomGen);
  if (application == "ring") StartSimulation<SimpleRing>(nodes, randomGen, &discovery);
  else if(application == "star") StartSimulation<SimpleStar>(nodes, randomGen, &discovery);
  else if (application == "purep2p") StartSimulation<PureP2P>(nodes, randomGen, &discovery);
  else if (application == "wellformed") StartSimulation<WellFormed>(nodes, randomGen, &discovery);
  else cerr << "No valid application defined." << endl;

  if (trace_file.is_open()){
    trace_file.close();
  }

  return EXIT_SUCCESS;

}
