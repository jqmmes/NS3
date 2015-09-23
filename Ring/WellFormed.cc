#include "WellFormed.h"
#include <inttypes.h>
#include <utility>

#include <ns3/node-list.h>

NS_LOG_COMPONENT_DEFINE ("WellFormed");

using namespace ns3;
using namespace std;

WellFormed::WellFormed()
{
	NS_LOG_DEBUG("Starting WellFormed");
}

WellFormed::~WellFormed()
{
	NS_LOG_DEBUG("Destroy WellFormed");
}

void WellFormed::StartApplication(void)
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

void WellFormed::DoWork(void){
	m_is_running = true;
	Join();
	genFReqs();
}

Ptr<Socket> WellFormed::StartSocket(void){
	Ptr<Socket> socket;
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 20);
	socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
	socket->Bind(local);
	socket->Listen();
	socket->SetAcceptCallback (MakeCallback (&WellFormed::ConnectionRequest, this),MakeCallback(&WellFormed::AcceptConnection, this));
	socket->SetRecvCallback (MakeCallback (&WellFormed::ReceivePacket, this));
	socket->SetCloseCallbacks(MakeCallback (&WellFormed::NormalClose, this), MakeCallback (&WellFormed::ErrorClose, this));

	socket->SetConnectCallback(MakeCallback (&WellFormed::connectionSucceeded, this), MakeCallback (&WellFormed::connectionFailed, this));
	socket->SetDataSentCallback(MakeCallback (&WellFormed::dataSent, this));
	socket->SetSendCallback(MakeCallback (&WellFormed::sendCb, this));


	socket->SetAllowBroadcast (true);
	NS_LOG_DEBUG(m_address << " StartSocket (" << socket << ")");
	m_map[socket] = malloc(0);
	m_map_last_pos[socket] = 0;
	return socket;
}

void WellFormed::sendCb(Ptr<Socket> socket, uint32_t size){
	if (debugAll)
		cout << m_address << " SendCb (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
}

void WellFormed::connectionSucceeded(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " ConnectionSucceeded (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
}

void WellFormed::connectionFailed(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " ConnectionFailed (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
}

void WellFormed::dataSent(Ptr<Socket> socket, uint32_t size){
	inSending = false;
	if (debugAll)
		cout << m_address << " DataSent (" << socket << ")" << " at " << Simulator::Now().GetSeconds()<< endl;
}

void WellFormed::NormalClose(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " NormalClose (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
}

void WellFormed::ErrorClose(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " ErrorClose (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
}

bool WellFormed::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from){
	if (debugAll)
		cout << m_address << " ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4() << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4());
	return true;
}

void WellFormed::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from){
	if (debugAll)
		cout << m_address << " AcceptConnection (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&WellFormed::ReceivePacket, this));
}

void WellFormed::ConnectSuccess(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " ConnectSuccess (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
}

void WellFormed::ConnectFail(Ptr<Socket> socket){
	if (debugAll)
		cout << m_address << " ConnectFail (" << socket << ")" << " at " << Simulator::Now().GetSeconds() << endl;
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
}

