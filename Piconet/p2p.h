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

/*! A class implementing a Ring overlay with communications with a server */
class p2p : public Application
{
public:

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

  // Overlay
  void Leave();
  void Join();
  void Change(stringstream& stream, string side, Ipv4Address ip, uint16_t port);

  // Features
  bool ReqFile(string fid, Ipv4Address reqAddress = Ipv4Address("0.0.0.0"), uint32_t hops = 0, bool toServer = false); /*!< Request File from neighours */

  // Comunications
  void Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port = 0);
  void Send(string sdata, Ipv4Address d_address, uint16_t port = 0);
  void Send(stringstream& sdata, Ipv4Address d_address, uint16_t port = 0);
  void ReceivePacket (Ptr<Socket> socket);
  void SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i = 0);

  //Generic
  uint32_t GetSize(string data);
  void readPacket(Ptr<Socket> socket, Address from);

  // Stuff
  void DoWork(void);



  // Usefull
  void Formation(void);
  void FormationAux(uint32_t i, uint32_t limit);
  void setDiscoverable(void);
  bool connected = false;
  list<Ipv4Address> connections;



  // Variables
  VirtualDiscovery *    m_discovery = NULL; /*!< Virtual Discovery object shared with all nodes*/
  vector<string>        m_file_list;
  Ptr<Socket>           m_socket; /*!< Communication listening socket*/
  uint32_t              m_packet_size = 1000; /*!< Max packet size */
  float                 m_delay = 0.02; /*!< Packet Send delay */
  Ipv4Address           m_address = Ipv4Address("0.0.0.0"); /*!< Node ip address */
  uint16_t              m_port = 20; /*!< Node communication port */
  Ipv4Address           m_server = Ipv4Address("10.1.0.1"); /*!< Common server ip address */
  Ipv4Address           m_left = Ipv4Address("0.0.0.0"); /*!< Left Node neighour ip */
  uint16_t              m_left_port = 0; /*!< Left Node neighour port */
  Ipv4Address           m_right = Ipv4Address("0.0.0.0"); /*!< Right Node neighour ip */
  uint16_t              m_right_port = 0; /*!< Right Node neighour port */
  Ptr<Packet>           m_packet = Create<Packet> (0);
  stringstream          m_data;
  map<Ptr<Socket>, void *> m_map;
  map<Ptr<Socket>, uint32_t> m_map_last_pos;

  /**
   * \param uint16_t times file request was generated on this node
   * Generate Random File Requests over time.
  **/
  void genFReqs();
  bool m_gen_traffic = true;
  EventId last_event;
  void updateFileList(void);
  void updateFileListTimed(void);

  // New params
  uint32_t n_files_available = 0;

  //Colocar isto a global algures no futuro:
  bool show_messages = false;
  bool random_client_sleep = false;
  bool use_churn = false;
  uint32_t client_fixed_sleep_duration = 10;
  uint32_t client_sleep_duration_min = 1;
  uint32_t client_sleep_duration_max = 20;

  string file_list_update_type = "server";
  uint32_t file_list_update_time = 10;


  bool m_is_running = false;
  uint32_t simulation_duration = 0;



  string file_requested = "\"File 0\"";
  string churn_intensity = "low";
  bool trace_to_file = false;
  fstream* trace_file;
  Ptr<UniformRandomVariable> randomGen;
};

#endif