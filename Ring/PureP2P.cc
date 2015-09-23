#include "PureP2P.h"
#include <inttypes.h>
#include <utility>

#include <ns3/node-list.h>

NS_LOG_COMPONENT_DEFINE ("PureP2P");

using namespace ns3;
using namespace std;

PureP2P::PureP2P()
{
	NS_LOG_DEBUG("Starting PureP2P");
}

PureP2P::~PureP2P()
{
	NS_LOG_DEBUG("Destroy PureP2P");
}

void PureP2P::StartApplication(void)
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

void PureP2P::DoWork(void){
	m_is_running = true;
	Join();
	genFReqs();
}

Ptr<Socket> PureP2P::StartSocket(void){
	Ptr<Socket> socket;
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 20);
	socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
	socket->Bind(local);
	socket->Listen();
	socket->SetAcceptCallback (MakeCallback (&PureP2P::ConnectionRequest, this),MakeCallback(&PureP2P::AcceptConnection, this));
	socket->SetRecvCallback (MakeCallback (&PureP2P::ReceivePacket, this));
	socket->SetCloseCallbacks(MakeCallback (&PureP2P::NormalClose, this), MakeCallback (&PureP2P::ErrorClose, this));
	socket->SetAllowBroadcast (true);
	NS_LOG_DEBUG(m_address << " StartSocket (" << socket << ")");
	m_map[socket] = malloc(0);
	m_map_last_pos[socket] = 0;
	return socket;
}

void PureP2P::NormalClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
}

void PureP2P::ErrorClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
}

bool PureP2P::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4());
	return true;
}

void PureP2P::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&PureP2P::ReceivePacket, this));
}

void PureP2P::ConnectSuccess(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
}

void PureP2P::ConnectFail(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
}

void PureP2P::StopApplication(void)
{
	if (m_is_running || (m_address == m_master_ip)){
		NS_ASSERT(m_discovery);
		//cout << m_address << " --->\t" << m_master_ip << endl;
		cout << m_role << " " << m_address << ": ";
		list<tuple<Ipv4Address, uint16_t>>::iterator iterator;
		for(iterator = begin(connections); iterator != end(connections); ++iterator){
			cout << get<0>(*iterator) << " ";
		}
	  cout << endl;


		//Leave();
		m_discovery->remove(m_address, m_port);
		m_discovery = NULL;
		m_socket->Close();
		m_socket = 0;
	}
	//Simulator::Remove(last_event); // Forçar remover evento para terminar logo. Senão pode demorar muito tempo a terminar.
}

void PureP2P::Setup(VirtualDiscovery *discovery, bool messages, bool churn, bool random_sleep, uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, uint32_t list_update_time, uint32_t duration, Ptr<UniformRandomVariable> rand, bool to_trace, fstream *trace, string churn_int)
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

