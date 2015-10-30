#include "ExperimentAppWifiDirect.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("HyraxExperimentAppWifiDirect");

HyraxExperimentAppWifiDirect::HyraxExperimentAppWifiDirect()
{
	NS_LOG_DEBUG("Starting HyraxExperimentAppWifiDirect");
}

HyraxExperimentAppWifiDirect::~HyraxExperimentAppWifiDirect()
{
	NS_LOG_DEBUG("Destroy HyraxExperimentAppWifiDirect");
}

void HyraxExperimentAppWifiDirect::Setup(std::string type, uint32_t NNodes, uint32_t NServers, uint32_t Scenario, uint32_t FileSize, bool Debug, bool ShowPackages, bool ShowData, bool exclusive){
	m_type = type;
	n_nodes = NNodes;
	n_servers = NServers;
	debug = Debug;
	show_packages = ShowPackages;
	show_data = ShowData;
	scenario_id = Scenario;
	packet_size = FileSize;
	exclusive_servers = exclusive;
}

void HyraxExperimentAppWifiDirect::StartApplication(void)
{
	m_address = GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	// std::cout << GetNode()->GetObject<ns3::MobilityModel>()->GetPosition () << std::endl;

	if (scenario_id == 42 || scenario_id == 43 || scenario_id == 6) n_servers = n_servers+1;
	genServerList();

	// O owner têm dois ips. 10.1.4.1 e 10.1.4.2
	SocketListener = CreateSocket();

	if(m_type == "WD_Slave") {
		if (exclusive_servers && std::distance(std::begin(ServerList), std::end(ServerList)) < ((scenario_id == 42 || scenario_id == 43 || scenario_id == 6) ? (n_servers-1) : n_servers))
			return;
		if ((scenario_id == 42 || scenario_id == 43 || scenario_id == 52 || scenario_id == 6)	&& (std::distance(std::begin(ServerList), std::end(ServerList)) == 0))
			return;
		RunSimulation();
	}
}

void HyraxExperimentAppWifiDirect::RunSimulation(){
	if (files_fetched >= files_to_fetch) return;
	if (scenario_id == 41){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_4_a, this);
	}else if (scenario_id == 42){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_4_b, this);
	}else if (scenario_id == 43){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_4_c, this);
	}else if (scenario_id == 51){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_5_a, this);
	}else if (scenario_id == 52){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_5_b, this);
	}else if (scenario_id == 6){
		ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentAppWifiDirect::Scenario_6, this);
	}
}

void HyraxExperimentAppWifiDirect::genServerList(){
	std::stringstream ip;
	uint32_t x = ((scenario_id == 43 || scenario_id == 42 || scenario_id == 6) ? 2 : 1); // Avoid using WD GO as Server.
	uint32_t limit = (exclusive_servers ? ((scenario_id == 43 || scenario_id == 42 || scenario_id == 6) ? n_servers : n_servers+1) : n_servers);
	for (; x <= limit; x++){
		ip << "10.1.4." << x;
		if (ns3::Ipv4Address(ip.str().c_str()) != m_address || !exclusive_servers){
			ServerList.emplace(std::end(ServerList), ns3::Ipv4Address(ip.str().c_str()));
			ip.str(std::string());
			ip << "10.1.5." << x;
			AdHocServerList.emplace(std::end(AdHocServerList), ns3::Ipv4Address(ip.str().c_str()));
		}
		//if (x == 1) x++;  // The first Node has two interfaces. (.1 and .2).
		ip.str(std::string());
	}
}


ns3::Ptr<ns3::Socket> HyraxExperimentAppWifiDirect::CreateSocket(void){
	ns3::Ptr<ns3::Socket> LocalSocket = ns3::Socket::CreateSocket (GetNode(), 
																							ns3::TcpSocketFactory::GetTypeId ());

	LocalSocket->Bind(ns3::InetSocketAddress (ns3::Ipv4Address::GetAny (), 20));
	LocalSocket->Listen();
	LocalSocket->SetAllowBroadcast (true);

	LocalSocket->SetAcceptCallback (ns3::MakeCallback (&HyraxExperimentAppWifiDirect::ConnectionRequest, this),
														 ns3::MakeCallback(&HyraxExperimentAppWifiDirect::AcceptConnection, this));
	LocalSocket->SetRecvCallback (ns3::MakeCallback (&HyraxExperimentAppWifiDirect::ReceivePacket, this));
	LocalSocket->SetCloseCallbacks (ns3::MakeCallback (&HyraxExperimentAppWifiDirect::NormalClose, this),
														 ns3::MakeCallback (&HyraxExperimentAppWifiDirect::ErrorClose, this));
	LocalSocket->SetConnectCallback (ns3::MakeCallback (&HyraxExperimentAppWifiDirect::ConnectSuccess, this),
														 ns3::MakeCallback (&HyraxExperimentAppWifiDirect::ConnectFail, this));
	return LocalSocket;	
}

