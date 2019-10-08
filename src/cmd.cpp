#include "cmd.h"
#include "common.h"
#include "shell.h"
#include "net_addr.h"

void add_SatBridge(int id)
{
	string br = cmd(SAT_BR_NAME, id);
	run(ovs_add_br(br));
	run(ovs_set_mode(br));
	// add ovs_sat port
	string port = cmd("s",id,"-l");
	run(dev_set_stat(port,true));
	run(ovs_add_port(br,port,1));
	// add flow
	run(ovs_add_flow(br,"arp,arp_op=1,actions="
		"move:\"NXM_OF_ETH_SRC[]->NXM_OF_ETH_DST[]\","
		"mod_dl_src:\"FF:FF:FF:FF:FF:FF\","
		"load:\"0x02->NXM_OF_ARP_OP[]\","
		"move:\"NXM_NX_ARP_SHA[]->NXM_NX_ARP_THA[]\","
		"load:\"0XFFFFFFFFFFFF->NXM_NX_ARP_SHA[]\","
		"push:\"NXM_OF_ARP_SPA[]\","
		"move:\"NXM_OF_ARP_TPA[]->NXM_OF_ARP_SPA[]\","
		"pop:\"NXM_OF_ARP_TPA[]\","
		"in_port")); // a simple arp agent
	run(ovs_add_flow(br,"in_port=1,priority=10,actions=output:flood"));
	run(ovs_add_flow(br,"priority=0,actions=output:1"));
}

void del_SatBridge(int id)
{
	run(ovs_del_br(cmd(SAT_BR_NAME,id)));
}

void add_HostBridge(int id)
{
	string br = cmd(HOST_BR_NAME, id);
	run(ovs_add_br(br));
	run(ovs_set_mode(br));
	// add link to sbr
	string tap1 = cmd(br,"p0");
	string tap2 = cmd(HOST_NAME,id,"p1");
	run(link_add(tap1,tap2));
	run(ovs_add_port(br,tap1,1));
	run(dev_set_stat(tap1,true));
	run(dev_set_stat(tap2,true));
	// add link to host
	string port = cmd(br,"p1");
	run(ovs_add_port(br,port,2));
	// add link to real host
	string vtap = cmd(br,"p2");
	run(dev_vxlan_add(vtap,id,cmd("192.168.0.",id+1),"192.168.0.1"));
	run(dev_set_stat(vtap,true));
	run(ovs_add_port(br,vtap,3));
	// add flow to hbr
	run(ovs_add_flow(br,"in_port=2,ip,nw_dst=100.0.0.1,priority=10,actions=output:1"));
	run(ovs_add_flow(br,"in_port=2,priority=5,actions=drop"));
	run(ovs_add_flow(br,"in_port=1,ip,nw_src=100.0.0.1,priority=10,actions=output:2"));
	run(ovs_add_flow(br,"in_port=1,priority=0,actions=output:3"));
	run(ovs_add_flow(br,"in_port=3,priority=0,actions=output:1"));
}

void del_HostBridge(int id)
{
	string br = cmd(HOST_BR_NAME,id);
	run(ovs_del_br(br));
	// del hbr-sbr
	string port = cmd(HOST_NAME,id,"p1");
	run(dev_set_stat(port,false));
	run(dev_del(port));
	// del vtap
	string vtap = cmd(br,"p2");
	run(dev_set_stat(vtap,false));
	run(dev_del(vtap));	
}

void add_HostNs(int id, bool real, int addr, int mask)
{
	// add ns
	string ns = cmd(HOST_NAME,id);
	run_q(ns_add(ns));
	run_q(ns_do(ns,dev_set_stat("lo",true)));
	// add port
	string tap1 = ns + "p0", tap2 = "";
	tap2 = real ? cmd(HOST_BR_NAME,id,"p1") : (ns + "p1");
	run_q(link_add(tap1,tap2));
	run_q(ns_add_port(ns,tap1));
	run_q(ns_do(ns,dev_set_stat(tap1,true)));
	run(dev_set_stat(tap2,true));
	// set port-ip and default-gw-ip/mac
	run_q(ns_do(ns,dev_addr_add(tap1,IpStr(addr,mask),BIpStr(addr,mask))));
	string gw = IpStr(addr - id);
	run_q(ns_do(ns,dev_route_add_default(tap1,gw)));
	run_q(ns_do(ns,dev_neigh_add_brd(tap1,gw)));
	// to do : tc_config
}

void del_HostNs(int id)
{
	run(ns_del(cmd(HOST_NAME,id)));
}

void change_DevMaster(int id,int oldSbrId,int newSbrId)
{
	string port = cmd(HOST_NAME,id,"p1");
	if(oldSbrId != -1){
		run(ovs_del_port(port));	
	}
	if(newSbrId != -1){
		run(ovs_add_port(cmd(SAT_BR_NAME,newSbrId),port));
	}
}

void CloseBgFlows()
{
	run(kill("iperf"));
}

void host_exec(int id,string command)
{
	string ns = cmd(HOST_NAME,id);
	run(ns_do(ns,command));
}
