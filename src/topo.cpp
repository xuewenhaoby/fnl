#include "topo.h"
#include "common.h"
#include "cmd.h"
#include "task.h"
#include "message.h"
#include "file.h"
#include "calculate.h"
#include <ctime>
#include <iostream>
using namespace std;

void TopoInitialize(int *host_num)
{
	int HOST_NUM = *host_num;
	cout << "Start TopoInitialize..." << endl;
	//cout <<HOST_NUM<<endl;
	time_t time0 = time(NULL);

	InitStaticTopo();

	// initialize satellites and sbrs
	for(int i = 0; i < SAT_NUM; i++){
		SAT(i).setId(i+1);
		Pool.addTask(new NodeTask(new Message(
			&SAT(i),MSG_NODE_Initialize)));
	}
	Pool.wait();
	// initialize SATable
	UpdateSATableByFile();
	Users.resize(HOST_NUM);
	// initialize hosts and hbr
	//HostNode User[HOST_NUM];
	for(int i=0; i<HOST_NUM; i++)
	{
		HostNode tmp;
		//Users.push_back(User[i]);
		Users.push_back(tmp);
		//Users[i].setId(i+1);
	}

	for(int i = 0; i < HOST_NUM; i++){ 
		//cout<<Users[i]<<endl;
		USER(i).setId(i+1);
		Pool.addTask(new NodeTask(new Message(
			&USER(i),MSG_NODE_Initialize)));
	}
	//*
	// if(enablePhysical && enableBgFlows){
	// 	// wait for iperf server finished
	// 	Pool.wait();
	// 	// initialize background flows
	// 	for(int i = 0; i < HOST_NUM; i++){
	// 		Pool.addTask(new NodeTask(new Message(
	// 			&USER(i),MSG_HOST_BgFlows)));
	// 	}
	// }
	//*/
	Pool.wait();

	time_t time1 = time(NULL);
	cout << "TopoInitialize finished:" << time1-time0 << endl;
}

void TopoFinalize(int* host_num)
{
	int HOST_NUM = *host_num;
	cout << "Start TopoFinalize..." << endl;
	time_t time0 = time(NULL);

	//delete satellites and sbrs
	for(int i = 0; i < SAT_NUM; i++){
		NodeTask *tk = new NodeTask(
			(void*)new Message(&SAT(i),MSG_NODE_Finalize));
		Pool.addTask(tk);
	}
	// delete hosts
	for(int i = 0; i < HOST_NUM; i++){
		NodeTask *tk = new NodeTask(
			(void*)new Message(&USER(i),MSG_NODE_Finalize));
		Pool.addTask(tk);
	}
	Pool.wait();
	// close background iperf
	if(enablePhysical && enableBgFlows){
		CloseBgFlows();
	}

	time_t time1 = time(NULL);
	cout << "TopoFinalize finished:" << time1-time0 << endl;

	cout << "Start clear thread Pool..." << endl;
	Pool.stopAll();
	cout << "Clear thread Pool finished." << endl;
}

void InitStaticTopo()
{
	// Set G.isl
	ReadIslFile(ISL_FILE);
	// Initilaize default value
	for(int i = 0; i < SAT_NUM; i++){
		// Initialize G.arcs
		for(int j = 0; j < SAT_NUM; j++){
			if(i == j){
				G.arcs[i][j].weight = 0;
				G.arcs[i][j].linkId = LINKID_SELFLOOP;
			}else{
				G.arcs[i][j].weight = MAX_ARC_WEIGHT;
				G.arcs[i][j].linkId = LINKID_NOLINK;
			}
		}
		// Initialize infData
		for(int j = 0; j < SINF_NUM; j++){
			InfData tmp = {-1,false};
			SAT(i).setInfData(tmp,j);
		}
	}
	// Set value
	for(int i = 0; i < SLINK_NUM; i++){
		Isl* isl = & G.isl[i];
		int v[2] = {isl->endpoint[0].nodeId-1, isl->endpoint[1].nodeId-1};
		// Set G.arcs
		for(int j = 0; j < 2; j++){
			G.arcs[ v[j] ][ v[(j+1)%2] ].linkId = isl->linkId;
			G.arcs[ v[j] ][ v[(j+1)%2] ].weight = isl->weight;
		}
		// Set infData
		for(int j = 0; j < 2; j++){
			int inf = isl->endpoint[j].inf;
			SAT(v[j]).acqInfData(inf).linkId = isl->linkId;
		}
	}
}

void topo_test()
{
}

