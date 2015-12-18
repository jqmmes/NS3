#include "ExperimentApp.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("HyraxExperimentApp");

HyraxExperimentApp::HyraxExperimentApp()
{
	NS_LOG_DEBUG("Starting HyraxExperimentApp");
}

HyraxExperimentApp::~HyraxExperimentApp()
{
	NS_LOG_DEBUG("Destroy HyraxExperimentApp");
}

void HyraxExperimentApp::Setup(std::string type, uint32_t NNodes, uint32_t NServers, uint32_t Scenario, uint32_t FileSize, bool Debug, bool ShowPackages, bool ShowData, bool exclusive){
	n_nodes = NNodes;
	n_servers = NServers;
	m_type = type;
	debug = Debug;
	show_packages = ShowPackages;
	show_data = ShowData;
	scenario_id = Scenario;
	packet_size = FileSize;
	exclusive_servers = exclusive;
}

void HyraxExperimentApp::StartApplication(void)
{
	m_address = GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	//if (m_address == ns3::Ipv4Address("10.1.10.1"))
		//std::cout << GetNode()->GetObject<ns3::Ipv4>()->GetNInterfaces() << std::endl;
	//std::cout << m_address << std::endl;
	// std::cout << GetNode()->GetObject<ns3::MobilityModel>()->GetPosition () << std::endl;

	std::cout << "init " << m_address << std::endl;
	
		Listen = CreateSocket();
		//Listen->ShutdownSend();
		TDLSListen = CreateTDLSSocket();
		//TDLSListen->ShutdownSend();


	if(m_type == "Server" && measure_only_server)	{
		RunSimulation();
	}


	if(m_type == "RemoteStation" && !measure_only_server)	{
		genServerList();
		if ((scenario_id == 2 || scenario_id == 3) && ((std::distance(std::begin(ServerList), std::end(ServerList)) == 0)
			|| (exclusive_servers && std::distance(std::begin(ServerList), std::end(ServerList)) < n_servers)))
		{
			return;
		}
		RunSimulation();
	}
}

void HyraxExperimentApp::RunSimulation(){
	if (measure_only_server) {
		if (server_files_fetched >= files_to_fetch) return;
			ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
			&HyraxExperimentApp::Stress_Server, this);
	} else{
		if (files_fetched >= files_to_fetch) return;
		if (scenario_id == 1){
			if (new_sim){
				ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(4,6)), 
				&HyraxExperimentApp::Scenario_1, this);
			}else{
				ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
				&HyraxExperimentApp::Scenario_1, this);
			}
		}else if (scenario_id == 2){
			ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
				&HyraxExperimentApp::Scenario_2, this);
		}else if (scenario_id == 3){
			ns3::Simulator::Schedule(ns3::Seconds(randomGen->GetValue(0,0.1)), 
				&HyraxExperimentApp::Scenario_3, this);
		}
	}
}

void HyraxExperimentApp::genServerList(){
	std::stringstream ip;
	for (uint32_t x = 1; x <= n_servers; x++){
		ip << "10.1.2." << x;
		if (ns3::Ipv4Address(ip.str().c_str()) != m_address){
			ServerList.emplace(std::end(ServerList), ns3::Ipv4Address(ip.str().c_str()));
			ip.str(std::string());
			ip << "10.1.3." << x;
			AdHocServerList.emplace(std::end(AdHocServerList), ns3::Ipv4Address(ip.str().c_str()));
		}
		ip.str(std::string());
	}
}

ns3::Ptr<ns3::Socket> HyraxExperimentApp::CreateSocket(void){
	ns3::Ptr<ns3::Socket> socket = ns3::Socket::CreateSocket (GetNode(), ns3::TcpSocketFactory::GetTypeId ());
	//std::cout << m_address << "\tCreateSocket" << std::endl;
	//socket->Bind(ns3::InetSocketAddress (ns3::Ipv4Address::GetAny (), 20));
	socket->Bind(ns3::InetSocketAddress (m_address, 20));
	socket->Listen();
	socket->SetAllowBroadcast (true);

	socket->SetAcceptCallback (ns3::MakeCallback (&HyraxExperimentApp::ConnectionRequest, this),
														 ns3::MakeCallback(&HyraxExperimentApp::AcceptConnection, this));
	socket->SetRecvCallback (ns3::MakeCallback (&HyraxExperimentApp::ReceivePacket, this));
	//std::cout << m_address << "\tCrt\t" << socket << std::endl;
	socket->SetCloseCallbacks (ns3::MakeCallback (&HyraxExperimentApp::NormalClose, this),
														 ns3::MakeCallback (&HyraxExperimentApp::TDLSErrorClose, this));
	socket->SetConnectCallback (ns3::MakeCallback (&HyraxExperimentApp::ConnectSuccess, this),
														 ns3::MakeCallback (&HyraxExperimentApp::ConnectFail, this));

	socket_data[socket] = malloc(0);
	current_socket_data[socket] = 0;

	return socket;
}

