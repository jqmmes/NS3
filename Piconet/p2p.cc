#include "p2p.h"
#include <inttypes.h>
#include <ns3/node-list.h>
#include <cstdint>

NS_LOG_COMPONENT_DEFINE ("p2pApplication");

using namespace ns3;
using namespace std;

TypeId
p2p::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::p2p")
			.SetParent<Application> ()
			.SetGroupName("Applications")
			.AddConstructor<p2p> ()
			.AddAttribute ("PeerFull", "If the connections are full.", // Um bocado hackish, mas funciona.
										BooleanValue (false),
										MakeBooleanAccessor (&p2p::m_peer_full),
										MakeBooleanChecker ())
	;
	return tid;
}

p2p::p2p()
{
	NS_LOG_DEBUG("Starting p2p");
}

p2p::~p2p()
{
	NS_LOG_DEBUG("Destroy p2p");
}

void p2p::StartApplication(void)
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

	Formation(0, randomGen->GetInteger(5,12));

	if (m_address == Ipv4Address("0.0.0.1"))
	{
		Simulator::Schedule(Seconds(1000),  &p2p::Gossip, this, "END\n", -1);
	}
}

//https://cseweb.ucsd.edu/~marzullo/pubs/harary.pdf
void p2p::Gossip(string msg, uint32_t msg_id)
{
	vector<Ipv4Address> broadcast_peers;
	vector<Ipv4Address>::iterator peer;
	if (msg_id == -1) // If is a new message we don't have a msg_id.
	{
		msg_id = randomGen->GetInteger(0, INT_MAX);
		m_gossip_msg_map[msg_id] = vector<Ipv4Address> (); // Start a new message group.
		stringstream msg_stream;
		msg_stream << "GOSSIP\n" << msg_id << "\n" << msg; // Add msg_id to message and mark it as Gossip.
		broadcast_peers = GetRandomPeers(m_connections, m_Binitial);
		for (peer = begin(broadcast_peers); peer != end(broadcast_peers); ++peer)
		{
			m_gossip_msg_map[msg_id].emplace_back((*peer));
			cout << Simulator::Now().GetSeconds() << "\t" << m_address << "  \tSEND_GOSSIP\t" << (*peer) << endl;
			Send(msg_stream.str(), (*peer));
		}
	}
	else
	{
		NS_ASSERT(m_gossip_msg_map[msg_id]);
		if (m_gossip_msg_n_rcv[msg_id] < m_F) // Number of times received this message < F
		{
			broadcast_peers = GetRandomPeers(GetVirginPeers(msg_id), m_B);
			for (peer = begin(broadcast_peers); peer != end(broadcast_peers); ++peer)
			{
				m_gossip_msg_map[msg_id].emplace_back((*peer));
				cout << Simulator::Now().GetSeconds() << "\t" << m_address << "  \tSEND_GOSSIP\t" << (*peer) << endl;
				Send(msg, (*peer));
			}
		}
	}
}

vector<Ipv4Address> p2p::GetVirginPeers(uint32_t MsgId)
{
	NS_ASSERT(m_gossip_msg_map[MsgId]);
	vector<Ipv4Address> peers;
	vector<Ipv4Address>::iterator peer;
	for (peer = begin(m_connections); peer != end(m_connections); ++peer)
	{
		if (find(begin(m_gossip_msg_map[MsgId]), end(m_gossip_msg_map[MsgId]), (*peer)) == end(m_gossip_msg_map[MsgId]))
		{
			peers.emplace_back((*peer));
		}
	}
	return peers;
}

vector<Ipv4Address> p2p::GetRandomPeers(vector<Ipv4Address> VirginPeers, uint32_t B)
{

	vector<Ipv4Address> peers, AuxVirginPeers;
	uint32_t i;
	copy(begin(VirginPeers), end(VirginPeers), back_inserter(AuxVirginPeers)); // We need to preserve original data.

  	while(B > 0 && begin(AuxVirginPeers) != end(AuxVirginPeers)) // For B peers, or until max peers available.
  	{
  		i = randomGen->GetInteger(0, distance(begin(AuxVirginPeers), end(AuxVirginPeers))-1); // Choose a random element
  		peers.emplace_back(AuxVirginPeers[i]); // Emplace element at the end of return vector
  		AuxVirginPeers.erase(begin(AuxVirginPeers)+i); // Remove selected element from original list
		--B; // Decrease B
  	}
	return peers;
}

