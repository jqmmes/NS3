#include "MyServer.h"

NS_LOG_COMPONENT_DEFINE ("MyServer");

using namespace ns3;
using namespace std;

void Receive_Packet (Ptr<Socket> socket);

MyServer::MyServer()
{
  NS_LOG_DEBUG("Starting MyServer");
}

MyServer::~MyServer()
{
  NS_LOG_DEBUG("Destroy MyServer");
}

void MyServer::StartApplication(void){
  NS_LOG_FUNCTION (this);

  m_node = GetNode();

  updateFList();

  if (m_socket == 0){ m_socket = StartSocket(9); }
}

Ptr<Socket> MyServer::StartSocket(uint16_t port){
  Ptr<Socket> socket;
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);
  socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
  socket->Bind(local);
  socket->Listen();
  socket->SetAcceptCallback (MakeCallback (&MyServer::ConnectionRequest, this),MakeCallback(&MyServer::AcceptConnection, this));
  socket->SetRecvCallback (MakeCallback (&MyServer::ReceivePacket, this));
  socket->SetCloseCallbacks(MakeCallback (&MyServer::NormalClose, this), MakeCallback (&MyServer::ErrorClose, this));
  socket->SetAllowBroadcast (true);
  NS_LOG_DEBUG("10.1.1.1 StartSocket (" << socket << ")");
  m_map[socket] = malloc(0);
  m_map_last_pos[socket] = 0;
  return socket;
}

void MyServer::NormalClose(Ptr<Socket> socket){
  NS_LOG_DEBUG("10.1.1.1 NormalClose (" << socket << ")");
}

void MyServer::ErrorClose(Ptr<Socket> socket){
  NS_LOG_DEBUG("10.1.1.1 ErrorClose (" << socket << ")");
}

bool MyServer::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from){
  NS_LOG_DEBUG("10.1.1.1 ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4());
  return true;
}

void MyServer::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from){
  NS_LOG_DEBUG("10.1.1.1 AcceptConnection (" << socket << ")");
  socket->SetRecvCallback (MakeCallback (&MyServer::ReceivePacket, this));
}

void MyServer::ConnectSuccess(Ptr<Socket> socket){
  NS_LOG_DEBUG("10.1.1.1 ConnectSuccess (" << socket << ")");
}

void MyServer::ConnectFail(Ptr<Socket> socket){
  NS_LOG_DEBUG("10.1.1.1 ConnectFail (" << socket << ")");
}
void MyServer::StopApplication(){
  m_socket->Close();
  m_gen_traffic = false;
  Simulator::Remove(last_event);
  Simulator::Stop();
  //cout << "SERVER Stoping (" << Simulator::Now().GetSeconds() << ")" << endl;
}

void
MyServer::Setup (bool messages, bool random_file_inc, bool random_sleep, uint32_t fixed_file_increase, uint32_t initial_file_count, uint32_t file_increase_min, 
                uint32_t file_increase_max, uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, 
                string application, bool trace_to, fstream *trace, Ptr<UniformRandomVariable> rand)
{
  show_messages = messages;
  random_server_file_inc = random_file_inc;
  random_server_sleep = random_sleep;
  server_fixed_file_increase = fixed_file_increase;
  server_file_increase_min = file_increase_min; 
  server_file_increase_max = file_increase_max;
  server_sleep_duration_min = sleep_duration_min;
  server_sleep_duration_max = sleep_duration_max;
  server_fixed_sleep_duration = fixed_sleep_duration;

  if(random_server_sleep) {
    if(!(server_sleep_duration_max > server_sleep_duration_min)){
      cout << "Using: random_server_sleep" << endl << "Condition: \"server_sleep_duration_max > server_sleep_duration_min\" not met" << endl;
    }
    NS_ASSERT(server_sleep_duration_max > server_sleep_duration_min); //Terminate execution
  }
  if(random_server_file_inc) {
    if(!(server_file_increase_max > server_file_increase_min)){
      cout << "Using: random_server_file_inc" << endl << "Condition: \"server_file_increase_max > server_file_increase_min\" not met." << endl;
    }
    NS_ASSERT(server_file_increase_max > server_file_increase_min); //Terminate execution
  }

  file_list_update_type = list_update_type;

  m_application = application;

  trace_to_file = trace_to;

  if(trace_to_file) trace_file = trace;

  stringstream s;
  for(uint16_t k=1; k <= initial_file_count; k++){
    s << "\"File " << (k) << "\"";
    m_file_list.emplace(m_file_list.end(), s.str());
    s.str("");
    s.clear();
  }

  randomGen = rand;
}

