/**
 *  TODO:
 *  
 *  - Em vez de encontrar ips no VirtualDiscovery, permitir encontrar diretamente Node.
 *  - Em vez de enviar mensagens por socket, posso usar um ScheduleWithContext noutro nó para fazer ligação.  [importante - Fica mais real. Ou não.]
 *  - Usar as socket connections para controlar as ligações. Incluindo as refusals. Manter a socket connection aberta. [importante - Fica mais real.]
 */

#include "VirtualDiscovery.h"
#include "p2p.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include <regex>
#include <utility>
#include <iterator>
#include <fstream>
#include <climits>
#include <boost/algorithm/string.hpp>
#include "piconet.h"
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("PiconetSimulator");


using namespace ns3;
using namespace std;

// Gerar uma grid de dispositivos
vector<tuple<double, double>> MakeGrid(uint32_t nnodes, uint32_t perchoice = 4, double xspace = 0.2, double yspace = 0.2, bool rowfirst = true)
{
  uint32_t nrows, ncols;
  double xmin, ymin;
  vector<tuple<double, double>> ret;
  if (rowfirst)
  {
    nrows = perchoice;
    ncols = (uint32_t)ceil((double)nnodes / (double)perchoice);
  }
  else
  {
    nrows = (uint32_t)ceil((double)nnodes / (double)perchoice);
    ncols = perchoice;
  }
  if (nrows % 2 == 0)
  {
    xmin = -1 * (((nrows / 2) * xspace) - (xspace / 2));
  }
  else
  {
    xmin = -1 * (((nrows - 1) / 2) * xspace);
  }
  if (ncols % 2 == 0)
  {
    ymin = -1 * (((ncols / 2) * yspace) - (yspace / 2));
  }
  else
  {
    ymin = -1 * (((ncols - 1) / 2) * yspace);
  }
  cout << ncols << "\t" << nrows << endl;
  for (uint32_t r = 0; r < nrows; ++r)
  {
    // if (r >= (nnodes % nrows))
    // {
    //   ncols = nnodes % ncols;
    // }
    for (uint32_t c = 0; c < ncols; ++c)
    {
      cout << "(" << (xmin + (r * xspace)) << ", " << (ymin + (c * yspace)) << ") ";
      ret.emplace(end(ret), make_tuple((xmin + (r * xspace)), (ymin + (c * yspace))));
    }
    cout << endl;
  }
  return ret;
}


int 
main (int argc, char *argv[])
{
  // start
  srand(time(NULL));
  string phyMode ("DsssRate1Mbps");


  CommandLine cmd;
  cmd.AddValue("Nodes", "Number of Nodes (1 default)", node_number);
  cmd.AddValue("P", "Probability of CS [0.0-1.0] (0.5 default)", prob);
  cmd.AddValue("MinPeers", "Mininum number of peers.", min_peers);
  cmd.AddValue("DiscoveryTimer", "Time between discovery lookups (in ms)", discovery_timer);
  cmd.AddValue("MinDiscoveryTimeout", "Minimum discovery timeout (in s)", min_discovery_timeout);
  cmd.AddValue("MaxDiscoveryTimeout", "Maximum discovery timeout (in s)", max_discovery_timeout);
  cmd.AddValue("MinIdleTimeout", "Minimum discovery idle timeout (in s)", min_idle_timeout);
  cmd.AddValue("MaxIdleTimeout", "Maximum discovery idle timeout (in s)", max_idle_timeout);
  cmd.AddValue("SimDuration", "Simulation time (in s)", simulation_time);
  cmd.AddValue("FirstJoinTime", "Last join time (in s)", first_join_time);
  cmd.AddValue("LastJoinTime", "Last join time (in s)", last_join_time);
  cmd.AddValue("GossipBinitial", "Gossip Algorithm Binitial", gossip_b_initial);
  cmd.AddValue("GossipB", "Gossip Algorithm B", gossip_b);
  cmd.AddValue("GossipF", "Gossip Algorithm F", gossip_f);
  cmd.AddValue ("Seed", "Set Random Seed", random_seed);
  cmd.Parse (argc, argv);
  LogComponentEnable("p2pApplication", LOG_LEVEL_DEBUG);
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

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
  //wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  QosWifiMacHelper wifiMac = QosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
  NetDeviceContainer TDLSdevices = wifi.Install (wifiPhy, wifiMac, nodes);

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

  //! Using a random position alocator
  Ptr<RandomBoxPositionAllocator> positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
  positionAlloc->SetX(x);
  positionAlloc->SetY(y);
  positionAlloc->SetZ(z);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("0.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);


  VirtualDiscovery discovery(randomGen);
  StartSimulation<p2p>(nodes, randomGen, min_peers, min_discovery_timeout, max_discovery_timeout, min_idle_timeout, max_idle_timeout, discovery_timer, gossip_b_initial, gossip_b, gossip_f, &discovery);
  printf("ALL_DONE\n");

  return EXIT_SUCCESS;

}
