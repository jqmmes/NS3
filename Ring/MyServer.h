#ifndef MYSERVER_H
#define MYSERVER_H

#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include <climits>

using namespace ns3;
using namespace std;

class MyServer : public Application
{
public:

  MyServer ();
  virtual ~MyServer ();
  void Setup (bool messages, bool random_file_inc, bool random_sleep, uint32_t fixed_file_increase, uint32_t initial_file_count, uint32_t file_increase_min, uint32_t file_increase_max, 
              uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, string application, bool trace_to, fstream *trace, Ptr<UniformRandomVariable> rand);
  void TraceToFile(string file_name);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // Features
  vector<string> m_file_list;


  // Private
  bool ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from);
  void AcceptConnection(Ptr<Socket> socket, const ns3::Address& from);
  void ConnectSuccess(Ptr<Socket> socket);
  void ConnectFail(Ptr<Socket> socket);
  void NormalClose(Ptr<Socket> socket);
  void ErrorClose(Ptr<Socket> socket);
  Ptr<Socket> StartSocket(uint16_t port);

  void Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port = 0);
  void Send(string sdata, Ipv4Address d_address, uint16_t port = 0);
  void Send(stringstream& sdata, Ipv4Address d_address, uint16_t port = 0);
  void BCast(const uint8_t * data, uint32_t size);
  void BCast(string sdata);
  void BCast(stringstream& sdata);
  void ReceivePacket (Ptr<Socket> socket);
  void SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i = 0);


  void readPacket(Ptr<Socket> socket, Address from);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  Ptr<Node>       m_node;
  uint32_t        m_packetSize;
  uint32_t              m_packet_size = 1000; /*!< Max packet size */
  map<Ptr<Socket>, void *> m_map;
  map<Ptr<Socket>, uint32_t> m_map_last_pos;


  void updateFList(void);
  bool m_gen_traffic = true;
  EventId last_event;

  // Colocar global num futuro...
  bool show_messages = false;
  bool trace_to_file = false;
  bool random_server_file_inc = false;
  bool random_server_sleep = false;

  uint32_t server_fixed_file_increase = 5;
  uint32_t server_file_increase_min = 1;
  uint32_t server_file_increase_max = 10;
  uint32_t server_sleep_duration_min = 1;
  uint32_t server_sleep_duration_max = 20;
  uint32_t server_fixed_sleep_duration = 20;

  string file_list_update_type = "server";

  string m_application = "ring";

  fstream *trace_file;

  Ipv4Address m_star_master_ip = Ipv4Address("0.0.0.0");

  Ptr<UniformRandomVariable> randomGen;
};

#endif