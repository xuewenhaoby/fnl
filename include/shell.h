#ifndef __SHELL_H__
#define __SHELL_H__

#include <iostream>
using namespace std;

inline string ovs_add_br(string br)
{
	return "ovs-vsctl add-br " + br;
}
inline string ovs_del_br(string br)
{
	return "ovs-vsctl del-br " + br;
}
inline string ovs_set_mode(string br)
{
	return "ovs-vsctl set-fail-mode " + br + " secure";
}
inline string ovs_add_port(string br, string port)
{
	return "ovs-vsctl add-port " + br + " " + port;
}
inline string ovs_add_port(string br, string port, int pid)
{
	return "ovs-vsctl add-port " + br + " " + port + " -- set Interface " + port + " ofport_request=" + to_string(pid);
}
inline string ovs_del_port(string port)
{
	return "ovs-vsctl del-port " + port;
}
inline string ovs_add_flow(string br, string flow)
{
	return "ovs-ofctl add-flow " + br + " " + flow;
}
inline string ovs_del_flow(string br, string match)
{
	return "ovs-ofctl del-flows " + br + " " + match;
}

inline string ns_add(string ns)
{
	return "ip netns add " + ns;
}
inline string ns_del(string ns)
{
	return "ip netns del " + ns;
}
inline string ns_add_port(string ns, string port)
{
	return "ip link set " + port + " netns " + ns;
}
inline string ns_do(string ns, string cmd)
{
	return "ip netns exec " + ns + " " + cmd;
}

inline string dev_set_stat(string dev, bool stat)
{
	return "ip link set " + dev + (stat ? " up":" down");
}
inline string dev_set_master(string dev, string br = "")
{
	return "ip link set " + dev + (br.empty() ? " nomaster" : (" master " + br));
}
inline string dev_set_name(string dev, string name)
{
	return "ip link set " + dev + " name " + name;
}
inline string dev_addr_add(string dev, string addr, string brd = "")
{
	return "ip addr add " + addr + (brd.empty() ? "" : (" brd " + brd)) + " dev " + dev;
}
inline string dev_route_add(string dev, string route)
{
	return "ip route add " + route + " dev " + dev;
}
inline string dev_neigh_add(string dev,string neigh)
{
	return "ip neigh add " + neigh + " dev " + dev;
}
inline string dev_route_add_default(string dev, string gw)
{
	return dev_route_add(dev,"default via " + gw);
}
inline string dev_neigh_add_brd(string dev, string addr)
{
	return dev_neigh_add(dev, addr + " lladdr ff:ff:ff:ff:ff:ff");
}
inline string dev_tc_add(string dev,string cmd)
{
	return "tc qdisc add dev " + dev + " " + cmd;
}
inline string dev_vxlan_add(string name,int id,string remote,string local)
{
	return "ip link add " + name + " type vxlan id " + to_string(id) + " remote " + remote + " local " + local;
}
inline string dev_del(string dev)
{
	return "ip link del " + dev;
}
inline string link_add(string tap1,string tap2)
{
	return "ip link add " + tap1 + " type veth peer name " + tap2;
}

inline string kill(string name)
{
	return "kill -9 `ps -e | grep " + name + " | awk '{print $1}'`";
}


#endif