ns3::Ptr<ns3::Socket> HyraxExperimentApp::CreateTDLSSocket(void){
	ns3::Ptr<ns3::Socket> socket = ns3::Socket::CreateSocket (GetNode(), ns3::TcpSocketFactory::GetTypeId ());
	std::cout << m_address << "\tCreateTDLSSocket" << std::endl;
	socket->Bind(ns3::InetSocketAddress (ns3::Ipv4Address::GetAny (), 20)); // Binds to 0.0.0.0:20
	socket->Listen();
	socket->SetAllowBroadcast (true);

	socket->SetAcceptCallback (ns3::MakeCallback (&HyraxExperimentApp::TDLSConnectionRequest, this),
														 ns3::MakeCallback(&HyraxExperimentApp::AcceptTDLSConnection, this)); // Accepting connection. If accepted, next callback returns socket.
	socket->SetRecvCallback (ns3::MakeCallback (&HyraxExperimentApp::ReceivePacket, this));
	std::cout << m_address << "\tCrt\t" << socket << std::endl;
	socket->SetCloseCallbacks (ns3::MakeCallback (&HyraxExperimentApp::TDLSNormalClose, this),
														 ns3::MakeCallback (&HyraxExperimentApp::TDLSErrorClose, this));
	socket->SetConnectCallback (ns3::MakeCallback (&HyraxExperimentApp::ConnectSuccess, this),
														 ns3::MakeCallback (&HyraxExperimentApp::ConnectFail, this));

	socket_data[socket] = malloc(0);
	current_socket_data[socket] = 0;

	return socket;
}

void HyraxExperimentApp::Stress_Server(void){
	std::stringstream ip;
	for (uint32_t x = 1; x <= n_nodes; x++){
		ip << "10.1.2." << x;
		socket = CreateSocket();
		socket->Connect (ns3::InetSocketAddress (ns3::Ipv4Address(ip.str().c_str()), 20));
		Send(socket, "SEND\nEND\n");
		ip.str(std::string());
	}
}

// Data is only get from Server.
void HyraxExperimentApp::Scenario_1(void){
	socket = CreateSocket();
	socket->Connect (ns3::InetSocketAddress (ns3::Ipv4Address("10.1.1.2"), 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(socket, "SEND\nEND\n");
}

// Data is only get from other Nodes with help from AP.
void HyraxExperimentApp::Scenario_2(void){
	socket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(ServerList.begin(), ServerList.end())-1);
	socket->Connect (ns3::InetSocketAddress (ServerList.at(next), 20));
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	Send(socket, "SEND\nEND\n");
}

// Data is only get directly from other Nodes with AP establishing connection. TDLS
void HyraxExperimentApp::Scenario_3(void){
	std::cout << "Scenario3" << std::endl;
	ns3::Ptr<ns3::Socket> ApSocket = CreateSocket();
	uint32_t next = randomGen->GetInteger(0, std::distance(std::begin(ServerList), std::end(ServerList))-1);
	ApSocket->Connect (ns3::InetSocketAddress (ServerList.at(next), 20));
	socket = CreateTDLSSocket();
	uint32_t status = socket->Connect (ns3::InetSocketAddress (AdHocServerList.at(next), 20));
	//socket->Close();
	fetch_init_time = ns3::Simulator::Now().GetSeconds();
	std::cout << status << std::endl;
	if (status == 0){
		std::cout << ns3::Simulator::Now().GetSeconds() << "\t" << "OkTDLSConnect to " << AdHocServerList.at(next) << std::endl;
		Send(socket, "SEND\nEND\n");
	}else{ // Fallback
		std::cout << "Fallback to " << ServerList.at(next) << std::endl;
		Send(ApSocket, "SEND\nEND\n");
	}
}

void HyraxExperimentApp::Send(ns3::Ptr<ns3::Socket> socket, std::string data){
	if (ns3::Simulator::IsFinished()) return;
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
		ns3::Simulator::Schedule(ns3::MilliSeconds(10), &HyraxExperimentApp::Send, this, socket, data);
	}
}

void HyraxExperimentApp::NormalClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " NormalClose (" << socket << ")" << std::endl;
	std::cout << "Normal Closing Normal Bro...\t" << socket << std::endl;
}

void HyraxExperimentApp::TDLSNormalClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " NormalClose (" << socket << ")" << std::endl;
	NS_ASSERT(TDLS_cons > 0);
	--TDLS_cons;
	std::cout << "Closing Normal Bro...\t" << socket << std::endl;
}

void HyraxExperimentApp::ErrorClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ErrorClose (" << socket << ")" << std::endl;
	std::cout << "Normal Closing Error Bro...\t" << socket << std::endl;
}

