#include "SimpleStar.h"
#include <inttypes.h>
#include <utility>
#include "BTInterface.h"

#include <ns3/node-list.h>

NS_LOG_COMPONENT_DEFINE ("SimpleStar");

using namespace ns3;
using namespace std;

SimpleStar::SimpleStar()
{
	NS_LOG_DEBUG("Starting SimpleStar");
}

SimpleStar::~SimpleStar()
{
	NS_LOG_DEBUG("Destroy SimpleStar");
}

void SimpleStar::StartApplication(void)
{
	Ptr<Node> m_node = GetNode();

	Ptr<NetDevice> net = m_node->GetDevice(0);
	Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
	m_address = ipv4->GetAddress(1,0).GetLocal();

	NS_ASSERT(m_discovery);
	NS_LOG_DEBUG("Start: " << m_address);


	// Join
	if (m_socket == 0)
	{
		m_socket = StartSocket();
		NS_LOG_DEBUG(m_address << " m_socket: " << m_socket);
	}

	DoWork();
}

void SimpleStar::DoWork(void){
	m_is_running = true;
	Join();
	if(file_list_update_type == "master_client_timed" && m_master_ip == m_address){
		updateFileListTimed(true);
	}
	else if(file_list_update_type == "client_master_timed" && m_master_ip != m_address){
		updateFileListTimed(true);	
	}
	if(use_churn) {
		if(!Simulator::IsFinished()){
			uint32_t left = simulation_duration - Simulator::Now().GetSeconds() - 5;
			if (left > 0){ 
				uint32_t next_stop;
				if (churn_intensity == "low")	next_stop = randomGen->GetInteger(1,2*left);
				else if (churn_intensity == "medium") next_stop = randomGen->GetInteger(1,1.5*left);
				else next_stop = randomGen->GetInteger(1,left);
				if ((Simulator::Now().GetSeconds()+next_stop) < (simulation_duration-5)){
					Simulator::Schedule (MilliSeconds(next_stop*1000), &SimpleStar::Leave, this);
				}
			}
		}
	}
	if (file_list_update_type == "client_timed") updateFileListTimed();
	genFReqs();
}

Ptr<Socket> SimpleStar::StartSocket(void){
	Ptr<Socket> socket;
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 20);
	socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
	socket->Bind(local);
	socket->Listen();
	socket->SetAcceptCallback (MakeCallback (&SimpleStar::ConnectionRequest, this),MakeCallback(&SimpleStar::AcceptConnection, this));
	socket->SetRecvCallback (MakeCallback (&SimpleStar::ReceivePacket, this));
	socket->SetCloseCallbacks(MakeCallback (&SimpleStar::NormalClose, this), MakeCallback (&SimpleStar::ErrorClose, this));
	socket->SetAllowBroadcast (true);
	NS_LOG_DEBUG(m_address << " StartSocket (" << socket << ")");
	m_map[socket] = malloc(0);
	m_map_last_pos[socket] = 0;
	return socket;
}

void SimpleStar::NormalClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
}

void SimpleStar::ErrorClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
}

bool SimpleStar::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4());
	return true;
}

void SimpleStar::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&SimpleStar::ReceivePacket, this));
}

void SimpleStar::ConnectSuccess(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
}

void SimpleStar::ConnectFail(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
}

void SimpleStar::StopApplication(void)
{
	if (m_is_running || (m_address == m_master_ip)){
		NS_ASSERT(m_discovery);
		cout << m_address << " --->\t" << m_master_ip << endl;
		Leave();
		m_discovery->remove(m_address, m_port);
		m_discovery = NULL;
		m_socket->Close();
		m_socket = 0;
	}
	//Simulator::Remove(last_event); // Forçar remover evento para terminar logo. Senão pode demorar muito tempo a terminar.
}

void SimpleStar::Setup(VirtualDiscovery *discovery, bool messages, bool churn, bool random_sleep, uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, uint32_t list_update_time, uint32_t duration, Ptr<UniformRandomVariable> rand, bool to_trace, fstream *trace, string churn_int)
{
	m_discovery = discovery;
	show_messages = messages;
	random_client_sleep = random_sleep;
	client_sleep_duration_min = sleep_duration_min;
	client_sleep_duration_max = sleep_duration_max;
	client_fixed_sleep_duration = fixed_sleep_duration;

  if(random_client_sleep) {
    if(!(client_sleep_duration_max > client_sleep_duration_min)){
      cout << "Using: random_client_sleep" << endl << "Condition: \"server_client_duration_max > server_client_duration_min\" not met" << endl;
    }
    NS_ASSERT(client_sleep_duration_max > client_sleep_duration_min); //Terminate execution
  }
  file_list_update_type = list_update_type;
  file_list_update_time = list_update_time;

  simulation_duration = duration;
  use_churn = churn;

  churn_intensity = churn_int;

  trace_to_file = to_trace;
  trace_file = trace;
  randomGen = rand;  
}

