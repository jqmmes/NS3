#include "SimpleRing.h"
#include <inttypes.h>
#include "BTInterface.h"

#include <ns3/node-list.h>

NS_LOG_COMPONENT_DEFINE ("SimpleRing");

using namespace ns3;
using namespace std;

SimpleRing::SimpleRing()
{
	NS_LOG_DEBUG("Starting SimpleRing");
}

SimpleRing::~SimpleRing()
{
	NS_LOG_DEBUG("Destroy SimpleRing");
}

void SimpleRing::StartApplication(void)
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

void SimpleRing::DoWork(void){
	m_is_running = true;
	Join();
	if(use_churn) {
		if(!Simulator::IsFinished()){
			uint32_t left = simulation_duration - Simulator::Now().GetSeconds() - 5;
			if (left > 0){
				uint32_t next_stop;
				if (churn_intensity == "low")	next_stop = randomGen->GetInteger(1,2*left);
				else if (churn_intensity == "medium") next_stop = randomGen->GetInteger(1,1.5*left);
				else next_stop = randomGen->GetInteger(1,left);
				if ((Simulator::Now().GetSeconds()+next_stop) < (simulation_duration-5)){
					Simulator::Schedule (MilliSeconds(next_stop*1000), &SimpleRing::Leave, this);
				}
			}
		}
	}
	if (file_list_update_type == "client_timed") updateFileListTimed();
	genFReqs();
}

Ptr<Socket> SimpleRing::StartSocket(void){
	Ptr<Socket> socket;
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 20);
	socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
	socket->Bind(local);
	socket->Listen();
	socket->SetAcceptCallback (MakeCallback (&SimpleRing::ConnectionRequest, this),MakeCallback(&SimpleRing::AcceptConnection, this));
	socket->SetRecvCallback (MakeCallback (&SimpleRing::ReceivePacket, this));
	socket->SetCloseCallbacks(MakeCallback (&SimpleRing::NormalClose, this), MakeCallback (&SimpleRing::ErrorClose, this));
	socket->SetAllowBroadcast (true);
	NS_LOG_DEBUG(m_address << " StartSocket (" << socket << ")");
	m_map[socket] = malloc(0);
	m_map_last_pos[socket] = 0;
	return socket;
}

void SimpleRing::NormalClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
}

void SimpleRing::ErrorClose(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
}

bool SimpleRing::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << InetSocketAddress::ConvertFrom (from).GetIpv4());
	return true;
}

void SimpleRing::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from){
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	socket->SetRecvCallback (MakeCallback (&SimpleRing::ReceivePacket, this));
}

void SimpleRing::ConnectSuccess(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
}

void SimpleRing::ConnectFail(Ptr<Socket> socket){
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
}

void SimpleRing::StopApplication(void)
{
	NS_ASSERT(m_discovery);
	if (m_is_running) {
		cout << m_left << ":" << m_left_port << "\t<--- " << m_address << " --->\t" << m_right << ":" << m_right_port << endl;
		Leave();
		stringstream buffer;
		m_address.Print(buffer);
		m_discovery->remove(m_address, m_port);
		m_discovery = NULL;
		m_socket->Close();
		m_socket = 0;
		m_gen_traffic = false;
		Simulator::Remove(last_event); // Forçar remover evento para terminar logo. Senão pode demorar muito tempo a terminar.
	}
}

void SimpleRing::Setup(VirtualDiscovery *discovery, bool messages, bool churn, bool random_sleep, 
											 uint32_t sleep_duration_min, uint32_t sleep_duration_max, uint32_t fixed_sleep_duration, string list_update_type, uint32_t list_update_time, uint32_t duration, Ptr<UniformRandomVariable> rand, bool to_trace, fstream *trace, string churn_int)
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

  use_churn = churn;
  simulation_duration = duration;

  churn_intensity = churn_int;

  trace_to_file = to_trace;
  trace_file = trace;

  randomGen = rand;
}

