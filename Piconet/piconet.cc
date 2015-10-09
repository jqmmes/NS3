//#include "MyServer.h"
#include "VirtualDiscovery.h"
#include "p2p.h"
//#include "SimpleStar.h"
//#include "PureP2P.h"
//#include "WellFormed.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include <regex>
#include <utility>
#include <iterator>
#include <fstream>
#include <climits>
///usr/local/include/
#include <boost/algorithm/string.hpp>
#include "piconet.h"

NS_LOG_COMPONENT_DEFINE ("PiconetSimulator");

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
  uint32_t node_number = 1;
  double prob = 0.5;
  double timeout = 10.0;

  CommandLine cmd;
  cmd.AddValue("N", "Number of Nodes (1 default)", node_number);
  cmd.AddValue("P", "Probability of CS [0.0-1.0] (0.5 default)", prob);
  cmd.AddValue("Timeout", "CM/Discovery Timeout (10.0s default)", timeout);
  cmd.AddValue ("Seed", "Set Random Seed", random_seed);
  cmd.Parse (argc, argv);

  NS_ASSERT(prob >= 0.0 && prob <= 1.0);
  
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

  //PointToPointHelper BT;
  //BT.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  //BT.SetChannelAttribute("Delay", StringValue("100ms"));


  WifiHelper wifi;
  //wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  //wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  //wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

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
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute("Min", DoubleValue(-10.0));
  x->SetAttribute("Max", DoubleValue(10.0));
  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  y->SetAttribute("Min", DoubleValue(-10.0));
  y->SetAttribute("Max", DoubleValue(10.0));
  Ptr<UniformRandomVariable> z = CreateObject<UniformRandomVariable> ();
  z->SetAttribute("Min", DoubleValue(1.0));
  z->SetAttribute("Max", DoubleValue(1.0));

  Ptr<RandomBoxPositionAllocator> positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
  positionAlloc->SetX(x);
  positionAlloc->SetY(y);
  positionAlloc->SetZ(z);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  InternetStackHelper internet;
  //internet.SetTcp("ns3::TcpL4Protocol");
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("0.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  
  VirtualDiscovery discovery(randomGen);
  // if (application == "ring") StartSimulation<SimpleRing>(nodes, randomGen, &discovery);
  // else if(application == "star") StartSimulation<SimpleStar>(nodes, randomGen, &discovery);
  // else if (application == "purep2p") StartSimulation<PureP2P>(nodes, randomGen, &discovery);
  // else if (application == "wellformed") StartSimulation<WellFormed>(nodes, randomGen, &discovery);
  // else cerr << "No valid application defined." << endl;

  // if (trace_file.is_open()){
  //   trace_file.close();
  // }

  printf("ALL_DONE\n");

  return EXIT_SUCCESS;

}
