#ifndef PICO_H
#define PICO_H

using namespace ns3;
using namespace std;

enum update_type_t {server, client_on_request, client_timed};

/**
 * Parameters
 **/
uint32_t random_seed = 1893; // Foundation year

uint32_t node_number = 1; // Node number to use in the simulation
uint32_t simulation_time = 3600; // Time to simulate (in seconds)
double first_join_time = 0.1;
double last_join_time = 1.0;
double prob = 0.5;
double timeout = 10.0;
uint32_t min_peers = 1;
uint32_t min_discovery_timeout = 5;
uint32_t max_discovery_timeout = 12;
uint32_t min_idle_timeout = 20;
uint32_t max_idle_timeout = 20;
uint32_t discovery_timer = 100;
// Gossip Parameters
uint32_t gossip_b_initial = 1;
uint32_t gossip_b = 1;
uint32_t gossip_f = 1;


/**
 * Start the simulation for application T.
 **/
template <typename T>
void StartSimulation(NodeContainer nodes, Ptr<UniformRandomVariable> randomGen, uint32_t minPeers, uint32_t minDiscoveryTimeout, uint32_t maxDiscoveryTimeout, 
        uint32_t minIdleTimeout, uint32_t maxIdleTimeout, uint32_t discoveryTimer, uint32_t gossipBinitial, uint32_t gossipB, uint32_t gossipF, VirtualDiscovery *discovery)
{
  NS_ASSERT(node_number > 0);
  Ptr<T> node;
  for (int x = 0; x < node_number; x++)
  {
    node = CreateObject<T> ();
    node->Setup(discovery, minPeers, minDiscoveryTimeout, maxDiscoveryTimeout, minIdleTimeout, maxIdleTimeout, discoveryTimer, gossipBinitial, gossipB, gossipF, randomGen);
    nodes.Get (x)->AddApplication (node);
    uint32_t start = 1000*(randomGen->GetValue(0.1,1.0));
    node->SetStartTime (MilliSeconds (start));
    node->SetStopTime (MilliSeconds (90000));
  }
  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();
}

#endif