void PureP2P::readPacket(Ptr<Socket> socket, Address from){
	//cout << m_address << " packet " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds() << endl;
	stringstream answer; /*!< iostream of a possible answer */
	stringstream request; /*!< iostream of a request */
	string param; /*!< param requested in the packet. Extracted from request. */
	bool keep = true;
	uint32_t hops = 0;
	request << string((char*)m_map[socket]);
	if (show_messages)	{
		cout << "\nRecv: " << m_address << "\tFrom: " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << "\tTime: " << Simulator::Now().GetSeconds() << endl;
		cout << string((char*)m_map[socket]); 
	}
	//cout << "Receive (final) at " << Simulator::Now().GetSeconds () << "\n" <<  string((char*)m_map[socket]) << "\n Size: " << string((char*)m_map[socket]).length() << " Socket: " << socket << endl;
	if (show_messages)
		cout << "Receive (final) at " << Simulator::Now().GetSeconds () << " Size: " << string((char*)m_map[socket]).length() << " Socket: " << socket << endl;
	while (keep){
		getline(request, param);
		NS_LOG_DEBUG(param);
		if (param == "END"){
			keep = false;
		}else if (param == "JOIN"){ // Receive a JOIN request. Take an action depending on the node status (connection number)
			answer.str(string());
			if (distance(begin(connections), end(connections)) < connections_limit){
				answer << "ACCEPT" << endl << m_role << endl << "END" << endl;
				if (use_table){
					list<tuple<Ipv4Address, uint16_t>>::const_iterator it;
					stringstream req;
					req << "DIFF\nIN\n" << InetSocketAddress::ConvertFrom (from).GetIpv4 () << "END" << endl;
					for(it = begin(connections); it != end(connections); ++it){
						Send(req, (Ipv4Address)get<0>(*it), (uint16_t)get<1>(*it));
					}
					Simulator::Schedule(Seconds(0.5), &PureP2P::TabledSendListTo, this, InetSocketAddress::ConvertFrom (from).GetIpv4 ());
				}
				connections.emplace(end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			}else{
				answer << "FULL" << endl << "END" << endl;
			}
			Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
		}else if(param == "FULL"){ // Receive a FULL request. Exclude current node, try another.
			rejected_connections.emplace(end(rejected_connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			//cout << "Rejected" << Simulator::Now().GetSeconds() << endl;
			Simulator::Schedule(Seconds(0.5), &PureP2P::Join, this);
		}else if(param == "ACCEPT"){ // The Node is willing to accept us. Lets define our role.
			getline(request, param);
			NS_ASSERT(param == "master" || param == "slave");
			if (param == "master"){
				m_role = "slave";
			}else{
				m_role = "master";
			}
			connections.emplace(end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			if (use_table){
				if (tabled_neighours.find(InetSocketAddress::ConvertFrom (from).GetIpv4 ()) == end(tabled_neighours)){
					tabled_neighours[InetSocketAddress::ConvertFrom (from).GetIpv4 ()] = vector<Ipv4Address>();
				}
			}
		}else if(param == "LEAVE"){
			NS_LOG_DEBUG(m_address << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " is leaving the network");
		}else if(param == "BCAST"){
			getline(request, param);
			uint32_t msg_uid = (uint32_t)(stoul(param) & UINT32_MAX); // Cast from unsigned long to unsigned int32
			getline(request, param);
			hops = (uint32_t)(stoul(param) & UINT32_MAX)+1;
			answer.str(string());
			answer << "BCAST\n" << msg_uid << endl << hops << endl;
			auto pos = request.tellg();
			getline(request, param);
			while (param != "END"){
				answer << param << endl;
				getline(request, param);
			}
			//cout << m_address << " BCAST " << msg_uid << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds() << endl;
			request.seekg(pos);
			answer << "END" << endl;
			keep = false;
			if (find(begin(MySentMessages), end(MySentMessages), msg_uid) == end(MySentMessages)){ // Message is not mine
				if (MessageList.find(msg_uid) == end(MessageList)){ // A new BCAST message.
					MessageList.emplace(make_pair(msg_uid, InetSocketAddress::ConvertFrom (from).GetIpv4 ()));
					if(controlHops && hops < maxHops && Simulator::Now().GetSeconds()<simulation_duration){
						BCast(reinterpret_cast<const uint8_t *>(answer.str().c_str()), (uint32_t)(answer.str().size()+1));
					}
					keep = true;
				}
			}
		}else if(param == "SEND_BACK"){
			getline(request, param);
			uint32_t msg_uid = (uint32_t)(stoul(param) & UINT32_MAX); // Cast from unsigned long to unsigned int32
			keep = false;
			const map<uint32_t, Ipv4Address>::iterator it = MessageList.find(msg_uid);
			if (find(begin(MySentMessages), end(MySentMessages), msg_uid) != end(MySentMessages)){ // Message is for me
				keep = true;
			}else if (it != end(MessageList)){
				Send(request, (*it).second, 20);
			}
		}else if(param == "DIFF"){
			getline(request, param);
			Ipv4Address from_ip = InetSocketAddress::ConvertFrom(from).GetIpv4();
			uint16_t control = 0;
			if (tabled_neighours.find(from_ip) == end(tabled_neighours))
				tabled_neighours[from_ip] = vector<Ipv4Address>();
			if (param == "IN"){
				while (param != "END" && control < connections_limit){
					getline(request, param);
					tabled_neighours[from_ip].emplace(end(tabled_neighours[from_ip]), (Ipv4Address)(param.c_str()));
					control++;
				}
			}else if(param == "OUT"){
				while (param != "END" && control < connections_limit){
					getline(request, param);
					Ipv4Address ip = (Ipv4Address)(param.c_str());
					auto it = find(begin(tabled_neighours[from_ip]), end(tabled_neighours[from_ip]), ip);
					if (it != end(tabled_neighours[from_ip])){
						tabled_neighours[from_ip].erase(it);
					}
					control++;
				}
			}
		}else if(param == "SAY"){
			getline(request, param);
			cout << m_address << ": " << param << " (" << Simulator::Now().GetSeconds() << ")" << endl;
		}else if(param == "WHO_HAS"){
			string file;
			getline(request, file);
			getline(request, param);
			Ipv4Address ip = (Ipv4Address)(param.c_str());
			getline(request, param);
			uint32_t port = atoi(param.c_str());
			// Here I should have some protocol. But for now, allways send
			if (find(begin(m_file_list), end(m_file_list), file) != end(m_file_list)){
				answer.str(string());
				answer << "SEND_FILE" << endl << file << endl << hops << "END" << endl;
				Send(answer, ip, 20);
			}
		}else if(param == "SEND_FILE"){
			getline(request, param);
			m_file_list.emplace(m_file_list.end(), param);
			string file = param;
			//cout << m_address << " requesting " << requested_file << " got " << file << " from " << InetSocketAddress::ConvertFrom(from). GetIpv4() << " " << Simulator::Now().GetSeconds() << endl;
			if (file == requested_file){
				requested_file = "\"File 0\"";
				getline(request, param);
				if (param != "END") hops = atoi(param.c_str());
				if (use_table && InetSocketAddress::ConvertFrom (from).GetIpv4 () != m_server) hops = 1;
				cout << m_address << " Got " << file << " from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " (" << Simulator::Now().GetSeconds() << ") - " << hops << endl;
			}
		}else if(param == "FILE_LIST_UPDATE"){
			getline(request, param);
			n_files_available = atoi(param.c_str());
		}else{
			keep = false;
		}
	}
	free(m_map[socket]);
	socket->Close();
}

void PureP2P::ReceivePacket (Ptr<Socket> socket)
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

void PureP2P::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, data, size);
}

void PureP2P::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void PureP2P::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}	
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void PureP2P::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i){
	if (Simulator::Now().GetSeconds() > simulation_duration) return;
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
		//cout << "Socket " << Simulator::Now().GetSeconds() << endl;
		Simulator::Schedule (Seconds(0.00001), &PureP2P::SendPacket, this, socket, data, size, i);
	}else{
		socket->Close();
	}
}