void MyServer::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
  Ptr<Socket> socket = StartSocket(9);
  if (port){
    socket->Connect (InetSocketAddress (d_address, port));
  }else{
    socket->Connect (InetSocketAddress (d_address));
  }
  SendPacket(socket, data, size);
}

void MyServer::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
  Ptr<Socket> socket = StartSocket(9);
  if (port){
    socket->Connect (InetSocketAddress (d_address, port));
  }else{
    socket->Connect (InetSocketAddress (d_address));
  }
  SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void MyServer::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
  Ptr<Socket> socket = StartSocket(9);
  if (port){
    socket->Connect (InetSocketAddress (d_address, port));
  }else{
    socket->Connect (InetSocketAddress (d_address));
  } 
  SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void MyServer::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i){
  void * p_data = malloc(0);
  uint32_t c_size = min(m_packet_size, size-i);
  bool err = false;
  while((socket->GetTxAvailable() > c_size) && (i < size))
  // Não deixar esgotar o buffer em envios muito grandes.
  {
    p_data = realloc(p_data, c_size*sizeof(uint8_t));
    memcpy(p_data, &data[i], c_size*sizeof(uint8_t));
    if ((socket->Send(Create<Packet> (reinterpret_cast<const uint8_t *>(p_data), c_size)) >= 0)){
      NS_LOG_DEBUG("Send Success");
    }else{
      NS_LOG_DEBUG("Send Fail (" << socket->GetErrno () << ")");
      err = true;
      break;
    }
    i += c_size;
    c_size = min(m_packet_size, size-i);
  }
  if (i < size && !err)
  {
    //cout << "Server: send_pkt " << Simulator::Now().GetSeconds() << endl;
    Simulator::Schedule (Seconds(0.00001), &MyServer::SendPacket, this, socket, data, size, i);
  }else{
    socket->Close();
  }
}

void MyServer::readPacket(Ptr<Socket> socket, Address from){
  stringstream answer; /*!< iostream of a possible answer */
  stringstream request; /*!< iostream of a request */
  string param; /*!< param requested in the packet. Extracted from request. */
  bool keep = true;
  request << string((char*)m_map[socket]);
  if (show_messages){
    cout << "\nRecv: Server\tFrom: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << "\tTime: " << Simulator::Now().GetSeconds() << endl;
    cout << string((char*)m_map[socket]); 
  }
  getline(request, param);
  if(show_messages)
    cout << "Receive (final) at " << Simulator::Now().GetSeconds () << " Size: " << string((char*)m_map[socket]).length() << " Socket: " << socket << endl;
  while (keep){
    if (param == "END"){
      keep = false;
    }else if(param == "GET_FILE"){
      //cout << "Server got request to GET_FILE from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds () << endl;
      getline(request, param);
      string fid = (string)(param.c_str());
      if (m_application == "ring"){
        getline(request, param);
        Ipv4Address reqFrom = (Ipv4Address)(param.c_str());
        getline(request, param);
        uint32_t hops = (uint32_t)(atoi(param.c_str()));
      }
      auto fP = find(begin(m_file_list), end(m_file_list), fid);
      if(fP!=end(m_file_list)){
        answer.str(string());
        answer << "SEND_FILE" << endl;
        answer << fid << endl;
        if (m_application == "ring") answer << 0 << endl;
        answer << "END" << endl;
        //Send(answer, reqFrom, 20);  
        //cout << "Server is sending " << fid << " to " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds () << endl;
        Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
        if(trace_to_file){
          stringstream msg;
          msg << "server, " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << ", " << fid << ", 0, " << Simulator::Now().GetSeconds () << endl;
          trace_file->write(msg.str().c_str(), msg.str().size()+1);
          trace_file->flush();
        }
        continue;
      }else{
        answer.str(string());
        answer << "FILE_NOT_FOUND" << endl;
        answer << fid << endl;
        answer << "END" << endl;
        //Send(answer, reqFrom, 20);
        Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
        continue;
      }
    }else if(param == "GET_FILE_LIST"){
      getline(request, param);
      Ipv4Address reqFrom = (Ipv4Address)(param.c_str());
      stringstream answer;
      answer << "FILE_LIST_UPDATE" << endl << distance(m_file_list.begin(), m_file_list.end()) <<  endl << "END" << endl;
      Send(answer, reqFrom, 20);
    }else if (param == "I_AM_MASTER"){ // Used in star application.
      m_star_master_ip = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
    }else{
      keep = false;
    }
    getline(request, param);
  }
  free(m_map[socket]);
  socket->Close();
}