void HyraxExperimentApp::TDLSErrorClose(ns3::Ptr<ns3::Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ErrorClose (" << socket << ")" << std::endl;
	NS_ASSERT(TDLS_cons > 0);
	--TDLS_cons;
	std::cout << "Closing Error Bro...\t" << socket << std::endl;
}

bool HyraxExperimentApp::ConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	std::cout << "ConnectionRequest\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4());
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectionRequest (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
	return true;
}

bool HyraxExperimentApp::TDLSConnectionRequest(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	std::cout << "TDLSConnectionRequest\t" << TDLS_cons << "\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " TDLSConnectionRequest (" << socket << ") from " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4());
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " TDLSConnectionRequest (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
	if (TDLS_cons < max_TDLS_cons && TDLS_enabled){
		std::cout << m_address << "\t" << "ahoy brother" << std::endl;
		++TDLS_cons;
		return true;
	}
	std::cout << m_address << "\t" << "Naha brother" << std::endl;
	return false;
}

void HyraxExperimentApp::AcceptConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	std::cout << "AcceptConnection\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&HyraxExperimentApp::ReceivePacket, this));
	// socket->SetCloseCallbacks (ns3::MakeCallback (&HyraxExperimentApp::TDLSNormalClose, this),
	// 													 ns3::MakeCallback (&HyraxExperimentApp::TDLSErrorClose, this));
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " AcceptConnection (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
}

void HyraxExperimentApp::AcceptTDLSConnection(ns3::Ptr<ns3::Socket> socket, const ns3::Address& from){
	std::cout << "TDLSAcceptConnection\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&HyraxExperimentApp::ReceivePacket, this));
	// socket->SetCloseCallbacks (ns3::MakeCallback (&HyraxExperimentApp::TDLSNormalClose, this),
	// 													 ns3::MakeCallback (&HyraxExperimentApp::TDLSErrorClose, this));
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " AcceptConnection (" << socket << ") from " 
						<< ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
	tdlsSock = socket;
	//std::cout << m_address << "\t" << socket << std::endl;
	//std::cout << m_address << "\twanna close \t" << socket->Close() << "\t" << socket << std::endl;
	//std::cout << m_address << "\t" << socket << "\t" << socket->GetErrno() << std::endl;
}

void HyraxExperimentApp::ConnectSuccess(ns3::Ptr<ns3::Socket> socket){
	std::cout << ns3::Simulator::Now().GetSeconds() << "\t" << m_address << "\tConnectSuccess\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectSuccess (" << socket << ")" << std::endl;
}

void HyraxExperimentApp::ConnectFail(ns3::Ptr<ns3::Socket> socket){
	std::cout << m_address << "\tConnectFail\t" << socket << std::endl;
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
	if (debug)
		std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " ConnectFail (" << socket << ")" << std::endl;
}

void HyraxExperimentApp::ReceivePacket(ns3::Ptr<ns3::Socket> socket){
	ns3::Ptr<ns3::Packet> packet;
	ns3::Address DataFrom;
	while(socket->GetRxAvailable () > 0){
		packet = socket->RecvFrom(DataFrom);

		if ((m_type == "Server" && measure_only_server) || measure_data) 
			std::cout << ns3::Simulator::Now().GetSeconds() << " " << packet->GetSize() << std::endl;

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

void HyraxExperimentApp::ReadData(ns3::Ptr<ns3::Socket> socket, ns3::Address from, std::string data){
	std::cout << m_address << "\tRcv\t" << socket << std::endl;
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
			if (show_data)
				std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " Sent DATA (" << answer.str().size() << " bytes) to " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
		}
		else if(line == "DATA"){
			getline(request, line);
			if (show_data){
				std::cout << ns3::Simulator::Now().GetSeconds() << ": " << m_address << " Got DATA (" << line.size()+10 << " bytes) from " << ns3::InetSocketAddress::ConvertFrom (from).GetIpv4() << std::endl;
			}else{ 
				if (!measure_only_server && !measure_data)
					std::cout << m_address << "\t" << ns3::Simulator::Now().GetSeconds() - fetch_init_time << std::endl;
			}
			files_fetched++;
			if (!measure_only_server){
				RunSimulation();
			}else{
				if (files_fetched == n_nodes){
					files_fetched = 0;
					server_files_fetched++;
					RunSimulation();
				}
			}
		}
		getline(request, line);
	}
	//std::cout << socket << "\t" << socket->Close() << "\tclose" << std::endl;
	if (socket == tdlsSock){
		--TDLS_cons;
	}
	std::cout << m_address << "\t" << TDLS_cons << std::endl;
}

void HyraxExperimentApp::StopApplication(void)
{

	std::cout << m_address << " (" << m_type << ") Total bytes Sent: " << total_sent_data << std::endl;
}