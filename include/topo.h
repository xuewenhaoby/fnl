#ifndef __TOPO_H__
#define __TOPO_H__

#include <iostream>
using namespace std;

//#define HOST_NUM 6

#define TOPO_M 6 // number of orbit
#define TOPO_N 8 // number of satellite on one orbit
#define SAT_NUM TOPO_M * TOPO_N
#define SLINK_NUM 128
#define SINF_NUM 6
#define MAX_ARC_WEIGHT 1000

#define TOPO_BETA 70

#define LINKID_SELFLOOP 0
#define LINKID_NOLINK -1

#define SAT_NAME "sat"
#define HOST_NAME "host"
#define SAT_BR_NAME "sbr"
#define HOST_BR_NAME "hbr"

typedef int NodeAddress;

struct Coord
{
	double lat;
	double lon;
};

typedef struct Position
{
	Coord loc;
	bool isNorth;
}NodePos;

struct IslNode
{
	int nodeId;
	int inf;
	NodeAddress ip;
};

struct Isl
{
	int linkId;
	NodeAddress subnetIp;
	unsigned int weight;

	IslNode endpoint[2];
};

struct ArcNode
{
	int linkId;
	unsigned int weight;
};

struct StaticTopo
{
	Isl isl[SLINK_NUM];
	ArcNode arcs[SAT_NUM][SAT_NUM]; // weight;
};

struct LinkState
{
	int satId;
	double distance;
	bool isCovered;
	// bool operator==(const LinkState &b){
	// 	return this->sateId == b.sateId 
	// 	&& this->isCovered == b.isCovered;
	// 
};

typedef struct InterfaceData
{
	int linkId;
	bool stat;
}InfData;

void TopoInitialize(int* host_num);

void TopoFinalize(int* host_num);

void topo_test();

void InitStaticTopo();

#endif