void WellFormed::StopApplication(void)
{
	if (m_is_running || (m_address == m_master_ip)){
		NS_ASSERT(m_discovery);
		//cout << m_address << " --->\t" << m_master_ip << endl;
		cout << m_role << " " << m_address << ": ";
		list<tuple<Ipv4Address, uint16_t>>::iterator iterator;
		for(iterator = begin(connections); iterator != end(connections); ++iterator){
			cout << get<0>(*iterator) << " ";
		}
		if (Jend != 0){
	  	cout << "-> " << (Jend-Jinit).GetMilliSeconds() << "ms";
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

void WellFormed::Setup(VirtualDiscovery *discovery, bool messages, bool churn, bool random_sleep, uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, uint32_t list_update_time, uint32_t duration, Ptr<UniformRandomVariable> rand, bool to_trace, fstream *trace, string churn_int)
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

void WellFormed::readPacket(Ptr<Socket> socket, Address from){
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
	//cout << m_address << " -- " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl << request.str();
	while (keep){
		getline(request, param);
		NS_LOG_DEBUG(param);
		if (param == "END"){
			keep = false;
		}
		else if (param == "JOIN")
		{
			//cout << "WJ " << m_address << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds() << endl;
			answer.str(string());
			if (m_role == "master")
			{
				if (distance(begin(connections), end(connections)) < connections_limit)
				{
					answer << "ACCEPT" << endl << "END" << endl;
					connections.emplace(end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
					if (debugMin)
						cout << m_address << " Accepting " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " as slave at " << Simulator::Now().GetSeconds() << endl;
					if (not connect_groups && only_avail_master && distance(begin(connections), end(connections)) + 1 == connections_limit){
						m_discovery->remove(m_address, m_port);
					}
				}
				else
				{
					if (connect_groups){
						map<Ipv4Address, bool>::const_iterator it;
						bool flag = true;
						for (it = begin(group_neighours); it != end(group_neighours); it++){
							if (it->second && flag){
								answer << "GO_JOIN" << endl << it->first << endl;
								flag = false;
							}
							Send("IM_FULL\nEND\n", it->first, 20);
						}
						if (flag){
							uint16_t i = randomGen->GetInteger(1,connections_limit);
							uint16_t k;
							list<tuple<Ipv4Address, uint16_t>>::const_iterator it = begin(connections);
							for (k = 0; k < i-1; k++){
								it++;
							}
							answer << "MAKE_GROUP_WITH\n" << get<0>(*it) << endl;
							group_neighours[InetSocketAddress::ConvertFrom (from).GetIpv4 ()] = true;
						}
					}else{
						cout << "FF " << m_address << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds() << endl;
						answer << "FULL" << endl;
						list<tuple<Ipv4Address, uint16_t>>::const_iterator it;
						for (it = begin(connections); it != end(connections); it++){
							answer << get<0>(*it) << endl;
						}
					}
					answer << "END" << endl;
				}
			}
			else if(m_role == "slave")
			{
				//// cout << m_address << ": " << m_master_ip << endl;
				answer << "ASK" << endl << m_master_ip << endl << "END" << endl;
			}
			Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
		}
		else if(param == "FULL")
		{ // Receive a FULL request. Exclude current node, try another.
			//// cout << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " is full"<<endl;
			if (debugMin)
				cout << "F " << m_address << " " << Simulator::Now().GetSeconds() << endl;
			getline(request, param);
			while(param != "END"){
				rejected_connections.emplace(end(rejected_connections), make_tuple((Ipv4Address)(param.c_str()), 20));
				getline(request, param);

			}
			rejected_connections.emplace(end(rejected_connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			Simulator::Schedule(Seconds(0.0), &WellFormed::Join, this);
		}
		else if(param == "IM_FULL")
		{ // A coinnected group just became full
			Ipv4Address from_ip = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
			if (group_neighours.find(from_ip) != end(group_neighours)){
				group_neighours[from_ip] = false;
			}
		}
		else if(param == "MAKE_GROUP_WITH" && m_role == "nothing")
		{
			getline(request, param);
			Ipv4Address cip = (Ipv4Address)(param.c_str());	
			m_role = "master";
			if (debugMin)
				cout << m_address << " is now Master at " << Simulator::Now().GetSeconds() << endl;
			Send("MAKE_GROUP\nEND\n", cip, 20);
			m_discovery->add(m_address, m_port);
			connections.emplace(end(connections), make_tuple(cip, 20));
			Jend = Simulator::Now();
			group_neighours[InetSocketAddress::ConvertFrom (from).GetIpv4 ()] = true;
		}
		else if(param == "MAKE_GROUP")
		{
			if(m_role == "slave"){
				connections.emplace(end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			}
		}
		else if(param == "GO_JOIN")
		{
			getline(request, param);
			Ipv4Address cip = (Ipv4Address)(param.c_str());	
			Send("JOIN\nEND\n", cip, 20);
		}
		else if (param == "ASK")
		{
			getline(request, param);
			Ipv4Address cip = (Ipv4Address)(param.c_str());
			Send("JOIN\nEND\n", cip, 20);
		}
		else if(param == "ACCEPT")
		{ // The Node is willing to accept us. Lets define our role.
			//// cout << m_address << ": accepted by " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl;
			if (m_role == "nothing")
			{
				m_role = "slave";
				//m_discovery->add(m_address, m_port);
				m_master_ip = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
				if (debugMin)
					cout << m_address << " Accepted as slave of " << m_master_ip << " at " << Simulator::Now().GetSeconds() << endl;
				connections.emplace(end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
				Jend = Simulator::Now();
			}else{
				Send("DROP\nEND\n", InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
			}
			// if (use_table){
			// 	if (tabled_neighours.find(InetSocketAddress::ConvertFrom (from).GetIpv4 ()) == end(tabled_neighours)){
			// 		tabled_neighours[InetSocketAddress::ConvertFrom (from).GetIpv4 ()] = vector<Ipv4Address>();
			// 	}
			// }
		}else if(param == "DROP"){
			auto it = find(begin(connections), end(connections), make_tuple(InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20));
			if (m_role == "master" && it != end(connections))
			{
				connections.erase(it);
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
			//// cout << m_address << " BCAST " << msg_uid << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << Simulator::Now().GetSeconds() << endl;
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
		}else if(param == "WHO_HAS" && m_role == "master"){
			answer.str(string());
			string file;
			getline(request, file);
			getline(request, param);
			Ipv4Address ip = (Ipv4Address)(param.c_str());
			if (find(begin(m_file_list), end(m_file_list), file) != end(m_file_list)){
				answer << "SEND_FILE" << endl << file << endl << 1 << endl << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom(from).GetIpv4(), 20);
			}else if (m_master_file_list.find(file) != end(m_master_file_list)){
				uint32_t elem = randomGen->GetInteger(0, distance(begin(m_master_file_list[file]), end(m_master_file_list[file]))-1);
				answer << "ASK_FILE_TO" << endl << get<0>(m_master_file_list[file].at(elem)) << endl << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom(from).GetIpv4(), 20);
			}else{
				if (allow_hops && distance(begin(group_neighours), end(group_neighours)) > 0){
					AskNeighbours(file, ip);
				}else{
					answer << "GO_TO_SERVER" << endl << "END" << endl;
					Send(answer, InetSocketAddress::ConvertFrom(from).GetIpv4(), 20);
				}
			}

			// getline(request, param);
			// uint32_t port = atoi(param.c_str());
			// Here I should have some protocol. But for now, allways send
			// if (find(begin(m_file_list), end(m_file_list), file) != end(m_file_list)){
			// 	answer.str(string());
			// 	answer << "SEND_FILE" << endl << file << endl << hops << "END" << endl;
			// 	Send(answer, ip, 20);
			// }
		}else if(param == "ASK_FILE_TO" && m_role == "slave" && requested_file != "\"File 0\""){
			answer.str(string());
			answer << "GET_FILE" << endl << requested_file << endl << m_address << 1 << endl << "END" << endl;
			getline(request, param);
			Send(answer, (Ipv4Address)(param.c_str()), 20);
		}else if(param == "HOP_ASK_FILE_TO"  && m_role == "slave" && requested_file != "\"File 0\""){
			answer.str(string());
			answer << "HOP_GET_FILE" << endl << requested_file << endl << m_address << 1 << endl << "END" << endl;
			getline(request, param);
			Send(answer, (Ipv4Address)(param.c_str()), 20);
		}else if(param == "GO_TO_SERVER" && m_role == "slave" && requested_file != "\"File 0\""){
			answer.str(string());
			answer << "GET_FILE" << endl << requested_file << endl << m_address << 1 << endl << "END" << endl;
			Send(answer, m_server, m_server_port);
		}else if(param == "GET_FILE"){
			getline(request, param);
			if(find(begin(m_file_list), end(m_file_list), param) != end(m_file_list)){
        answer.str(string());
        answer << "SEND_FILE" << endl << param << endl << 2 << "END" << endl;
        Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
      }
		}else if(param == "HOP_GET_FILE"){
			getline(request, param);
			if(find(begin(m_file_list), end(m_file_list), param) != end(m_file_list)){
        answer.str(string());
        answer << "SEND_FILE" << endl << param << endl << 4 << "END" << endl;
        Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
      }
		}else if(param == "I_HAVE" && m_role == "master"){
			getline(request, param);
			SetItem(param, InetSocketAddress::ConvertFrom(from).GetIpv4());
		}else if(param == "DO_YOU_HAVE" && group_neighours.find(InetSocketAddress::ConvertFrom(from).GetIpv4()) != end(group_neighours) && m_role == "master"){
			string file;
			getline(request, file);
			getline(request, param);
			Ipv4Address requester = (Ipv4Address)(param.c_str());
			if (find(begin(m_file_list), end(m_file_list), file) != end(m_file_list)){
				answer << "SEND_FILE" << endl << file << endl << 3 << endl << "END" << endl;
				Send(answer, requester, 20);
				answer.str(string());
				answer << "I_HAVE_SENT" << endl << file << endl << requester << endl << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom(from).GetIpv4(), 20);
			}else if (m_master_file_list.find(file) != end(m_master_file_list)){
				uint32_t elem = randomGen->GetInteger(0, distance(begin(m_master_file_list[file]), end(m_master_file_list[file]))-1);
				answer << "HOP_ASK_FILE_TO" << endl << get<0>(m_master_file_list[file].at(elem)) << endl << "END" << endl;
				Send(answer, requester, 20);
				answer.str(string());
				answer << "I_HAVE_SENT" << endl << file << endl << requester << endl << "END" << endl;
				Send(answer, InetSocketAddress::ConvertFrom(from).GetIpv4(), 20);
			}
		}else if(param == "I_HAVE_SENT" && m_role == "master"){
			string file;
			getline(request, file);
			getline(request, param);
			if (AskNeighboursStatus.find(make_tuple<string, Ipv4Address>(static_cast<string>(file), static_cast<Ipv4Address>((Ipv4Address)(param.c_str())))) != end(AskNeighboursStatus)){
				AskNeighboursStatus[make_tuple<string, Ipv4Address>(static_cast<string>(file), static_cast<Ipv4Address>((Ipv4Address)(param.c_str())))] = true;
			}
		}else if(param == "SEND_FILE" && requested_file != "\"File 0\""){
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
				if(m_role == "slave"){
					answer.str(string());
					answer << "I_HAVE" << endl << file << endl << "END" << endl;
					Send(answer, m_master_ip, m_master_port);
				}else if(m_role == "master"){
					SetItem(file, m_address);
				}
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

void WellFormed::AskNeighbours(string file, Ipv4Address ip){
	map<Ipv4Address, bool>::const_iterator it;
	stringstream answer;
	for(it = group_neighours.begin(); it != group_neighours.end(); it++){
		answer << "DO_YOU_HAVE" << endl << file << endl << ip << endl << "END" << endl;
		Send(answer, it->first, 20);
	}
	uint32_t timeout = 2; // set timeout of 5secs
	AskNeighboursStatus[make_tuple<string, Ipv4Address>(static_cast<string>(file), static_cast<Ipv4Address>(ip))] = false;
	Simulator::Schedule(Seconds(timeout), &WellFormed::AskNeighboursTimeout, this, file, ip);
}

void WellFormed::AskNeighboursTimeout(string file, Ipv4Address ip){
	if (!AskNeighboursStatus[make_tuple<string, Ipv4Address>(static_cast<string>(file), static_cast<Ipv4Address>(ip))]){
		Send("GO_TO_SERVER\nEND\n", ip, 20);
	}
	AskNeighboursStatus.erase(AskNeighboursStatus.find(make_tuple<string, Ipv4Address>(static_cast<string>(file), static_cast<Ipv4Address>(ip))));
}

void WellFormed::SetItem(string file, Ipv4Address ip){
	if (m_role != "master") return;
	if (m_master_file_list.find(file) == end(m_master_file_list)){
		m_master_file_list[file] = vector<tuple<Ipv4Address,uint16_t>> ();
	}
	m_master_file_list[file].emplace(end(m_master_file_list[file]), make_tuple(ip, 20));
}

void WellFormed::ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> p; /*!< Received packet */
	Address from; /*!< Origin address of the packet*/
	void *local;

	while(socket->GetRxAvailable () > 0){
		p = socket->RecvFrom (from);//socket->Recv (socket->GetRxAvailable (), 0);
		if (debugAll)
			cout << m_address << " Recv Something from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl;
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

void WellFormed::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, data, size);
}

void WellFormed::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void WellFormed::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}	
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void WellFormed::inSendChk(){
	if (inSending){
		if (debugMin)
	 		cout << m_address << " Timed out at " << Simulator::Now().GetSeconds() << endl;
	 	Join();
	}
}

void WellFormed::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i){
	if (debugAll)
		cout << m_address << " SendPacket at " << Simulator::Now().GetSeconds()<< endl;
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
			if (debugAll)
				cout << m_address << " Send Success!"  << Simulator::Now().GetSeconds() << endl;
			NS_LOG_DEBUG("Send Success");
		}else{
			if (debugAll)
				cout << m_address << " Send Error! " << Simulator::Now().GetSeconds() << endl;
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
		Simulator::Schedule (Seconds(0.00001), &WellFormed::SendPacket, this, socket, data, size, i);
	}else{
		socket->Close();
	}
}

void WellFormed::Leave(void){
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
						if (debugMin)
							cout << "Leave " << Simulator::Now().GetSeconds()+(next_start/1000) << endl;
						Simulator::Schedule (MilliSeconds(next_start), &WellFormed::DoWork, this);
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

void WellFormed::Join(void){
	Jinit = Simulator::Now();
	if (m_role == "nothing"){
		vector<tuple<Ipv4Address,uint16_t>> all_ip = m_discovery->getAll();
		vector<tuple<Ipv4Address,uint16_t>> avail;
		uint32_t nelems = 0;
		vector<tuple<Ipv4Address,uint16_t>>::const_iterator it;
		for (it = begin(all_ip); it != end(all_ip); it++){
			if (
				find(begin(rejected_connections), end(rejected_connections), (*it)) == end(rejected_connections) 
				&& get<0>(*it) != m_address 
				&& get<0>(*it) != tryOther
				){
				avail.emplace(end(avail), (*it));
				nelems++;
			}
		}
		if (nelems > 0){
			tuple<Ipv4Address,uint16_t> ip = avail.at(randomGen->GetInteger(0, nelems-1));
			NS_LOG_DEBUG(m_address << " is JOIN " << get<0>(ip) << ":" << get<1>(ip) << endl);
			//cout << m_address << ": request join to " << get<0>(ip) << endl;
			//cout << "J " << m_address << " " << get<0>(ip) << " " << Simulator::Now().GetSeconds() << endl;
			if (debugMin)
				cout << m_address << " Wants to Join " << get<0>(ip) << " at " << Simulator::Now().GetSeconds() << endl;
			inSending = true;
			Simulator::Schedule(Seconds(5), &WellFormed::inSendChk, this);
			tryOther = get<0>(ip);
			Send("JOIN\nEND\n", get<0>(ip), get<1>(ip));
		}else{
			//cout << m_address << ": im master" << endl;
			if (debugMin)
				cout << m_address << " is now Master at " << Simulator::Now().GetSeconds() << endl;
			m_role = "master";
			m_discovery->add(m_address, m_port);
		}
		if (Simulator::Now().GetSeconds()+5 < simulation_duration && !connect_groups){
			Simulator::Schedule(Seconds(5), &WellFormed::isJoin, this);
		}
	}
}

void WellFormed::isJoin(void){
	if (m_role == "nothing"){
		Join();
	}
}

void WellFormed::updateFileList(void){
	stringstream request;
	request << "GET_FILE_LIST" << endl << "END" << endl;
	Send(request, m_master_ip, m_master_port);
}

void WellFormed::updateNodesFileList(void){
	stringstream updateMsg;
	updateMsg << "FILE_LIST_UPDATE\n" << n_files_available << "\nEND\n";
	for(auto it = begin(m_master_node_list); it != end(m_master_node_list); it++){
		Send(updateMsg, get<0>(*it), get<1>(*it));
	}
}

void WellFormed::BCast(string message){
	uint32_t msg_uid = randomGen->GetInteger(0, UINT32_MAX);
	MySentMessages.emplace(end(MySentMessages), msg_uid);
	stringstream bcast;
	bcast << "BCAST" << endl << msg_uid << endl << message;
	BCast(reinterpret_cast<const uint8_t *>(bcast.str().c_str()), (uint32_t)(bcast.str().size()+1));
}

// In this demo, a broadcast can only reach one level of connected peers
void WellFormed::BCast(const uint8_t * data, uint32_t size){
	list<tuple<Ipv4Address, uint16_t>>::const_iterator iterator;
  for(iterator = begin(connections); iterator != end(connections); ++iterator){
    Send(data, size, get<0>(*iterator), get<1>(*iterator));
  }
}

void WellFormed::CheckFileRequests(){
	//cout << m_address << " is checking file requests for " << requested_file << " " << Simulator::Now().GetSeconds() << endl;
	if (requested_file != "\"File 0\"" || find(begin(m_file_list), end(m_file_list), requested_file) == end(m_file_list)){
		//cout << m_address << " will go to server for " << requested_file << " " << Simulator::Now().GetSeconds() << endl;
		stringstream request;
		request << "GET_FILE\n" << requested_file << "\n" << m_address << "\n" << to_string(m_port) << "\nEND\n";
		Send(request, m_server, m_server_port);
	}
}

void WellFormed::genFReqs(){
	// Next file request
	const uint32_t left = floor(simulation_duration - Simulator::Now().GetSeconds());
	if (left > 5){
		uint32_t next = randomGen->GetInteger(5, min(left, (const uint32_t)60));
		//cout << "FReqs " << Simulator::Now().GetSeconds()+next << endl;
		Simulator::Schedule(Seconds(next), &WellFormed::genFReqs, this);
	}
	if (n_files_available == 0 || requested_file != "\"File 0\"" || (Simulator::Now().GetSeconds()+5)>simulation_duration) return;
	requested_file = string("\"File ")+to_string(randomGen->GetInteger(1, n_files_available))+ string("\"");
	stringstream request;
	cout << m_address << " Want " << requested_file << " (" << Simulator::Now().GetSeconds() << ")" << endl;
	if (m_role == "master"){
		request << "GET_FILE\n" << requested_file << endl << m_address << endl << 0 << endl << "END" << endl;
		if (m_master_file_list.find(requested_file) != end(m_master_file_list)){
			uint32_t elem = randomGen->GetInteger(0, (int)distance(begin(m_master_file_list[requested_file]), end(m_master_file_list[requested_file]))-1);
			Send(request, get<0>(m_master_file_list[requested_file].at(elem)), get<1>(m_master_file_list[requested_file].at(elem)));
		}else{
			Send(request, m_server, m_server_port);
		}
	}else if(m_role == "slave"){
		request << "WHO_HAS" << endl << requested_file << endl << m_address << endl << "END" << endl;
		Send(request, m_master_ip, 20);
	}
	//Simulator::Schedule(MilliSeconds(static_cast<uint64_t>(delay*1000)), &WellFormed::CheckFileRequests, this);


	//cout << m_address << " Want " << requested_file << " (" << Simulator::Now().GetSeconds() << ")" << endl;
	// if (use_table){
	// 	map<Ipv4Address, vector<Ipv4Address>>::const_iterator it;
	// 	for (it = begin(tabled_neighours); it != end(tabled_neighours); it++){
	// 		request << "WHO_HAS" << endl << requested_file << endl << m_address << endl << m_port << endl << "END" << endl;
	// 		Send(request, (Ipv4Address)it->first, 20);
	// 		vector<Ipv4Address>::const_iterator sub_it;
	// 		for (sub_it = begin(it->second); sub_it != end(it->second); sub_it++){
	// 			Send(request, (Ipv4Address)(*sub_it), 20);
	// 		}
	// 	}
	// }else{
	// 	request << (uint32_t)(0) << endl << "WHO_HAS" << endl << requested_file << endl << m_address << endl << m_port << endl << "END" << endl;
	// 	BCast(request.str());
	// }
	//cout << "GenTimeout " << Simulator::Now().GetSeconds() + static_cast<uint64_t>(delay) << endl;
	//Simulator::Schedule(MilliSeconds(static_cast<uint64_t>(delay*1000)), &WellFormed::CheckFileRequests, this);
}

void WellFormed::TabledSendListTo(Ipv4Address to){
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