void SimpleStar::readPacket(Ptr<Socket> socket, Address from){
	stringstream answer; /*!< iostream of a possible answer */
	stringstream request; /*!< iostream of a request */
	string param; /*!< param requested in the packet. Extracted from request. */
	bool keep = true;
	request << string((char*)m_map[socket]);
	if (show_messages)	{
		cout << "\nRecv: " << m_address << "\tFrom: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << "\tTime: " << Simulator::Now().GetSeconds() << endl;
		cout << string((char*)m_map[socket]); 
	}
	getline(request, param);
	if (show_messages)
		cout << "Receive (final) at " << Simulator::Now().GetSeconds () << " Size: " << string((char*)m_map[socket]).length() << " Socket: " << socket << endl;
	while (keep){
		NS_LOG_DEBUG(param);
		//cout << param << endl;
		if (param == "END"){
			keep = false;
		}else if (param == "JOIN"){
			answer.str(string());
			answer << "ACCEPT_JOIN" << endl << m_master_ip << endl << m_master_port << endl << "END" << endl;
			Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
			m_master_node_list.emplace(end(m_master_node_list), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
		}else if(param == "LEAVE"){
			NS_LOG_DEBUG(m_address << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " is leaving the network");
		}else if(param == "ACCEPT_JOIN"){
			char * endptr;
			getline(request, param);
			m_master_ip = (Ipv4Address)(param.c_str());
			getline(request, param);
			m_master_port = (uint16_t)strtoimax(param.c_str(), &endptr, 10);
		}else if(param == "REQUEST_FILE"){
			answer.str(string());
			getline(request, param);
			string fid = (string)(param.c_str());
			if (m_address == m_master_ip){
				auto it = m_master_file_list.find(fid);
				if (it != m_master_file_list.end()){
					vector<tuple<Ipv4Address, uint16_t>> & tmp_data = it->second;
					uint32_t size = distance(begin(tmp_data), end(tmp_data));
					uint32_t i = randomGen->GetInteger(0,size-1);
					tuple<Ipv4Address, uint16_t> node = tmp_data.at(i);
					answer << "REQUEST_FILE_TO" << endl;
					answer << fid << endl;
					answer << get<0>(node) << endl;
					answer << get<1>(node) << endl;
					answer << "END" << endl;
				}else{
					answer << "GOTO_SERVER" << endl;
					answer << fid << endl;
					answer << "END" << endl;
				}
				Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
			}
		}else if(param == "GET_FILE"){
			answer.str(string());
			getline(request, param);
			auto fP = find(begin(m_file_list), end(m_file_list), param);
			if(fP!=end(m_file_list)){
				answer << "SEND_FILE" << endl;
				answer << param << endl;
				answer << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
        if(trace_to_file){
          stringstream msg;
          msg << m_address << ", " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << ", " << param << ", 2, " << Simulator::Now().GetSeconds () << endl;
          trace_file->write(msg.str().c_str(), msg.str().size()+1);
          trace_file->flush();
        }
			}else{
				answer << "GOTO_SERVER" << endl;
				answer << param << endl;
				answer << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
				if (m_address != m_master_ip){
					answer.str(string());
					answer << "DONT_HAVE_FILE" << endl;
					answer << param << endl;
					answer << "END" << endl;
					Send(answer, m_master_ip, m_master_port);
				}else{
					// remove file from my list...
				}
			}
		}else if(param == "SEND_FILE"){
			getline(request, param);
			m_file_list.emplace(m_file_list.end(), param);
			string file = param;
			getline(request, param);
			//uint32_t hops = (uint32_t)(atoi(param.c_str()));
			//cout << m_address << " Got " << file << " from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " (" << hops << " hops)" << endl;
			if (file == file_requested){
				file_requested = "\"File 0\"";
				cout << m_address << " Got " << file << " from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " - " << Simulator::Now().GetSeconds() << endl;
				answer << "I_HAVE_FILE" << endl;
				answer << file << endl;
				answer << "END" << endl;
				Send(answer, m_master_ip, m_master_port);
			}
		}else if(param == "GOTO_SERVER"){
			getline(request, param);
			answer.str(string());
			answer << "GET_FILE" << endl;
			answer << param << endl;
			answer << "END" << endl;
			Send(answer, m_server, m_server_port);
		}else if(param == "DONT_HAVE_FILE"){ //Não deve ser preciso, mas....
			getline(request, param);
			if (m_address == m_master_ip){
				auto it = m_master_file_list.find(param);
				if(it != m_master_file_list.end()){
					vector<tuple<Ipv4Address, uint16_t>> & tmp_data = it->second;
					Ipv4Address old_ip = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
					auto fp = find(begin(tmp_data), end(tmp_data), make_tuple(old_ip, 20));
					// If the data is in the array, remove.
					if (fp != end(tmp_data)){
						tmp_data.erase(fp);
					}
					// If there are no nodes with this file.
					if (begin(tmp_data) == end(tmp_data)){
						m_master_file_list.erase(it);
					}
				}
			}
		}else if(param == "REQUEST_FILE_TO"){
			char * endptr;
			getline(request, param);
			string fid = param;
			getline(request, param);
			Ipv4Address reqFrom = (Ipv4Address)(param.c_str());
			getline(request, param);
			uint16_t port = (uint16_t)strtoimax(param.c_str(), &endptr, 10);
			answer.str(string());
			answer << "GET_FILE" << endl;
			answer << fid << endl;
			answer << "END" << endl;
			Send(answer, reqFrom, port);
		}else if(param == "I_HAVE_FILE"){
			getline(request, param);
			if (m_address == m_master_ip){
				auto it = m_master_file_list.find(param);
				if (it != m_master_file_list.end()){
					vector<tuple<Ipv4Address, uint16_t>> & tmp_data = it->second;
					auto fp = find(begin(tmp_data), end(tmp_data), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
					if (fp != end(tmp_data)){
						tmp_data.emplace(end(tmp_data), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
					}
				}else{
					vector<tuple<Ipv4Address, uint16_t>> b;
					b.emplace(end(b), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
					m_master_file_list.emplace(make_pair(param, b));
				}
			}
		}else if(param == "FILE_LIST_UPDATE"){
			getline(request, param);
			n_files_available = atoi(param.c_str());
			if(file_list_update_type == "master_client_on_update" && m_master_ip == m_address){
				updateNodesFileList();
			}
		}else if(param == "GET_FILE_LIST"){
			if (m_address == m_master_ip){
      	stringstream answer;
      	answer << "FILE_LIST_UPDATE" << endl << n_files_available <<  endl << "END" << endl;
      	Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
      }
    }else{
			keep = false;
		}
		getline(request, param);
	}
	free(m_map[socket]);
	socket->Close();
}

void SimpleStar::ReceivePacket (Ptr<Socket> socket)
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

void SimpleStar::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, data, size);
}

void SimpleStar::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void SimpleStar::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}	
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void SimpleStar::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i){
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
		//Let's hope no send if really, really big. (Shouldn't be)
		Simulator::Schedule (Seconds(0.00001), &SimpleStar::SendPacket, this, socket, data, size, i);
	}else{
		socket->Close();
	}
}

