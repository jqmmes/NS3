#include "VirtualDiscovery.h"


NS_LOG_COMPONENT_DEFINE ("VirtualDiscovery");

VirtualDiscovery::VirtualDiscovery(Ptr<UniformRandomVariable> rand)
{
  randomGen = rand;
  NS_LOG_DEBUG("Init VirtualDiscovery");
}

VirtualDiscovery::~VirtualDiscovery()
{
  NS_LOG_DEBUG("Destroy VirtualDiscovery"); 
}

void VirtualDiscovery::add(Ipv4Address ip, uint16_t port)
{
  i_mutex.lock();
  m_ips.emplace(m_ips.end(), make_tuple(ip, port));
  i_mutex.unlock();
}

tuple<Ipv4Address,uint16_t> VirtualDiscovery::discover(void)
{
  if (distance(begin(m_ips), end(m_ips)) == 0)
  {
    return make_tuple(Ipv4Address("0.0.0.0"), 0);
  }
  //int i = rand() % distance(begin(m_ips), end(m_ips));
  uint32_t i = randomGen->GetInteger(1,INT_MAX) % distance(begin(m_ips), end(m_ips));
  return m_ips.at(i);
}

vector<tuple<Ipv4Address,uint16_t>> VirtualDiscovery::getAll(void)
{
  random_device rd;
  mt19937 g(rd());
  shuffle(m_ips.begin(), m_ips.end(), g);
  return m_ips;
}

uint32_t VirtualDiscovery::GetN(void){
  return (uint32_t)m_ips.size();
}

void VirtualDiscovery::remove(Ipv4Address ip, uint16_t port)
{
  o_mutex.lock();
  auto r1 = find(begin(m_ips), end(m_ips), make_tuple(ip, port));
  if(r1!=end(m_ips))
  {
    m_ips.erase(m_ips.begin()+distance(m_ips.begin(), r1));
  }else
  {
    NS_LOG_INFO("Element is not in vector");
  }
  o_mutex.unlock();
}