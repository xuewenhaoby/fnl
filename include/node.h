#ifndef __NODE_H__
#define __NODE_H__

#include <iostream>
#include <cstdlib>
#include <vector>
#include "topo.h"
using namespace std;

class Node{
public:
	Node(){}
	~Node(){}

	int getId(){return id;}
	Position getPos(){return pos;}

	void setId(int _id){id = _id;}
	void setPos(NodePos _pos){pos = _pos;}

	virtual void nodeInitialize() = 0;
	virtual void nodeFinalize() = 0;
	virtual void updatePos() = 0;
	virtual void updateLink() = 0;

protected:
	int id;
	NodePos pos;
	string name;
	bool real;
};

class SatNode:public Node{
public:
	SatNode();

	void nodeInitialize();
	void nodeFinalize();
	void updateLink(){};

	void updatePos();
	void setInfData(InfData data,int i){infData[i] = data;}
	InfData & acqInfData(int i){return infData[i];}
    bool getInfStat(int infId);
private:
	string br_name;
	InfData infData[6];
};

class HostNode:public Node{
public:
	HostNode();
	
	void nodeInitialize();
	void nodeFinalize();
	void updatePos();
	void updateLink();

	void iperf_server();
	void iperf_client();
protected:
	void changeSatellite(int oldSat, int newSat);
	void sendRegisterPacket();
	int getAreaId(){return areaId;} 
	int getAddr(){return addr;}
private:
	int areaId;
	int addr;
	LinkState link_stat[SAT_NUM];
	vector<LinkState> link_use;
};

bool Compare(LinkState a, LinkState b);

#endif