#ifndef OPSPF_H
#define OPSPF_H

#define OPSPF_EXATA_BUG 0 // bug in sending cbr

//-------------------------------------------
// default define
//-------------------------------------------
#define OPSPF_DEFAULT_RT_INTERVAL 1*SECOND
#define OPSPF_DEFAULT_SA_INTERVAL 1*SECOND
#define OPSPF_DEFAULT_PROCESS_BROADCAST TRUE
#define OPSPF_DEFAULT_BROADCAST_INTERVAL 1*SECOND
#define OPSPF_DEFAULT_IS_SATELLITE FALSE
#define OPSPF_DEFAULT_IS_USER FALSE
#define OPSPF_DEFAULT_ENABLE_STATICROUTE FALSE
#define OPSPF_DEFAULT_ENABLE_CHANGEADDRESS FALSE
#define OPSPF_DEFAULT_ENABLE_REGISTER FALSE

//-------------------------------------------
// define file path
//-------------------------------------------
#define OPSPF_MOBILITY_FILE "C:\\Scalable\\exata\\5.1\\scenarios\\user\\satellites\\mobilityFile\\orbit.bin"

//-----------------------------------------
// Define some constant value
//-----------------------------------------
#ifndef PI
#define PI 				3.1415926  //If PI is not defined before , define it
#endif //PI
#define N 				10000
#define DATA 			4147248    //The size of mobility data : 48*(24*60*60+1) 48 st 24 hours data per second
#define DATA_SIZE		86401	   //The size of mobility data : 24*60*60+1 24 hours data per second
#define OPSPFMAX 		0XFFFF		//To mean that the link is not available

const int TotalNode = 48; //The total number of ST nodes
const int NumOfLinks = 128; //The total number of links
const int MaxInterfaceNum = 6; //The total number of links
const double OpspfBeita= 70; //The latitude to switch , above which the st can't connect with the st in deffrent oribit
const double Gamma=4.0;

enum OpspfPacketType
{
	OPSPF_BROADCAST_PACKET = 1,
	OPSPF_REGISTER_PACKET = 2
};

//-----------------------------------------
// Define the structs
//-----------------------------------------
struct OpspfHeader {
	OpspfPacketType packetType;
	int satelliteId;
};

// struct to read st mobility information from file
// contain latitude and longitude of st
struct Coord {
	double x; //latitude of st in file
	double y; //longitude of st in file
};

// struct to save the real longitude and the longitude used to calculate angle phi
struct Lon{
	double reallon;	//the real longitude of st
	double calculatelon; //the longitude used to calculate angle phi
};

// struct to save st mobility information
// contain latitude , longitude and phi of st
struct STCoordinates{
	Lon lon;	//the longitude struct defined by struct Lon
	double lat;	//the latitude of st
	double phi;	//the latitude of st in the sloping oribit
};

// struct to save the mobility information of st in different oribit
struct Neighbor
{
	STCoordinates ST_leftneighbor; // the mobility information of st in left oribit
	STCoordinates ST_rightneighbor;// the mobility information of st in right oribit
};

// struct that contain all the information of one st , used by various fuctions
struct ST{
	int orbitnum; // the number of st's oribit
	int index;    // the index of st in this oribit
	STCoordinates STC;	//the mobility information of st
	Neighbor neighbor;	//the neighbor information of st
};

// the single link state information of the current scenario
// example : ST1(id) can connect to ST2's interface 0 via interface 1 , the Metric is 3
struct LSA
{
	int SourceSatellite_ID; // the source st id
	int SourceSatellite_interface; //the source interface
	int DestinationSatellite_ID; //the destination st id
	int DestinationSatellite_interface; //the destination interface
	int Metric; //the cost of this link
	LSA* NextLSA; // the next LSA in LSDB
};

// the vertex that contain the distance between another st and the current st
// calculate by Dijstra function
struct OpspfVertex
{
	NodeAddress vertexId; // the destination st id
	unsigned int distance; // the distance between the destination st and the current st
	OpspfVertex* NextVertex; //The next Vertex , to make a list

} ;