void p2p::Formation(uint32_t i, uint32_t limit)
{
	if (m_connected)
	{
		return;
	}
	if (i == 0)
	{
		cout << Simulator::Now().GetSeconds() << "   \t" << m_address << "  \tSTART_DISCOVERY  \t" << limit << "s" << endl;
	}
	setDiscoverable(false);
	vector<tuple<Ipv4Address,uint16_t>> devices = m_discovery->getAll(); // devices = discovery()
	if (distance(devices.begin(), devices.end()) > 0)
	{
		// FOR device : devices
		vector<tuple<Ipv4Address, uint16_t>>::iterator iterator;
		uint32_t n_peers = 0;
		for (iterator = devices.begin(); iterator != devices.end(); ++iterator)
		{
			if (!isPeerFull(getNode(get<0>(*iterator))))
			{
				Send("FORMATION\nJOIN\nEND\n", get<0>(*iterator), 20);
				// Start a new socket to this new device.
				cout << Simulator::Now().GetSeconds() << "  \t" << m_address << "  \tFOUND_DEVICE\t"
				 	 << get<0>(*iterator) << "  \t" << i << "*" << m_discovery_timer << "ms" << endl;
				++n_peers;
				if (n_peers >= m_min_peers)
				{
					break;
				}
			}

		}
		if (n_peers > 0)
		{
			return;
		}
	}

	if (i < limit*10)
	{
		Simulator::Schedule(MilliSeconds(m_discovery_timer), &p2p::Formation, this, ++i, limit);
	}
	else
	{
		setDiscoverable(true);
		if (distance(m_connections.begin(), m_connections.end()) == 0)
		{
			cout << Simulator::Now().GetSeconds() << "\t" << m_address << "  \tTIMEOUT_DISCOVERY_NO_PEERS" << endl;
			uint32_t timeout = 20000; //wait(timeout)
			Simulator::Schedule (MilliSeconds(timeout), &p2p::Formation, this, 0, 
													 randomGen->GetInteger(5,12)); // goto start
		}
	}
}

Ptr<Socket> p2p::StartSocket(void)
{
	Ptr<Socket> socket;
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 20);
	socket = Socket::CreateSocket (m_node, TcpSocketFactory::GetTypeId ());
	socket->Bind(local);
	socket->Listen();
	socket->SetAcceptCallback (MakeCallback (&p2p::ConnectionRequest, this),
														 MakeCallback(&p2p::AcceptConnection, this));
	socket->SetCloseCallbacks(MakeCallback (&p2p::NormalClose, this), 
														MakeCallback (&p2p::ErrorClose, this));
	socket->SetAllowBroadcast (true);
	NS_LOG_DEBUG(m_address << " StartSocket (" << socket << ")");
	m_map[socket] = malloc(0);
	m_map_last_pos[socket] = 0;
	return socket;
}

void p2p::NormalClose(Ptr<Socket> socket)
{
	NS_LOG_DEBUG(m_address << " NormalClose (" << socket << ")");
	// Escutar aqui para a falha.
}

void p2p::ErrorClose(Ptr<Socket> socket)
{
	NS_LOG_DEBUG(m_address << " ErrorClose (" << socket << ")");
	// Escutar aqui para a falha.
}

bool p2p::ConnectionRequest(Ptr<Socket> socket, const ns3::Address& from)
{
	NS_LOG_DEBUG(m_address << " ConnectionRequest (" << socket << ") from " << 
							 InetSocketAddress::ConvertFrom (from).GetIpv4());
	cout << Simulator::Now().GetSeconds() << "\t" << m_address << "\tConnectionRequest" << endl;
	// Só aceitar se ainda houverem slots vazias
	return true;
}

void p2p::AcceptConnection(Ptr<Socket> socket, const ns3::Address& from)
{
	NS_LOG_DEBUG(m_address << " AcceptConnection (" << socket << ")");
	cout << Simulator::Now().GetSeconds()  << "\t"  << m_address << "\tAcceptConnection" << endl;
	// Posso por aqui na lista de sockets abertas, ou nao.
	socket->SetRecvCallback (MakeCallback (&p2p::ReceivePacket, this));
}

void p2p::ConnectSuccess(Ptr<Socket> socket)
{	
	NS_LOG_DEBUG(m_address << " ConnectSuccess (" << socket << ")");
	// Esta cena não esta a ser chamada nunca. Tentar perceber quando é que isto é usado.
	cout << Simulator::Now().GetSeconds()  << "\t"  << m_address << "\tConnectionSuccess" << endl;
}