void HyraxExperimentAppWifiDirect::ConnectSocket(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data){
	ns3::Simulator::Schedule(ns3::MilliSeconds(1000*randomGen->GetValue(1.5, 3.0)), &HyraxExperimentAppWifiDirect::CompleteConnectSocket, this, socket, ip, port, data);
}

void HyraxExperimentAppWifiDirect::CompleteConnectSocket(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data){
	socket->Connect (ns3::InetSocketAddress (ip, port));
	if (!ns3::Simulator::IsFinished()){
		Send(socket, data);
	}
}

void HyraxExperimentAppWifiDirect::ConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data, ns3::Ipv4Address Adhoc_ip, uint32_t Adhoc_port){
	//double delay = 1000*randomGen->GetValue(1.5, 3.0);
	uint32_t delay = 1;
	uint32_t ad_hoc_delay = 100;
	ns3::Simulator::Schedule(ns3::MilliSeconds(delay), &HyraxExperimentAppWifiDirect::CompleteConnectSocketTDLS, this, socket, ip, port);
	ns3::Simulator::Schedule(ns3::MilliSeconds(delay+ad_hoc_delay), &HyraxExperimentAppWifiDirect::CompleteAdHocConnectSocketTDLS, this, socket, Adhoc_ip, Adhoc_port, data);
}

void HyraxExperimentAppWifiDirect::CompleteConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port){
	socket->Connect(ns3::InetSocketAddress(ip, port));
}

void HyraxExperimentAppWifiDirect::CompleteAdHocConnectSocketTDLS(ns3::Ptr<ns3::Socket> socket, ns3::Ipv4Address ip, uint32_t port, std::string data){
	ns3::Ptr<ns3::Socket> TDLSsocket = CreateSocket();
	TDLSsocket->Connect (ns3::InetSocketAddress (ip, port));
	if (!ns3::Simulator::IsFinished()){
		Send(TDLSsocket, data);
	}
}