// the struct to represent the Shortest Path Tree
// example : 1->2->3 DestinationVertex=3 , AdvertisingVertext=2 , distance=2 ( source=1 )
struct OpspfShortestVertex
{
	NodeAddress     DestinationVertex; // the id of the destination st
	NodeAddress		AdvertisingVertext; // the id of the st before the destination st in tree
	unsigned int	distance; //the distance between the destination st and the source st
	OpspfShortestVertex*  NextShortestVertex; // the next ShortestVertex
};

// the link state database of the current st
// contain a lot of LSAs
struct LSDB
{
	LSA* LSAList; // the list head of LSAs
};

// used to convert from SatelliteIndex String ( example : "11" ) to number ( example : 1)
struct SatelitteNumMap
{
	string SatelliteIndex; // the SatelliteIndex String 
	int Num; // the number of st ( or id )
};

//-----------------------------------------
// when enable the register module
//-----------------------------------------

// The information in the register packet
struct RegisterPacket{
	bool isRelay; // wether the packet have relay or not
	int RelaySTId;// the id of the relay st
};

// The register row in the manager st
// example : { "190.0.21.3" , 1 , 18 }
// means : the user with ip "190.0.21.3" should have relay and the relay st is 18 ( id )
struct registerRow {
	NodeAddress usrip; // The ip address of the user
	bool isRelay; // To get to the user , should have relay st ?
	int RelaySTId; // The id of the relay st
	registerRow* next; // the pointer to the next register row
};

// The register list , contain the pointer to the first register row
// and the size of the list
struct registerList {
	registerRow* head; // The pointer to the first register row
	int size; // the size of thr list
};

// The register information in the UDP packets and the register msg
struct registerInfo {
	NodeAddress usrip; // The ip address of the user
	bool isRelay; // To get to the user , should have relay st ?
	int RelaySTId; // The id of the relay st
};

//-------------------------------------------
// main struct of OPSPFv2
//-------------------------------------------
typedef struct struct_network_opspf
{
// define the data to use in OPSPFv2

	// process control
	BOOL processBroadcast;
	BOOL isSatellite;
	BOOL isUser;
	BOOL enableStaticRoute;
	BOOL enableChangeAddress;
	BOOL enableRegister;

	// time control
	clocktype broadcastInterval;
	clocktype UpdateRTInterval; // the time interval to update routing table
	clocktype UpdateSAInterval; // the the interval to update the Satellite ID to Area mapping table and address
	clocktype LastNoRelay; // the last time that this st received a no relay register packet

	// the lists that the protocol keep
	ST st[TotalNode]; // contain the oribit information and position information of all the st
	ST oldst[TotalNode]; // contain the oribit information and position information of all the st in the previous time
	LSDB* lsdb; // the link state data base of the current st , contain a lot of LSAs , ( see more in Struct LSA )
	OpspfVertex GraphLinkList[TotalNode]; // contain the distance between sts ( see more in Struct OpspfVertex )
	OpspfShortestVertex* opspfShortestVertex; // the struct to save the shortest path tree ( see more in Struct OpspfShortestVertex )
	double s_a[TotalNode]; // the mapping table from Satellite Id to current Area
	registerList* rList; // the register list ( see more in Struct registerList )

	// The mapping tables used to get the source interface and destination interface
	// Due to the problem of the software , we can't self define the interfaces of node
	// So we need this mapping tables ( see more in documentation "Problem_Of_Link.pdf" )
	NodeAddress idtoaddress[TotalNode][MaxInterfaceNum]; // contain the interface ip address of st ( example : idtoaddress[2][3]= -1107295742
	                                // means : the ip address of ST2's interface 3 is "190.0.2.2"
	NodeAddress linktable[TotalNode][MaxInterfaceNum]; // contain the destination st id linked to current st's interface

	// used in FillOpspfRoutingTable()
	NodeAddress linknode[NumOfLinks]; // contain the first node of the links
	NodeAddress linkid[NumOfLinks]; // contain the subnet ip address of the links
	
	// the flags
	bool isfirst; // to tell wether it is the first time to run this update code

	//lxc added, to fif fxf, used for user
	int selfSatelliteId;

} OpspfData;