void p2p::ConnectFail(Ptr<Socket> socket)
{
	NS_LOG_DEBUG(m_address << " ConnectFail (" << socket << ")");
	cout << Simulator::Now().GetSeconds()  << "\t"  << m_address << "\tConnectionFail" << endl;
}

void p2p::StopApplication(void)
{
	NS_ASSERT(m_discovery);
	m_discovery->remove(m_address, 20);
}

void p2p::Setup(VirtualDiscovery *discovery, uint32_t minPeers, uint32_t minDiscoveryTimeout, uint32_t maxDiscoveryTimeout, 
				uint32_t minIdleTimeout, uint32_t maxIdleTimeout, uint32_t discoveryTimer, uint32_t gossipBinitial, uint32_t gossipB, uint32_t gossipF, 
				Ptr<UniformRandomVariable> rand)
{
	m_discovery = discovery;
	m_min_peers = minPeers;
	m_discovery_timer = discoveryTimer;
	m_min_discovery_timeout = minDiscoveryTimeout;
	m_max_discovery_timeout = maxDiscoveryTimeout;
	m_min_idle_timeout = minIdleTimeout;
	m_max_idle_timeout = minIdleTimeout;
	m_Binitial = gossipBinitial;
	m_B = gossipB;
	m_F = gossipF;
	randomGen = rand;
}

void p2p::ReadPacket(Ptr<Socket> socket, Address from)
{
	stringstream answer; /*!< iostream of a possible answer */
	stringstream request; /*!< iostream of a request */
	string param; /*!< param requested in the packet. Extracted from request. */
	bool keep = true;
	request << string((char*)m_map[socket]);
	getline(request, param);
	while (keep){
		NS_LOG_DEBUG(param);
		if (param == "END")
		{
			keep = false;
		}
		else if(param == "FORMATION")
		{
			getline(request, param);
			if (param == "JOIN")
			{
				 if (distance(m_connections.begin(), m_connections.end()) < MAX_CONNECTIONS)
				 {
				 	Send("FORMATION\nACCEPT\nEND\n", InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
				 	m_connections.emplace(m_connections.end(), InetSocketAddress::ConvertFrom (from).GetIpv4 ());
				 	if (distance(m_connections.begin(), m_connections.end()) >= MAX_CONNECTIONS)
				 	{
				 		m_node->GetApplication(0)->SetAttribute("PeerFull", BooleanValue(true));
				 	}
				 	m_connected = true;
				 }
				 else
				 {
				 	Send("FORMATION\nREJECT\nEND\n", InetSocketAddress::ConvertFrom (from).GetIpv4 (), 20);
				 }
			}
			else if (param == "ACCEPT")
			{
				m_connections.emplace(m_connections.end(), 
															InetSocketAddress::ConvertFrom (from).GetIpv4 ());
				m_discovery->add(m_address, 20);
				cout << Simulator::Now().GetSeconds() << "  \t" << m_address << "  \tNEW_PEER\t" << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl;
				setDiscoverable(true);
				m_connected = true;
				if (distance(m_connections.begin(), m_connections.end()) >= MAX_CONNECTIONS)
			 		m_node->GetApplication(0)->SetAttribute("PeerFull", BooleanValue(true));
			}
			else if (param == "REJECT")
			{
				cout << Simulator::Now().GetSeconds() << "  \t" << m_address 
					 << "  \tPEER_REJECTED\t" << InetSocketAddress::ConvertFrom (from).GetIpv4 () 
					 << endl;
				if (!m_connected)
				{
					Simulator::ScheduleNow(&p2p::Formation, this, 0, randomGen->GetInteger(5,12));
				}
			}
		}
		else if (param == "GOSSIP")
		{
			cout << Simulator::Now().GetSeconds() << "\t" << m_address << "  \tRECEIVED_GOSSIP\t" << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl;
			getline(request, param); // Parse MsgId.
			uint32_t msg_id = atoi(param.c_str());
			m_gossip_msg_map[msg_id].emplace_back(InetSocketAddress::ConvertFrom (from).GetIpv4 ()); // We know that who sent us msg, knows it already. We wont send it back.
			if (m_gossip_msg_n_rcv.find(msg_id) == end(m_gossip_msg_n_rcv))
			{
				m_gossip_msg_n_rcv[msg_id] = 0;
			}
			++m_gossip_msg_n_rcv[msg_id];
			Gossip(request.str(), msg_id); // Continue Gossip Algorithm.
		}else{
			keep = false;
		}
		getline(request, param);
	}
	free(m_map[socket]);
	socket->Close();
}

/**
 *	Low level functions to Send/Receive data. 
 */

void p2p::ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> p; /*!< Received packet */
	Address from; /*!< Origin address of the packet*/
	void *local;

	while(socket->GetRxAvailable () > 0)
	{
		p = socket->RecvFrom (from);//socket->Recv (socket->GetRxAvailable (), 0);
		m_map[socket] = realloc(m_map[socket], m_map_last_pos[socket]+(sizeof(uint8_t)*p->GetSize()));
		p->CopyData (reinterpret_cast<uint8_t *> (m_map[socket])+m_map_last_pos[socket], p->GetSize()); 
		local = malloc(p->GetSize()*sizeof(uint8_t));
		p->CopyData (reinterpret_cast<uint8_t *>(local), p->GetSize());
		m_map_last_pos[socket] += sizeof(uint8_t)*p->GetSize();

		// If the message as been fully received, lets process the request.
		// To check if it's complete lets check if:
		if (string(reinterpret_cast<char *>(local)).length() >= 4)
		{
			if (string(reinterpret_cast<char *>(local)).rfind("END\n") == 
				string(reinterpret_cast<char *>(local)).length()-4)
			{
				ReadPacket(socket, from);
			}
		}
		else
		{
			if (string((char*)m_map[socket]).rfind("END\n") == 
				string((char*)m_map[socket]).length()-4)
			{
				ReadPacket(socket, from);
			}
		}
		free(local);
	}
}