void PureP2P::Leave(void){
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
						cout << "Leave " << Simulator::Now().GetSeconds()+(next_start/1000) << endl;
						Simulator::Schedule (MilliSeconds(next_start), &PureP2P::DoWork, this);
					}
				}
			}
		}
		//Place this at the end of function when we have leader election
		m_gen_traffic = false; 
		m_is_running = false;
		Simulator::Remove(last_event);
	}else{
	}	
}

void PureP2P::Join(void){
	tuple<Ipv4Address, uint16_t> ip = m_discovery->discover();
	if (ip != make_tuple(Ipv4Address("0.0.0.0"), 0)){ // So far, the first node is allways master.
		NS_LOG_DEBUG(m_address << " is JOIN " << get<0>(ip) << ":" << get<1>(ip) << endl);
		Send("JOIN\nEND\n", get<0>(ip), get<1>(ip));
	}else{
		m_role = "master";
	}
	m_discovery->add(m_address, m_port);
}

void PureP2P::updateFileList(void){
	stringstream request;
	request << "GET_FILE_LIST" << endl << "END" << endl;
	Send(request, m_master_ip, m_master_port);
}

void PureP2P::updateNodesFileList(void){
	stringstream updateMsg;
	updateMsg << "FILE_LIST_UPDATE\n" << n_files_available << "\nEND\n";
	for(auto it = begin(m_master_node_list); it != end(m_master_node_list); it++){
		Send(updateMsg, get<0>(*it), get<1>(*it));
	}
}