void SimpleRing::readPacket(Ptr<Socket> socket, Address from){
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
			if (m_right == Ipv4Address("0.0.0.0")){
				m_right = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
				m_right_port = 20;
				Change(answer, "M_RIGHT", m_address, m_port);
			}else{
				Change(answer, "M_RIGHT", m_right, m_right_port);
			}
			answer << "END" << endl;
			Send(answer, InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
			answer.str(string());
			if (m_left == Ipv4Address("0.0.0.0")){
				m_left = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
				m_left_port = 20;
				Change(answer, "M_LEFT", m_address, m_port);
				answer << "END" << endl;
				Send(answer, m_left, m_left_port);
			}else{
				Change(answer, "M_LEFT", InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
				answer << "END" << endl;
				Send(answer, m_right, m_right_port);
			}
			m_right = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
			m_right_port = 20;
		}else if(param == "LEAVE"){
			NS_LOG_DEBUG(m_address << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " is leaving the network");
		}else if(param == "CHANGE"){
			char * endptr;
			getline(request, param);
			if (param == "M_RIGHT"){
				getline(request, param);
				m_right = (Ipv4Address)(param.c_str());
				getline(request, param);
				m_right_port = (uint16_t)strtoimax(param.c_str(), &endptr, 10);
			}else if(param == "M_LEFT"){
				getline(request, param);
				m_left = (Ipv4Address)(param.c_str());
				getline(request, param);
				m_left_port = (uint16_t)strtoimax(param.c_str(), &endptr, 10);
			}else{
				NS_LOG_DEBUG("Fail to check CHANGE side");
			}
		}else if(param == "GET_FILE"){
			getline(request, param);
			string fid = (string)(param.c_str());
			getline(request, param);
			Ipv4Address reqFrom = (Ipv4Address)(param.c_str());
			if (reqFrom == m_address){
				ReqFile(fid, reqFrom, 0, true);
			}
			else{
				getline(request, param);
				uint32_t hops = (uint32_t)(atoi(param.c_str()));
				hops++;
				auto fP = find(begin(m_file_list), end(m_file_list), fid);
				if(fP!=end(m_file_list)){
					answer.str(string());
					answer << "SEND_FILE" << endl;
					answer << fid << endl;
					answer << hops << endl;
					answer << "END" << endl;
					Send(answer, reqFrom, 20);
	        if(trace_to_file){
	          stringstream msg;
	          msg << m_address << ", " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << ", " << fid << ", " << hops << ", " << Simulator::Now().GetSeconds () << endl;
	          trace_file->write(msg.str().c_str(), msg.str().size()+1);
	          trace_file->flush();
	        }
					continue;
				}else{
					ReqFile(fid, reqFrom, hops);
				}
			}
		}else if(param == "SEND_FILE"){
			getline(request, param);
			m_file_list.emplace(m_file_list.end(), param);
			string file = param;
			getline(request, param);
			uint32_t hops = (uint32_t)(atoi(param.c_str()));
			if (file == file_requested && m_is_running){
				file_requested = "\"File 0\"";
				cout << m_address << " Got " << file << " from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " (" << hops << " hops) - " << Simulator::Now().GetSeconds() <<endl;
			}
		}else if(param == "FILE_LIST_UPDATE"){
			getline(request, param);
			n_files_available = atoi(param.c_str());
		}else{
			keep = false;
		}
		getline(request, param);
	}
	free(m_map[socket]);
	socket->Close();
}

void SimpleRing::ReceivePacket (Ptr<Socket> socket)
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

void SimpleRing::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, data, size);
}

void SimpleRing::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void SimpleRing::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port){
		socket->Connect (InetSocketAddress (d_address, port));
	}else{
		socket->Connect (InetSocketAddress (d_address));
	}	
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void SimpleRing::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i){
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
		Simulator::Schedule (Seconds(0.00001), &SimpleRing::SendPacket, this, socket, data, size, i);
	}else{
		socket->Close();
	}
}

bool SimpleRing::ReqFile(string fid, Ipv4Address reqAddress, uint32_t hops, bool toServer){
	if (m_right != Ipv4Address("0.0.0.0") && m_right != m_address && m_left != Ipv4Address("0.0.0.0") && m_left != m_address && toServer == false){
		stringstream request;
		request << "GET_FILE" << endl;
		request << fid << endl;
		if (reqAddress != Ipv4Address("0.0.0.0")){
			request << reqAddress << endl;
		}else{
			request << m_address << endl;
		}
		request << hops << endl;
		request << "END" << endl;
		Send(request, m_right, m_right_port);
		return true;
	}else{
		toServer = true;
	}
	if(toServer){
		stringstream request;
		request << "GET_FILE" << endl;
		request << fid << endl;
		request << m_address << endl;
		request << hops << endl;
		request << "END" << endl;
		Send(request, m_server, 9);
		return true;
	}
	return false;
}