void p2p::Send(const uint8_t * data, uint32_t size, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port)
	{
		socket->Connect (InetSocketAddress (d_address, port));
	}
	else
	{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, data, size);
}

void p2p::Send(string sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port)
	{
		socket->Connect (InetSocketAddress (d_address, port));
	}
	else
	{
		socket->Connect (InetSocketAddress (d_address));
	}
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.c_str()), sdata.size()+1);
}

void p2p::Send(stringstream& sdata, Ipv4Address d_address, uint16_t port)
{
	Ptr<Socket> socket = StartSocket();
	if (port)
	{
		socket->Connect (InetSocketAddress (d_address, port));
	}
	else
	{
		socket->Connect (InetSocketAddress (d_address));
	}	
	SendPacket(socket, reinterpret_cast<const uint8_t *>(sdata.str().c_str()), sdata.str().size()+1);
}

void p2p::SendPacket(Ptr<Socket> socket, const uint8_t* data, uint32_t size, uint32_t i)
{
	void * p_data = malloc(0);
	uint32_t c_size = min(m_packet_size, size-i);
	bool err = false;
	while((socket->GetTxAvailable() > c_size) && (i < size))
	// Não deixar esgotar o buffer em envios muito grandes.
	{
		p_data = realloc(p_data, c_size*sizeof(uint8_t));
		memcpy(p_data, &data[i], c_size*sizeof(uint8_t));
		if ((socket->Send(Create<Packet> (reinterpret_cast<const uint8_t *>(p_data), c_size)) >= 0))
		{
			NS_LOG_DEBUG("Send Success");
		}
		else
		{
			NS_LOG_DEBUG("Send Fail " << socket->GetErrno () );
			err = true;
			break;
		}
		i += c_size;
		c_size = min(m_packet_size, size-i);
	}
	if (i < size && !err)
	{
		Simulator::Schedule (Seconds(0.00001), &p2p::SendPacket, this, socket, data, size, i);
	}
	else
	{
		socket->Close();
	}
}

/**
 *	Internal functions.
 */

bool p2p::isPeerFull(Ptr<Node> node)
{
	BooleanValue isFull;
	node->GetApplication(0)->GetAttribute("PeerFull", isFull);
	return BooleanValue(isFull);
}

Ptr<Node> p2p::getNode(Ipv4Address ip)
{
	vector< Ptr< Node > >::const_iterator it;
	for (it = NodeList::Begin(); it != NodeList::End(); ++it)
	{
		for (uint32_t i = 0; i < (*it)->GetNDevices(); i++)
		{
			if ((*it)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() == ip)
				return (*it);
		}
	}
	return NULL;
}

void p2p::setDiscoverable(bool status)
{
	if (status)
	{
		m_discovery->add(m_address, 20);
	}
	else
	{
		m_discovery->remove(m_address, 20);
	}
}