void PureP2P::BCast(string message){
	uint32_t msg_uid = randomGen->GetInteger(0, UINT32_MAX);
	MySentMessages.emplace(end(MySentMessages), msg_uid);
	stringstream bcast;
	bcast << "BCAST" << endl << msg_uid << endl << message;
	BCast(reinterpret_cast<const uint8_t *>(bcast.str().c_str()), (uint32_t)(bcast.str().size()+1));
}

// In this demo, a broadcast can only reach one level of connected peers
void PureP2P::BCast(const uint8_t * data, uint32_t size){
	list<tuple<Ipv4Address, uint16_t>>::const_iterator iterator;
  for(iterator = begin(connections); iterator != end(connections); ++iterator){
    Send(data, size, get<0>(*iterator), get<1>(*iterator));
  }
}

void PureP2P::CheckFileRequests(){
	//cout << m_address << " is checking file requests for " << requested_file << " " << Simulator::Now().GetSeconds() << endl;
	if (requested_file != "\"File 0\"" || find(begin(m_file_list), end(m_file_list), requested_file) == end(m_file_list)){
		//cout << m_address << " will go to server for " << requested_file << " " << Simulator::Now().GetSeconds() << endl;
		stringstream request;
		request << "GET_FILE\n" << requested_file << "\n" << m_address << "\n" << to_string(m_port) << "\nEND\n";
		Send(request, m_server, m_server_port);
	}
}

void PureP2P::genFReqs(){
	// Next file request
	const uint32_t left = floor(simulation_duration - Simulator::Now().GetSeconds())-delay;
	if (left > 5){
		uint32_t next = randomGen->GetInteger(max(5, (int)ceil(delay)), min(left, (const uint32_t)60));
		//cout << "FReqs " << Simulator::Now().GetSeconds()+next << endl;
		Simulator::Schedule(Seconds(next), &PureP2P::genFReqs, this);
	}
	if (n_files_available == 0 || requested_file != "\"File 0\"" || (Simulator::Now().GetSeconds()+5)>simulation_duration) return;
	requested_file = string("\"File ")+to_string(randomGen->GetInteger(1, n_files_available))+ string("\"");
	stringstream request;
	cout << m_address << " Want " << requested_file << " (" << Simulator::Now().GetSeconds() << ")" << endl;
	if (use_table){
		map<Ipv4Address, vector<Ipv4Address>>::const_iterator it;
		for (it = begin(tabled_neighours); it != end(tabled_neighours); it++){
			request << "WHO_HAS" << endl << requested_file << endl << m_address << endl << m_port << endl << "END" << endl;
			Send(request, (Ipv4Address)it->first, 20);
			vector<Ipv4Address>::const_iterator sub_it;
			for (sub_it = begin(it->second); sub_it != end(it->second); sub_it++){
				Send(request, (Ipv4Address)(*sub_it), 20);
			}
		}
	}else{
		request << (uint32_t)(0) << endl << "WHO_HAS" << endl << requested_file << endl << m_address << endl << m_port << endl << "END" << endl;
		BCast(request.str());
	}
	//cout << "GenTimeout " << Simulator::Now().GetSeconds() + static_cast<uint64_t>(delay) << endl;
	Simulator::Schedule(MilliSeconds(static_cast<uint64_t>(delay*1000)), &PureP2P::CheckFileRequests, this);
}

void PureP2P::TabledSendListTo(Ipv4Address to){
	if (tabled_neighours.find(to) == end(tabled_neighours))
		tabled_neighours[to] = vector<Ipv4Address>();
	list<tuple<Ipv4Address, uint16_t>>::const_iterator it;
	stringstream req;
	req << "DIFF\nIN\n";
	for(it = begin(connections); it != end(connections); ++it){
		if ((Ipv4Address)get<0>(*it) != to){
			req << get<0>(*it) << endl;
		}
  }
  req << "END" << endl;
  Send(req, to, 20);
}