void SimpleRing::Change(stringstream& stream, string side, Ipv4Address ip, uint16_t port){
	NS_ASSERT(side == "M_RIGHT" || side == "M_LEFT");
	stream << "CHANGE" << endl;
	stream << side << endl;
	stream << ip << endl;
	stream << port << endl;
}

void SimpleRing::Leave(void){
	stringstream answer;
	if (m_left != Ipv4Address("0.0.0.0") && m_left != m_address){
		answer << "LEAVE" << endl;
		Change(answer, "M_RIGHT", m_right, m_right_port);
		answer << "END" << endl;
		Send(answer, m_left, m_left_port);
	}
	answer.str(string());
	if (m_right != Ipv4Address("0.0.0.0") && m_right != m_address){
		answer << "LEAVE" << endl;
		Change(answer, "M_LEFT", m_left, m_left_port);
		answer << "END" << endl;
		Send(answer, m_right, m_right_port);
	}

	// Clear data for a fresh start.
	file_requested = "\"File 0\"";
	m_left = Ipv4Address("0.0.0.0");	
	m_left_port = 0;
	m_right = Ipv4Address("0.0.0.0");
	m_right_port = 0;
	m_file_list.clear();
	m_discovery->remove(m_address, m_port);

	cout << m_address << " LEAVING (" << Simulator::Now().GetSeconds() << "s)" << endl;
	m_is_running = false;
	if(use_churn) {
		if(!Simulator::IsFinished()){
			uint32_t left = simulation_duration - Simulator::Now().GetSeconds() - 5;
			if (left > 0){
				uint32_t next_start;
				if (churn_intensity == "low")	next_start = randomGen->GetInteger(1,2*left);
				else if (churn_intensity == "medium") next_start = randomGen->GetInteger(1,1.5*left);
				else next_start = randomGen->GetInteger(1,left);
				if ((Simulator::Now().GetSeconds()+next_start) < (simulation_duration-5)){
					Simulator::Schedule (MilliSeconds(next_start)*1000, &SimpleRing::DoWork, this);
				}
			}
		}
	}
}

void SimpleRing::Join(void){
	cout << m_address << " JOINING (" << Simulator::Now().GetSeconds() << "s)" << endl;
	tuple<Ipv4Address, uint16_t> ip = m_discovery->discover();
	if (ip != make_tuple(Ipv4Address("0.0.0.0"), 0)){
		NS_LOG_DEBUG(m_address << " is JOIN " << get<0>(ip) << ":" << get<1>(ip) << endl);
		Send("JOIN\nEND\n", get<0>(ip), get<1>(ip));
		m_left = get<0>(ip);
		m_left_port = get<1>(ip);
	}
	m_discovery->add(m_address, m_port);
}

void SimpleRing::updateFileList(void){
	stringstream request;
	request << "GET_FILE_LIST" << endl << m_address << endl << "END" << endl;
	Send(request, m_server, 9);
}

void SimpleRing::updateFileListTimed(void){
	if(m_is_running){
		updateFileList();
		if (m_gen_traffic){
			Simulator::Schedule (Seconds(file_list_update_time), &SimpleRing::updateFileListTimed, this);	
		}
	}
}

void SimpleRing::genFReqs(){
	if(m_is_running){
		if (file_list_update_type == "client_on_request"){
			updateFileList();
		}
		stringstream s;
		if (n_files_available > 0 && file_requested == "\"File 0\""){
			s << "\"File " << randomGen->GetInteger(1,n_files_available) << "\"";
			file_requested = s.str();
			cout << m_address << " Req: " << s.str() << " " << Simulator::Now().GetSeconds() << endl;
			ReqFile(s.str(), m_address);
		}
		if (m_gen_traffic){
	    if (random_client_sleep){
	    	uint32_t next, mod = client_sleep_duration_max - client_sleep_duration_min;
	    	next = randomGen->GetInteger(1,mod + client_sleep_duration_min);
	    	if (next > 0 && (Simulator::Now().GetSeconds()+next) < simulation_duration){
	      	last_event = Simulator::Schedule (Seconds(next), &SimpleRing::genFReqs, this);
	    	}
	    }
	    else{
	    	if (client_fixed_sleep_duration > 0 && (Simulator::Now().GetSeconds()+client_fixed_sleep_duration) < simulation_duration){
	      	last_event = Simulator::Schedule (Seconds(client_fixed_sleep_duration), &SimpleRing::genFReqs, this);
	      }
	    }
		}
	}
}