// WD + GO is Server + n Clients.
void HyraxExperimentAppWifiDirect::Scenario_4_a(void){
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	//LocalSocket->Connect (ns3::InetSocketAddress (ns3::Ipv4Address("10.1.4.1"), 20));
	
	//uint32_t next = randomGen->GetInteger(0, std::distance(ServerList.begin(), ServerList.end())-1);
	//LocalSocket->Connect (ns3::InetSocketAddress (ServerList.at(next), 20));
	LocalSocket->Connect (ns3::InetSocketAddress ("10.1.4.1", 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(LocalSocket, "SEND\nEND\n");
}

// WD + GO + m MobileServers + n Clients.
void HyraxExperimentAppWifiDirect::Scenario_4_b(void){
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(std::begin(ServerList), std::end(ServerList))-1);
	LocalSocket->Connect(ns3::InetSocketAddress (ServerList.at(next), 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(LocalSocket, "SEND\nEND\n");
}

// WD + no GO + m MobileServers + m Clients.
void HyraxExperimentAppWifiDirect::Scenario_4_c(void){
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(std::begin(ServerList), std::end(ServerList))-1);
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	ConnectSocket(LocalSocket, AdHocServerList.at(next), 20, "SEND\nEND\n");
}

// WD + Legacy AP is Server + n Clients
void HyraxExperimentAppWifiDirect::Scenario_5_a(void){	
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	LocalSocket->Connect (ns3::InetSocketAddress (ns3::Ipv4Address("10.1.4.1"), 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(LocalSocket, "SEND\nEND\n");
}

// WD + Legacy AP + m Servers + n Clients
void HyraxExperimentAppWifiDirect::Scenario_5_b(void){	
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(ServerList.begin(), ServerList.end())-1);
	LocalSocket->Connect (ns3::InetSocketAddress (ServerList.at(next), 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(LocalSocket, "SEND\nEND\n");
}

// WD + GO + TDLS + m MobileServers + n Clients
void HyraxExperimentAppWifiDirect::Scenario_6(void){
	ns3::Ptr<ns3::Socket> LocalSocket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(std::begin(ServerList), std::end(ServerList))-1);
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	ConnectSocketTDLS (LocalSocket, 
		ServerList.at(next), 20, "SEND\nEND\n", 
		AdHocServerList.at(next), 20);
}

void HyraxExperimentAppWifiDirect::Send(ns3::Ptr<ns3::Socket> socket, std::string data){
	static uint32_t tx_size = static_cast<uint32_t>(socket->GetTxAvailable())/2;
	if (data.size() < tx_size){
		total_sent_data += data.size();
		socket->Send(reinterpret_cast<const uint8_t *>(data.c_str()), data.size(), 0);
	}else{
		std::string local = data.substr(0, tx_size);
		socket->Send(reinterpret_cast<const uint8_t *>(local.c_str()), tx_size, 0);
		if (socket->GetErrno () == 0){
			data.erase(0, tx_size);
			total_sent_data += tx_size;
		}else{
			std::cout << "Socket Error - " << socket->GetErrno () << "\n";
		}
		ns3::Simulator::Schedule(ns3::MilliSeconds(10), &HyraxExperimentAppWifiDirect::Send, this, socket, data);
	}
}

void HyraxExperimentAppWifiDirect::NormalClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " NormalClose (" << socket << ")" << std::endl;
}

void HyraxExperimentAppWifiDirect::ErrorClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ErrorClose (" << socket << ")" << std::endl;
}

bool HyraxExperimentAppWifiDirect::ConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4());
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectionRequest (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
	return true;
}

void HyraxExperimentAppWifiDirect::AcceptConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&HyraxExperimentAppWifiDirect::ReceivePacket, this));
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " AcceptConnection (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
}

void HyraxExperimentAppWifiDirect::ConnectSuccess(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectSuccess (" << socket << ")" << std::endl;
}

void HyraxExperimentAppWifiDirect::ConnectFail(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectFail (" << socket << ")" << std::endl;
}

void HyraxExperimentAppWifiDirect::ReceivePacket(ns3::Ptr<ns3::Socket> socket){
	ns3::Ptr<ns3::Packet> packet;
	ns3::Address DataFrom;
	while(socket->GetRxAvailable () > 0){
		packet = socket->RecvFrom(DataFrom);
		packet->CopyData(&SocketData[socket], packet->GetSize());
		total_data += packet->GetSize();
		while (SocketData[socket].str().size() >= 5 && (SocketData[socket].str().find("\nEND\n") < SocketData[socket].str().size())){
			ReadData(socket, DataFrom, SocketData[socket].str());
			SocketData[socket].str(SocketData[socket].str().erase(0, SocketData[socket].str().find ("\nEND\n")+5));
		}
		if (show_packages)
			std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ReceivePacket (" << socket << ")" << std::endl;
	}
}

void HyraxExperimentAppWifiDirect::ReadData(ns3::Ptr<ns3::Socket> socket, ns3::Address from, std::string data){
	std::string line;
	std::stringstream request(data);
	std::getline(request, line);
	while(line != "END"){
		if (line == "SEND"){
			std::stringstream answer;
			answer << "DATA\n";
			for(uint32_t x = 0; x < packet_size; x++) answer << std::rand() % 9;
			answer << "\nEND\n";
			Send(socket, answer.str());
			if (show_data){
				std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " Sent DATA (" << answer.str().size() << " bytes) to " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
			}
		}
		else if(line == "DATA"){
			getline(request, line);
			if (show_data){
				std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " Got DATA (" << line.size()+10 << " bytes) from " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
			}else{
				std::cout << m_address << "\t" << ns3::Simulator::Now().GetSeconds() - fetch_init_time << std::endl;
			}
			files_fetched++;
			RunSimulation();
		}
		getline(request, line);
	}
	SocketData.erase(socket);
}

void HyraxExperimentAppWifiDirect::StopApplication(void)
{
	std::cout << m_address << " (" << m_type << ") Total bytes Sent: " << total_sent_data << std::endl;
}