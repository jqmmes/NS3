#ifndef P2P_H
#define P2P_H

#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include "VirtualDiscovery.h"
#include <climits>

using namespace ns3;
using namespace std;

/*! 
 * A class implementing a P2P Formation Scheme. 
 * 
 * This class allows to form a Peer-to-Peer network varying
 * several parameters such as discovery timetout, ...
 */
class p2p : public Application
{
public:
  static TypeId GetTypeId (void);

  p2p();
  virtual ~p2p();
  void Setup(VirtualDiscovery *discovery, Ptr<UniformRandomVariable> rand);

private:

  // Application
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // Private
  bool ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from);
  void AcceptConnection(Ptr<Socket> socket, const ns3::Address& from);
  void ConnectSuccess(Ptr<Socket> socket);
  void ConnectFail(Ptr<Socket> socket);
  void NormalClose(Ptr<Socket> socket);
  void ErrorClose(Ptr<Socket> socket);
  Ptr<Socket> StartSocket(void);

  // Comunications
  void Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port = 0);
  void Send(string sdata, Ipv4Address d_address, uint16_t port = 0);
  void Send(stringstream& sdata, Ipv4Address d_address, uint16_t port = 0);
  void ReceivePacket (Ptr<Socket> socket);
  void SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i = 0);
  void ReadPacket(Ptr<Socket> socket, Address from);
  bool isPeerFull(Ptr<Node> node);
  Ptr<Node> getNode(Ipv4Address ip);
  
  // Formation
  void Formation(uint32_t i, uint32_t limit);
  void setDiscoverable(bool status = true);

  // Variables
  VirtualDiscovery *    m_discovery = NULL; /*!< Virtual Discovery object shared with all nodes*/
  Ptr<Socket>           m_socket; /*!< Communication listening socket*/
  uint32_t              m_packet_size = 1000; /*!< Max packet size */
  Ipv4Address           m_address = Ipv4Address("0.0.0.0"); /*!< Node ip address */
  uint16_t              m_port = 20; /*!< Node communication port */
  map<Ptr<Socket>, void *> m_map; /*!< Map of open socket connections */
  map<Ptr<Socket>, uint32_t> m_map_last_pos; /*!< Last position for each socket connection */
  Ptr<UniformRandomVariable> randomGen; /*!< Random Generator */
  list<Ipv4Address> m_connections; /*!< List of all connections */
  bool m_peer_full;
  uint32_t MAX_CONNECTIONS = 5; /*!< Maximum number of connections per node */
  bool m_connected = false;
};

#endif