void OpspfInit(
	Node* node,
	OpspfData** opspfPtr,
	const NodeInput* nodeInput,
	int interfaceIndex,
	NetworkRoutingProtocolType opspfProtocolType);

void OpspfHandleRoutingProtocolEvent(
	Node* node,
	Message* msg);

void OpspfFinalize(
	Node* node,
	int interfaceIndex);

static void OpspfInitializeConfigurableParameters(
	Node* node,
	const NodeInput* nodeInput,
	OpspfData* opspf,
	int interfaceIndex);

static void OpspfSetTimer(
	Node* node,
	int eventType,
	clocktype delay);

void OpspfRouterFunction(
	Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted);

void OpspfHandleProtocolPacket(
	Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned ttl,
    int interfaceIndex);

// APIs to use static route protocol

void OpspfInitSTScenario(
	OpspfData* opspf);

void UpdateOpspfRoutingTable(
	Node* node,
	Message* msg);

void UpdateSATable(
	Node* node,
	Message* msg);

ST GetST(
	Node* node);

void GetSTLatByFile(
	Node* node);

void freeLSAList(
	LSA* LSAListhead);

LSA* OpspfLookupLSAList(
       LSDB* lsdb, 
       int SatelliteIndex);

void ReadLSAToGraph(
	LSDB* lsdb, 
	OpspfVertex GraphLinkList[TotalNode]);

void FreeList(
	OpspfShortestVertex** List);

OpspfShortestVertex* SearchVertex(
	OpspfShortestVertex* Path,
	NodeAddress VertexId);

void Dijstra(
	OpspfVertex GraphLinkList[TotalNode], 
	NodeAddress Start,
	OpspfShortestVertex** Path);

void UpdateLSDB(
	Node* node);

void FindShortestPath(
	Node* node);

void VertexHopForRouting(
	NodeAddress AdvertisingVertex,
	NodeAddress DestinationVertex,
	OpspfShortestVertex* Path,
	OpspfShortestVertex** Router);

void FillOpspfRoutingTable(
	Node* node);

void OpspfAddRoutingTableRowById(
	Node* node, 
	NodeAddress nodeId,
	NodeAddress dstAddr);

// APIs to change SA mapping table and address

int GetAddressID(
	double lon,
	double lat);

double Dec(
	double lat);

void FixLon(
	int t,
	Coord sat[]);

double radian(
	double d);

void Fin(
	int t,
	Coord mat[48]);

void ChangeAdderss(
	Node* node,
	Message* msg);

void ChangeUserAdderss(
	Node* node);

// APIs to handle area change

void SendRigesterPacket(
	Node* node,
	Message* msg);

void SendBroadcastPacket(
	Node* node,
	Message* msg);

void InitRegisterList(
	Node* node);

void AddRegisterListRow(
	Node* node,
	NodeAddress usrip,
	bool isRelay,
	int RelaySTId);

registerRow* FindRegisterRowByUsrIp(
	Node* node,
	NodeAddress usrip);

void FreeRegisterList(
	Node* node);

void OpspfAddReigsterRow(
	Node* node,
	registerInfo* addInfo);

//lxc added
//The opposite orbit satellite id,used for user to choose whether to send register packet 
//after received a broadcast pkt.
const int oppSateId[2][8] = {
	{1,2,3,4,5,6,7,8},
	{41,42,43,44,45,46,47,48},
};

#define NORMAL_ORBIT -1
#define RIGHT_ORBIT 0
#define LEFT_ORBIT 1

bool isOppositeSatellite(
	NodeAddress selfAddress,
	NodeAddress sourceAddress,
	int srcSatelliteId,
	OpspfData* opspfData);

#endif // OPSPF_H