void MyServer::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> p; /*!< Received packet */
  Address from; /*!< Origin address of the packet*/
  void *local;

  while(socket->GetRxAvailable () > 0){
    p = socket->RecvFrom (from);//socket->Recv (socket->GetRxAvailable (), 0);
    m_map[socket] = realloc(m_map[socket], m_map_last_pos[socket]+(sizeof(uint8_t)*p->GetSize()));
    p->CopyData (reinterpret_cast<uint8_t *> (m_map[socket])+m_map_last_pos[socket], p->GetSize()); 
    local = malloc(p->GetSize()*sizeof(uint8_t));
    p->CopyData (reinterpret_cast<uint8_t *>(local), p->GetSize());
    m_map_last_pos[socket] += sizeof(uint8_t)*p->GetSize();

    // If the message as been fully received, lets process the request.
    // To check if it's complete lets check if:
    if (string(reinterpret_cast<char *>(local)).length() >= 4){
      if (string(reinterpret_cast<char *>(local)).rfind("END\n") == string(reinterpret_cast<char *>(local)).length()-4){
        readPacket(socket, from);
      }
    }else{
      if (string((char*)m_map[socket]).rfind("END\n") == string((char*)m_map[socket]).length()-4){
        readPacket(socket, from);
      }
    }
    free(local);
  }
}

void MyServer::BCast(const uint8_t * data, uint32_t size){
  uint32_t nnodes = NodeList::GetNNodes();
  uint32_t k;
  // k=1, since first node is allways the server
  for(k=1; k < nnodes; k++){
    Ptr<Ipv4> ipv4 = NodeList::GetNode(k)->GetObject<Ipv4>();
    Send(data, size, ipv4->GetAddress(1,0).GetLocal(), 20);
  }
}

void MyServer::BCast(string sdata){
  BCast(reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void MyServer::BCast(stringstream& sdata){
  BCast(reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void MyServer::updateFList(void){
  stringstream s;
  uint16_t file_increase;
  uint32_t n_items = distance(m_file_list.begin(), m_file_list.end());
  if (random_server_file_inc){
    file_increase = (randomGen->GetInteger(1,(server_file_increase_max-server_file_increase_min)) + server_file_increase_min);
  }else{
    file_increase = server_fixed_file_increase;
  }
  if (show_messages) cout << "Server (Adding Files):" << endl;
  for (uint16_t k = 1; k <= file_increase; k++){
    s << "\"File " << (n_items + k) << "\"";
    if (show_messages) cout << s.str() << endl;
    m_file_list.emplace(m_file_list.end(), s.str());
    s.str("");
    s.clear();
  }
  if ((file_list_update_type == "server" && (m_application == "ring" || m_application == "purep2p" || m_application == "wellformed")) || (file_list_update_type == "server_bcast" && m_application == "star")){
      stringstream warn;
      warn << "FILE_LIST_UPDATE" << endl << n_items+file_increase << endl << "END" << endl;
      BCast(warn);
  }if(file_list_update_type != "server_bcast" && m_application == "star" && m_star_master_ip != Ipv4Address("0.0.0.0")){
    stringstream warn;
    warn << "FILE_LIST_UPDATE" << endl << n_items+file_increase << endl << "END" << endl;
    Send(warn, m_star_master_ip, 20);
  }
  if (m_gen_traffic){
    if (random_server_sleep){
      uint32_t next = randomGen->GetInteger(1,(server_sleep_duration_max - server_sleep_duration_min) + server_sleep_duration_min);
      //cout << "Server: gen_traffic " << Simulator::Now().GetSeconds() << endl;
      last_event = Simulator::Schedule (Seconds(next), &MyServer::updateFList, this);
    }
    else{
      //cout << "Server: gen_traffic " << Simulator::Now().GetSeconds() << endl;
      last_event = Simulator::Schedule (Seconds(20), &MyServer::updateFList, this); // In 20 seconds insert again
    }
  }
}