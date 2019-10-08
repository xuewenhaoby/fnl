#include "node.h"
#include "common.h"
#include "cmd.h"
#include "timer.h"
#include "file.h"
#include "net_addr.h"
#include "calculate.h"
#include <iostream>
#include <algorithm>
using namespace std;

/******************************class SatNode*****************************/
SatNode::SatNode()
{
	name = SAT_NAME;
	br_name = SAT_BR_NAME;
}

void SatNode::nodeInitialize()
{
	setPos(ReadLocFile(id,GetTime())); // init pos
	real = (SatPhyNodes.count(id) == 1) ? true : false;
	// init sbr
	if(enablePhysical && enableInitPhy){
		if(real){
			print(cmd("Real Sat Node ",id));
		}else{
			add_SatBridge(id);
		}
	}
}

void SatNode::nodeFinalize()
{
	if(enablePhysical && enableFinaPhy){
		del_SatBridge(id);
	}
}

void SatNode::updatePos()
{
	setPos(ReadLocFile(id,GetTime()));
}

/******************************class HostNode*****************************/
HostNode::HostNode()
{
	name = HOST_NAME;
}

void HostNode::nodeInitialize()
{
	real = (HostPhyNodes.count(id) == 1) ? true : false;
	/* initialize position */
	int connect_satId = getId();
	setPos(ReadLocFile(connect_satId,GetTime()));
	//connect_satId = ;


	areaId = S_A[INDEX(connect_satId)];
	int area_satId = connect_satId;
	// areaId = getId();
	// int area_satId = GetSatIdByAreaId(areaId);

	print(cmd("user",id," is in area ",areaId,"(sat:",area_satId,")"));
	if(enablePhysical && enableInitPhy){
		addr = areaId*256*256*256 + id + 1;
		add_HostNs(id,real,addr,8);
		if(real){
			print(cmd("Real Host Node ",id));
			add_HostBridge(id);
		}			
	}
	// initialize link_stat[]
	for(int i = 0; i < SAT_NUM; i++){
		link_stat[i].satId = i+1;
		link_stat[i].distance = COVER_RADIUS+1;
		link_stat[i].isCovered = false;
	}
	// initialize link_use
	link_use.clear();
	// start iperf_server
	if(enablePhysical && enableBgFlows){
		iperf_server();
	}
	// test
	updateLink();
}

void HostNode::nodeFinalize()
{
	if(enablePhysical && enableFinaPhy){
		del_HostNs(id);
		if(real){
			del_HostBridge(id);
		}
	}
}

void HostNode::updatePos()
{
	setPos(ReadLocFile(GetSatIdByAreaId(areaId),GetTime()));
}

void HostNode::updateLink()
{	
	// updatePos();

	LinkState old_link = {-1,COVER_RADIUS+1,false};
	if(!link_use.empty()){
		old_link = link_use[0];
	}
	link_use.clear();

	for(int i = 0; i < SAT_NUM; i++)
	{
		double distance = CalculateDistance(pos.loc, Sats[i].getPos().loc);
		bool isCovered = distance < COVER_RADIUS;

		link_stat[i].distance = distance;
		link_stat[i].isCovered = isCovered;

		if(isCovered){
			link_use.push_back(link_stat[i]);				
		}
	}
	sort(link_use.begin(),link_use.end(),Compare);
	
	LinkState new_link = {-1,COVER_RADIUS+1,false};
	if(!link_use.empty()){
		new_link = link_use[0];
	}

	if(old_link.satId != new_link.satId)
	{
			if(enablePhysical){
					change_DevMaster(id,old_link.satId,new_link.satId);
					sendRegisterPacket();
			}
			if(enablePrintHostLinkStat){
				print(cmd("user:",id," from ",old_link.satId," change to ",new_link.satId));
			}
	}
}

void HostNode::sendRegisterPacket()
{
	host_exec(id,"./bin/send register");
}

void HostNode::iperf_server()
{
	host_exec(id,"iperf -u -s > /dev/null &");
}

void HostNode::iperf_client()
{
	int dstIdx = rand() % 6;
	//int dstIdx = 2;
	int addr = USER(dstIdx).getAddr();
	host_exec(id,cmd("iperf -u -c ",IpStr(addr)," -b 10M -t 1000 -i 2 > host_app/iperf/iperf_log",id,".txt &"));
}
/******************************API Funcs***********************************/
bool Compare(LinkState a, LinkState b)
{
	return a.distance < b.distance; // "<":asc ">":desc
}