bool SimpleStar::ReqFile(string fid, Ipv4Address reqAddress, bool toServer){
	auto fp = find(begin(m_file_list), end(m_file_list), fid);
	if (fp != end(m_file_list)){ // I already have the file...
		return false;
	}
	stringstream request;
	if (m_master_ip == m_address){ // I'm master
		request << "GET_FILE" << endl << fid << endl << "END" << endl;
		auto it = m_master_file_list.find(fid);
		if (it != m_master_file_list.end()){ // Someone has the file
			vector<tuple<Ipv4Address, uint16_t>> ips = it->second;
			uint32_t n_elem = distance(begin(ips), end(ips));
			//tuple<Ipv4Address, uint16_t> to_ask = ips.at(rand() % n_elem);		
			tuple<Ipv4Address, uint16_t> to_ask = ips.at(randomGen->GetInteger(0,n_elem-1));		
			Send(request, get<0>(to_ask), get<1>(to_ask));
		}
		else{ // Nobody has the file. Ask server
			Send(request, m_server, m_server_port);
		}
	}else{
		request << "REQUEST_FILE" << endl << fid << endl << "END" << endl;
		Send(request, m_master_ip, m_master_port);
	}
	return true;
}

void SimpleStar::Leave(void){
	cout << m_address << " LEAVING (" << Simulator::Now().GetSeconds() << ")" << endl;
	stringstream answer;
	if (m_address != m_master_ip){
		answer << "LEAVE" << endl;
		answer << "END" << endl;
		Send(answer, m_master_ip, m_master_port);
		if(use_churn) {
			if(!Simulator::IsFinished()){
				uint32_t left = simulation_duration - Simulator::Now().GetSeconds() - 5;
				if (left > 0){ //Can't `mod 0`. Even if could, if there's 0 seconds left, terminate.
					uint32_t next_start;
					if (churn_intensity == "low")	next_start = randomGen->GetInteger(1,2*left);
					else if (churn_intensity == "medium") next_start = randomGen->GetInteger(1,1.5*left);
					else next_start = randomGen->GetInteger(1,left);
					if ((Simulator::Now().GetSeconds()+next_start) < (simulation_duration-5)){
						Simulator::Schedule (MilliSeconds(next_start)*1000, &SimpleStar::DoWork, this);
					}
				}
			}
		}
		//Place this at the end of function when we have leader election
		file_requested = "\"File 0\"";
		m_gen_traffic = false; 
		m_is_running = false;
		Simulator::Remove(last_event);
	}else{ // I'm master and I'm leaving. What now?
		// Lider election algorithm, or something...
		// Can chose a random node and make it master.
		// Can have some kind of vote...
	}	
}

void SimpleStar::Join(void){
	cout << m_address << " JOINING (" << Simulator::Now().GetSeconds() << ")" << endl;
	tuple<Ipv4Address, uint16_t> ip = m_discovery->discover();
	if (ip != make_tuple(Ipv4Address("0.0.0.0"), 0)){ // So far, the first node is allways master.
		NS_LOG_DEBUG(m_address << " is JOIN " << get<0>(ip) << ":" << get<1>(ip) << endl);
		Send("JOIN\nEND\n", get<0>(ip), get<1>(ip));
	}else{
		m_master_ip = m_address;
		m_master_port = m_port;
		Send("I_AM_MASTER\nEND\n", m_server, m_server_port);
	}
	m_discovery->add(m_address, m_port);
}

void SimpleStar::updateFileList(void){
	stringstream request;
	request << "GET_FILE_LIST" << endl << "END" << endl;
	Send(request, m_master_ip, m_master_port);
}

void SimpleStar::updateNodesFileList(void){
	stringstream updateMsg;
	updateMsg << "FILE_LIST_UPDATE\n" << n_files_available << "\nEND\n";
	for(auto it = begin(m_master_node_list); it != end(m_master_node_list); it++){
		Send(updateMsg, get<0>(*it), get<1>(*it));
	}
}

void SimpleStar::updateFileListTimed(bool master){
	if(master){
		updateNodesFileList();
	}else{
		updateFileList();
	}
	if (m_gen_traffic){
		if (Simulator::Now().GetSeconds()+file_list_update_time < simulation_duration-5){
			Simulator::Schedule (Seconds(file_list_update_time), &SimpleStar::updateFileListTimed, this, master);	
		}
	}
}

void SimpleStar::genFReqs(){
	if (file_list_update_type == "client_master_on_request" && m_address != m_master_ip){
		if (show_messages) cout << m_address << ": Updating File List" << endl;
		updateFileList();
	}
	stringstream s;
	if (n_files_available > 0 && file_requested == "\"File 0\""){
		s << "\"File " << randomGen->GetInteger(1,n_files_available) << "\"";
		if (find(begin(m_file_list), end(m_file_list), s.str()) == end(m_file_list)){
			file_requested = s.str();
			cout << file_requested << endl;
			ReqFile(s.str(), m_address);
		}
	}
	if (m_gen_traffic){
    if (random_client_sleep){
      uint32_t next = randomGen->GetInteger(0,client_sleep_duration_max - client_sleep_duration_min + client_sleep_duration_min);
      cout << next << endl;
      if (Simulator::Now().GetSeconds()+next < simulation_duration-5){
      	last_event = Simulator::Schedule (Seconds(next), &SimpleStar::genFReqs, this);
      }else{
      	m_gen_traffic = false;
      }
    }
    else{
    	if (Simulator::Now().GetSeconds()+client_fixed_sleep_duration < simulation_duration-5){
      	last_event = Simulator::Schedule (Seconds(client_fixed_sleep_duration), &SimpleStar::genFReqs, this);
    	}else{
      	m_gen_traffic = false;
      }
    }
	}
}
