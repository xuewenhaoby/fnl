#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <cmath>
#include <windows.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "network.h"

#include "opspfv2.h"

#define OPSPF_DEBUG_INIT 0
#define OPSPF_DEBUG_FINALIZE 0
#define OPSPF_DEBUG_CONFIGURE 0
#define STD_TO_FILE 0

void OpspfInit(
	Node* node,
	OpspfData** opspfPtr,
	const NodeInput* nodeInput,
	int interfaceIndex,
	NetworkRoutingProtocolType opspfProtocolType)
{
	// debug
	if(OPSPF_DEBUG_INIT)
	{
		printf("Node %d interface %d init OPSPF!\n",
			node->nodeId,
			interfaceIndex);
	}

	// out to file
#ifdef STD_TO_FILE
	if( freopen( "C:\\Users\\fnl\\Desktop\\cyj.txt" , "a+" , stdout) == NULL ){
		ERROR_ReportError("freopen() failed!\n");
	}
#endif //STD_TO_FILE

	// create the opspf struct to use
	NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
	OpspfData* opspf = (OpspfData *) MEM_malloc(sizeof(OpspfData));
	(*opspfPtr) = opspf;

	// read from configure file
	OpspfInitializeConfigurableParameters(
		node,
		nodeInput,
		opspf,
		interfaceIndex);

	// Initialize OPSPFv2 routing table
	//NetworkEmptyForwardingTable(node,opspfProtocolType);

	// Initialize static parameters
	opspf->LastNoRelay=0*SECOND;
	opspf->opspfShortestVertex=NULL; // initialize the shortest path tree
	opspf->lsdb=NULL; // initialize the link state database
	opspf->isfirst=1; // define that this is the first time the protocol run
	memset(opspf->st, 0, sizeof (ST)*TotalNode);// initialize the struct ST
	memset(opspf->oldst, 0, sizeof (ST)*TotalNode);
	for(int i=0;i<TotalNode;i++)
	{
		opspf->GraphLinkList[i].distance=0;
		opspf->GraphLinkList[i].NextVertex=NULL;
		opspf->GraphLinkList[i].vertexId=0;
	}

	//lxc added, to fix fxf 
	opspf->selfSatelliteId = 0;

	if(OPSPF_DEBUG_CONFIGURE)
	{
		printf("==============================================\n");
		printf("Node %u configure informations:\n",node->nodeId);
		printf("==============================================\n");
		printf("Node %u opspf->UpdateRTInterval = %u\n",node->nodeId,(unsigned int)opspf->UpdateRTInterval);
		printf("Node %u opspf->UpdateSAInterval = %u\n",node->nodeId,(unsigned int)opspf->UpdateSAInterval);
		printf("Node %u opspf->broadcastInterval = %u\n",node->nodeId,(unsigned int)opspf->broadcastInterval);
		printf("Node %u opspf->processBroadcast = %u\n",node->nodeId,(unsigned int)opspf->processBroadcast);
		printf("Node %u opspf->isSatellite = %u\n",node->nodeId,(unsigned int)opspf->isSatellite);
		printf("Node %u opspf->isUser = %u\n",node->nodeId,(unsigned int)opspf->isUser);
		printf("Node %u opspf->enableStaticRoute = %u\n",node->nodeId,(unsigned int)opspf->enableStaticRoute);
		printf("Node %u opspf->enableChangeAddress = %u\n",node->nodeId,(unsigned int)opspf->enableChangeAddress);
		printf("Node %u opspf->enableRegister = %u\n",node->nodeId,(unsigned int)opspf->enableRegister);
	}

	// Set the router function
	NetworkIpSetRouterFunction(
		node,
		&OpspfRouterFunction,
		interfaceIndex);

	// set timer to process event
	if (opspf->processBroadcast)
	{
		OpspfSetTimer(node, MSG_OPSPF_SendBroadcast, opspf->broadcastInterval);
	}

	if(opspf->isSatellite)
	{
		if(opspf->enableChangeAddress)
		{
			// start change satellite address
			OpspfSetTimer(node, MSG_OPSPF_UpdateSATable, opspf->UpdateSAInterval);
		}
		if(opspf->enableStaticRoute)
		{
			// read configure information when use satellite scenario
			OpspfInitSTScenario(opspf);
			OpspfSetTimer(node, MSG_OPSPF_UpdateStaticRoute, opspf->UpdateRTInterval);
		}
		if(opspf->enableRegister)
		{
			InitRegisterList(node);
		}
	}

	if (opspf->isUser)
	{
		if(opspf->enableChangeAddress)
		{
			// change user address
			//OpspfSetTimer(node, MSG_OPSPF_UpdateUserAddress, 10*MILLI_SECOND);
			for(int i=0;i<48;i++)
			{
				NodeAddress dstAddr = (i+1)<<24;
				NodeAddress NetMask = 255<<24;
				NetworkUpdateForwardingTable(node,dstAddr,NetMask,-1,0,0,ROUTING_PROTOCOL_OPSPF);
				//NetworkUpdateForwardingTable(node,dstAddr,NetMask,-1,0,0,ROUTING_PROTOCOL_OPSPF);
			}
		}
	}

}

void OpspfHandleRoutingProtocolEvent(
	Node* node,
	Message* msg)
{
	OpspfData* opspf = (OpspfData *) NetworkIpGetRoutingProtocol(
                                node,
                                ROUTING_PROTOCOL_OPSPF,
                                NETWORK_IPV4);
	int i;

	switch (MESSAGE_GetEvent(msg))
    {
    	case MSG_OPSPF_SendBroadcast:
        {
            // send broadcast packet
            if(opspf->isSatellite == TRUE){
            	i = node->numberInterfaces - 1;
            	Message* newMsg = MESSAGE_Duplicate(node, msg);
            	MESSAGE_PacketAlloc(node, newMsg, 512, TRACE_OPSPF);
            	OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(newMsg);
            	OpspfHdr->packetType = OPSPF_BROADCAST_PACKET;
            	OpspfHdr->satelliteId = node->nodeId;
            	NodeAddress sourceAddress = NetworkIpGetInterfaceAddress(node, i);
            	NetworkIpSendRawMessage(node, newMsg, sourceAddress, ANY_ADDRESS, i, 0, IPPROTO_OPSPF, 64);
            }
            else{
            	for (i = 0; i < node->numberInterfaces; i++){
	            	Message* newMsg = MESSAGE_Duplicate(node, msg);
	            	MESSAGE_PacketAlloc(node, newMsg, 512, TRACE_OPSPF);
	            	OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(newMsg);
            		OpspfHdr->packetType = OPSPF_BROADCAST_PACKET;
	            	NodeAddress sourceAddress = NetworkIpGetInterfaceAddress(node, i);
	            	NetworkIpSendRawMessage(node, newMsg, sourceAddress, ANY_ADDRESS, i, 0, IPPROTO_OPSPF, 64);
	            }
            }
            
            // set the timer again
            MESSAGE_Send(node, msg, opspf->broadcastInterval);

            break;
        }

        case MSG_OPSPF_UpdateStaticRoute:
        {
        	//printf("Node %u update static route table!\n",node->nodeId);
        	UpdateOpspfRoutingTable(node,msg);
        	
        	// set the timer again
            MESSAGE_Send(node, msg, opspf->UpdateRTInterval);

            break;
        }
        
        case MSG_OPSPF_UpdateSATable:
        {
        	//printf("Node %u change address!\n",node->nodeId);
        	UpdateSATable(node,msg);
        	
        	// set the timer again
            MESSAGE_Send(node, msg, opspf->UpdateSAInterval);

            break;
        }

        case MSG_OPSPF_UpdateUserAddress:
        {
        	printf("user %u change address!\n",node->nodeId);
        	ChangeUserAdderss(node);
        	
        	// free the message
            MESSAGE_Free(node, msg);

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Opspf: Unknown MSG type!\n");
            break;
        }
    }

}

void OpspfFinalize(
	Node* node,
	int interfaceIndex)
{
	if(OPSPF_DEBUG_FINALIZE){
		printf("Node %d interface %d finished OPSPF!\n",
			node->nodeId,
			interfaceIndex);
	}
	static bool flag = false;
	if(!flag){
		flag = true;
	}

#ifdef STD_TO_FILE
	fclose(stdout);
#endif //STD_TO_FILE

}

static void OpspfInitializeConfigurableParameters(
	Node* node,
	const NodeInput* nodeInput,
	OpspfData* opspf,
	int interfaceIndex)
{
	BOOL wasFound;
	char buf[MAX_STRING_LENGTH];
	UInt32 nodeId = node->nodeId;

	IO_ReadTime(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-RT-INTERVAL",
		&wasFound,
		&opspf->UpdateRTInterval);

	if (!wasFound)
	{
		opspf->UpdateRTInterval = OPSPF_DEFAULT_RT_INTERVAL;
	}
	else
	{
		ERROR_Assert(
			opspf->UpdateRTInterval > 0,
			"Invalid OPSPF-RT-INTERVAL configuration");
	}

	IO_ReadTime(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-SA-INTERVAL",
		&wasFound,
		&opspf->UpdateSAInterval);

	if (!wasFound)
	{
		opspf->UpdateSAInterval = OPSPF_DEFAULT_SA_INTERVAL;
	}
	else
	{
		ERROR_Assert(
			opspf->UpdateSAInterval > 0,
			"Invalid OPSPF-SA-INTERVAL configuration");
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-PROCESS-BROADCAST",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->processBroadcast = TRUE;            
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->processBroadcast = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-PROCESS-BROADCAST should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->processBroadcast = OPSPF_DEFAULT_PROCESS_BROADCAST;
	}

	// read broadcast interval
    IO_ReadTime(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-BROADCAST-INTERVAL",
		&wasFound,
		&opspf->broadcastInterval);

	if (!wasFound)
	{
		opspf->broadcastInterval = OPSPF_DEFAULT_BROADCAST_INTERVAL;
	}
	else
	{
		ERROR_Assert(
			opspf->broadcastInterval > 0,
			"Invalid OPSPF-BROADCAST-INTERVAL configuration");
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-IS-SATELLITE",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->isSatellite = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->isSatellite = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-IS-SATELLITE should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->isSatellite = OPSPF_DEFAULT_IS_SATELLITE;
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-IS-USER",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->isUser = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->isUser = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-IS-USER should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->isUser = OPSPF_DEFAULT_IS_USER;
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-ENABLE-STATICROUTE",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->enableStaticRoute = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->enableStaticRoute = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-ENABLE-STATICROUTE should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->enableStaticRoute = OPSPF_DEFAULT_ENABLE_STATICROUTE;
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-ENABLE-CHANGEADDRESS",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->enableChangeAddress = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->enableChangeAddress = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-ENABLE-CHANGEADDRESS should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->enableChangeAddress = OPSPF_DEFAULT_ENABLE_CHANGEADDRESS;
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"OPSPF-ENABLE-REGISTER",
		&wasFound,
		buf);

	if (wasFound)
	{
		if (strcmp(buf, "YES") == 0)
        {
            opspf->enableRegister = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            opspf->enableRegister = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "OPSPF-ENABLE-REGISTER should be either \"YES\" or \"NO\".\n");
        }
	}
	else{
		opspf->enableRegister = OPSPF_DEFAULT_ENABLE_REGISTER;
	}

	// IO_ReadInt ...
}

static void OpspfSetTimer(
	Node* node,
	int eventType,
	clocktype delay)
{
	Message *newMsg = NULL;
	newMsg = MESSAGE_Alloc(
		node,
		NETWORK_LAYER,
		ROUTING_PROTOCOL_OPSPF,
		eventType);
	MESSAGE_Send(node, newMsg, delay);
}

void OpspfRouterFunction(
	Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted)
{
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	char buf[MAX_STRING_LENGTH];
	char sourceStr[MAX_STRING_LENGTH];
	char destinationStr[MAX_STRING_LENGTH];
	char interfaceStr[MAX_STRING_LENGTH];
    IpHeaderType* ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);

    IO_ConvertIpAddressToString(
    	ipHeader->ip_src,
    	sourceStr);
    IO_ConvertIpAddressToString(
    	ipHeader->ip_dst,
    	destinationStr);
    NetworkIpGetInterfaceAddressString(
    	node,
    	node->numberInterfaces-1,
    	interfaceStr);

    if( (opspf->isSatellite) && (ipHeader->ip_p != IPPROTO_OPSPF)  && (ipHeader->ip_p != IPPROTO_ICMP) )
    {
    	NodeAddress selfAddr = NetworkIpGetInterfaceAddress(node,node->numberInterfaces-1);
    	printf(" Satellite %u (%s) get a (%u) packet : src =%s ,dst = %s, id = %d , ttl = %d\n",
    		node->nodeId,
    		interfaceStr,
    		ipHeader->ip_p,
    		sourceStr,
    		destinationStr,
    		ipHeader->ip_id,
    		ipHeader->ip_ttl);
        RegisterPacket* rpkt = (RegisterPacket*)MEM_malloc(sizeof(RegisterPacket));
        unsigned char *tos = (unsigned char *)&ipHeader->ip_v_hl_tos_len + 2;
        rpkt->isRelay = *tos>>7;
        rpkt->RelaySTId = (*tos & 0x7f) >> 1;
        printf(" ---> pkt.isRelay = %d , pkt.RelaySTId = %d\n",
        	rpkt->isRelay,
        	rpkt->RelaySTId);

        NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OPSPF_TEMP);
		if( (ipHeader->ip_dst>>24) == (selfAddr>>24) )
		{// this satellite is the gateway satellite of the destination user
			// find the register row of the destination user
			printf( " ---> satellite RegisterList :\n");
			if (opspf->rList != NULL){
				registerRow* temp = opspf->rList->head;				
				while (temp != NULL){
					IO_ConvertIpAddressToString(temp->usrip,buf);
					printf(" ---> usrip: %s , isRelay: %d , RelaySTId: %d\n", buf , temp->isRelay , temp->RelaySTId);
					temp = temp->next;
				}
			}
			registerRow* rRow = FindRegisterRowByUsrIp( node , ipHeader->ip_dst );
			if( rRow != NULL )
			{
				// set the packet's TOS using the relay information in the register row
                *tos = ((int)rRow->isRelay<<7) + (rRow->RelaySTId<<1);
                rpkt->isRelay = rRow->isRelay;
                rpkt->RelaySTId = rRow->RelaySTId;
				if(rpkt->isRelay == 1)
				{// When this packet should be relayed , add a temp route to the relay satellite
					OpspfAddRoutingTableRowById( node , rpkt->RelaySTId , ipHeader->ip_dst );
				}
			}
		}
		else
		{// this satellite is the router satellite
			if( rpkt->isRelay == 1)
			{// this packet have reached the gateway satellite , and it should be relayed
				OpspfAddRoutingTableRowById(node, rpkt->RelaySTId, ipHeader->ip_dst);
			}
		}
        MEM_free(rpkt);
    }

    if( (opspf->isUser) && (ipHeader->ip_p != IPPROTO_OPSPF)  && (ipHeader->ip_p != IPPROTO_ICMP) )
    {
    	if( NetworkIpIsMyIP(node,ipHeader->ip_src) )
    	{
    		if( !NetworkIpIsMyIP(node,ipHeader->ip_dst) )
    		{
	    		printf("---------------------------------------------------------------------------\n");
	    		printf("User %u (%s) send a packet : src= %s, dst= %s , id = %d ,ttl = %d >>>>>>>>>>\n",
	    			node->nodeId,
	    			interfaceStr,
	    			sourceStr,
	    			destinationStr,
	    			ipHeader->ip_id,
	    			ipHeader->ip_ttl);
	    		/*
	    		ofstream out1;
				out1.open("C:\\Users\\cyj\\Desktop\\send.txt",ios::app);
				if(!out1)
				{
					cout<<"open error in out file!\n";
					exit(1);
				}
	    		out1<<ipHeader->ip_id<<endl;
	    		out1.close();
	    		*/
    		}    		
    	}
    	else if( NetworkIpIsMyIP(node,ipHeader->ip_dst) )
    	{
    		if( !NetworkIpIsMyIP(node,ipHeader->ip_src) )
    		{
    			printf("User %u (%s) receive a packet! src= %s, dst= %s , id = %d , ttl = %d <<<<<<<<<<\n",
	    			node->nodeId,
	    			interfaceStr,
	    			sourceStr,
	    			destinationStr,
	    			ipHeader->ip_id,
	    			ipHeader->ip_ttl);
    			/*
    			ofstream out2;
    			out2.open("C:\\Users\\cyj\\Desktop\\receive.txt",ios::app);
    			if(!out2)
				{
					cout<<"open error in out file!\n";
					exit(1);
				}
    			out2<<ipHeader->ip_id<<endl;
    			out2.close();
    			*/
    		}    		
    	}
    }

    *packetWasRouted = FALSE;
}

void OpspfHandleProtocolPacket(
	Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned ttl,
    int interfaceIndex)
{
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	char sourceStr[MAX_STRING_LENGTH];
	char destinationStr[MAX_STRING_LENGTH];
	char selfAddressStr[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(
    	sourceAddress,
    	sourceStr);
    IO_ConvertIpAddressToString(
    	destinationAddress,
    	destinationStr);
    NetworkIpGetInterfaceAddressString(
    	node,
    	node->numberInterfaces-1,
    	selfAddressStr);

    //---------------------------------------
    // User protocol packet handler
    //---------------------------------------
    if( opspf->isUser )
    {
    	if(opspf->enableChangeAddress)
    	{
    		/*
	    	NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OPSPF);
			for(int i=0;i<48;i++)
			{
				NodeAddress dstAddr = (i+1)<<24;
				NodeAddress NetMask = 255<<24;
				NetworkUpdateForwardingTable(node,dstAddr,NetMask,sourceAddress,0,0,ROUTING_PROTOCOL_OPSPF);
				//NetworkUpdateForwardingTable(node,dstAddr,NetMask,-1,0,0,ROUTING_PROTOCOL_OPSPF);
			}
			*/
    	}
    	if(opspf->enableRegister)
    	{
    		printf("// user %u get a broadcast packet from %s //\n", node->nodeId , sourceStr);
    		// get user address
    		NodeAddress selfAddr = NetworkIpGetInterfaceAddress(node,node->numberInterfaces-1);
			// find the packet type
			OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(msg);
			// when it is a broadcast packet
			if( (OpspfHdr->packetType == OPSPF_BROADCAST_PACKET)
				//lxc added, to fix fxf
				&& (!isOppositeSatellite(
							selfAddr,
							sourceAddress,
							OpspfHdr->satelliteId,
							opspf))
				//end lxc
			  )
			{
				Message* newMsg = MESSAGE_Alloc(
					node,
					NETWORK_LAYER,
					ROUTING_PROTOCOL_OPSPF,
					MSG_OPSPF_SendBroadcast);
				MESSAGE_PacketAlloc(node, newMsg, 512, TRACE_OPSPF);
				OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(newMsg);
            	OpspfHdr->packetType = OPSPF_REGISTER_PACKET;
            	registerInfo* rpacket = (registerInfo*) ( MESSAGE_ReturnPacket(newMsg) + sizeof(OpspfHeader) );
            	if( (sourceAddress>>24) == (selfAddr>>24) )
            	{
            		rpacket->isRelay=0;
					rpacket->RelaySTId=0;
            	}
            	else
            	{
            		rpacket->isRelay=1;
            		rpacket->RelaySTId=0;
            	}
            	rpacket->usrip = selfAddr;
            	NodeAddress destAddr = sourceAddress;
            	int inf = node->numberInterfaces - 1;
            	NetworkIpSendRawMessage(node, newMsg, selfAddr, destAddr, inf, 0, IPPROTO_OPSPF, 64);
			}
    	}
    }

    //---------------------------------------
    // Satellite protocol packet handler
    //---------------------------------------
    if( opspf->isSatellite )
    {
    	if(opspf->enableRegister)
    	{
    		// get satellite address
    		NodeAddress selfAddr = NetworkIpGetInterfaceAddress(node,node->numberInterfaces-1);
			// find the packet type
			OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(msg);
			// when it is a register packet
			if(OpspfHdr->packetType == OPSPF_REGISTER_PACKET)
			{
				registerInfo* rpacket = (registerInfo*) ( MESSAGE_ReturnPacket(msg) + sizeof(OpspfHeader) );
				//printf("// satellite %u get a rigister packet from %s //\n", node->nodeId , sourceStr );
				//printf("// isRelay= %d , RelaySTId= %d , self_ip = %s\n", rpacket->isRelay, rpacket->RelaySTId , selfAddressStr);

				// When the satellite is the normal satellite
				if((rpacket->isRelay==1)&&(rpacket->RelaySTId==0))
				{// When this register packet is relay packet , add the first satellite as relay satellite	
					Message* newMsg = MESSAGE_Alloc(
						node,
						NETWORK_LAYER,
						ROUTING_PROTOCOL_OPSPF,
						MSG_OPSPF_SendBroadcast);
					MESSAGE_PacketAlloc(node, newMsg, 512, TRACE_OPSPF);
					OpspfHeader*  OpspfHdr = (OpspfHeader*) MESSAGE_ReturnPacket(newMsg);
	            	OpspfHdr->packetType = OPSPF_REGISTER_PACKET;
	            	registerInfo* newRpacket = (registerInfo*) ( MESSAGE_ReturnPacket(newMsg) + sizeof(OpspfHeader) );
	            	newRpacket->isRelay = 1;
	            	newRpacket->RelaySTId = node->nodeId;
	            	newRpacket->usrip = rpacket->usrip;
	            	NodeAddress destAddr = (sourceAddress & 0xff000000) + 1;
	            	NetworkIpSendRawMessage(node, newMsg, ANY_IP, destAddr, ANY_INTERFACE, 0, IPPROTO_OPSPF, 64);
				}
				else
				{// When the satellite is the gateway satellite of the user , add a register row to the register list
					//printf("// gateway satellite get register packet!\n");
					registerInfo *rInfo = (registerInfo *)MEM_malloc(sizeof(registerInfo));
					rInfo->usrip = rpacket->usrip;
					rInfo->isRelay = rpacket->isRelay;
					rInfo->RelaySTId = rpacket->RelaySTId;
					OpspfAddReigsterRow(node,rInfo);
					MEM_free(rInfo);
				}
			} // end if(OpspfHdr->packetType == OPSPF_REGISTER_PACKET)...
    	} // end if(opspf->enableRegister)...
    } // end if( opspf->isSatellite )...
}

//------------------------------------------------------------
// api for satellite scenarios
//------------------------------------------------------------

void UpdateOpspfRoutingTable(Node* node, Message* msg){

	// Satellite calculate all the latitude of satellites
	GetSTLatByFile(node);
	
	// Update Link state database	
	UpdateLSDB(node);

	// calculatelon the shortest path tree
	FindShortestPath(node);

	// fill the routing table
	FillOpspfRoutingTable(node);
}

void GetSTLatByFile(Node* node){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);

	// configure orbit information
	const int orbital=6;
	const int maxST=8;

	// init ST struct
	for(int i=0;i<orbital*maxST;i++){
		opspf->st[i].orbitnum=i/maxST+1;		
		opspf->st[i].index=(i+1)%maxST;
		if(opspf->st[i].index==0) opspf->st[i].index=8;
		opspf->st[i].STC.phi=0;
	}
	
	int start_time=0;
	ifstream infile;
	infile.open( OPSPF_MOBILITY_FILE ,ios::in|ios::binary);
	if(!infile)
	{
		cerr<<"OPSPF_MOBILITY_FILE open error!"<<endl;
		exit(1);
	}

	int currentId = node->nodeId-1;
	int currentTime = int(node->getNodeTime()/SECOND);

	Coord* TempST=NULL;
	TempST=(Coord*)MEM_malloc(sizeof(Coord));
	for(int i=0;i<TotalNode;i++){
		start_time = i * DATA_SIZE + currentTime % DATA_SIZE;
		infile.seekg(sizeof(Coord) * start_time,ios_base::beg);
		infile.read((char*)(TempST), sizeof(Coord));
		opspf->st[i].STC.lat = TempST->x;
	}
	free(TempST);
	infile.close();
}

void UpdateLSDB(Node* node)
{
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	LSA* head;
	LSA* back;

	if(opspf->lsdb != NULL){
		if(opspf->lsdb->LSAList != NULL)
		{
			freeLSAList(opspf->lsdb->LSAList);
		}
	}

	opspf->lsdb = (LSDB*)malloc(sizeof(LSDB));	
	head = (LSA*)malloc(sizeof(LSA));
	opspf->lsdb->LSAList = head;

	int j=0;
	for (int i = 0; i < 48; i++)
	{
		back = head;
		back->DestinationSatellite_ID = (opspf->st[i].orbitnum-1)*8 + (opspf->st[i].index)%8;
		back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index-1;
		for(j=0;j<6;j++){
			if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
				break;
				}
			}
		back->DestinationSatellite_interface = j;
		for(j=0;j<6;j++){
			if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
				break;
				}
			}
		back->SourceSatellite_interface = j;
		back->Metric = 1;
		back->NextLSA = NULL;

		head = (LSA*)malloc(sizeof(LSA));
		back->NextLSA = head;
		back = head;
		back->DestinationSatellite_ID = (opspf->st[i].orbitnum-1) * 8 + (opspf->st[i].index +6)%8;
		back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
		for(j=0;j<6;j++){
			if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
				break;
				}
			}
		back->DestinationSatellite_interface = j;
		for(j=0;j<6;j++){
			if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
				break;
				}
			}
		back->SourceSatellite_interface = j;
		back->Metric = 1;
		head= (LSA*)malloc(sizeof(LSA));
		back->NextLSA = head;
	}
	free(head);

	for (int i = 0; i < 48; i++)
	{
		if ((abs(opspf->st[i].STC.lat) > OpspfBeita) && (abs(opspf->st[i].STC.lat) <= 90))
		{
			head = (LSA*)malloc(sizeof(LSA));
			back->NextLSA = head;
			back = head;
			back->DestinationSatellite_ID = 0;
			back->DestinationSatellite_interface = 3;
			back->SourceSatellite_ID = ((opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1);
			back->SourceSatellite_interface = 1;
			back->Metric = OPSPFMAX;
			back->NextLSA = NULL;

			head = (LSA*)malloc(sizeof(LSA));
			back->NextLSA = head;
			back = head;
			back->DestinationSatellite_ID = 0;
			back->DestinationSatellite_interface = 1;
			back->SourceSatellite_ID = ((opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1);
			back->SourceSatellite_interface = 3;
			back->Metric = OPSPFMAX;
			back->NextLSA = NULL;
		}
		else
		{
			if (opspf->st[i].orbitnum == 1)
			{
				if((((opspf->st[i].STC.lat)- (opspf->oldst[i].STC.lat))>=0)||(opspf->isfirst==1) )
				{
					opspf->isfirst=0;
					head = (LSA*)malloc(sizeof(LSA));
					back->NextLSA = head;
					back = head;
					back->DestinationSatellite_ID = 0;
					back->DestinationSatellite_interface = 1;
					back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
					back->SourceSatellite_interface = 3;
					back->Metric = OPSPFMAX;
					back->NextLSA = NULL;

					if ((abs(opspf->st[ (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8].STC.lat) > OpspfBeita) && (abs(opspf->st[ (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8].STC.lat)  <= 90))
					{
						head = (LSA*)malloc(sizeof(LSA));
						back->NextLSA = head;
						back = head;
						back->DestinationSatellite_ID = 0;
						back->DestinationSatellite_interface = 3;
						back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8+ opspf->st[i].index-1;
						back->SourceSatellite_interface = 1;
						back->Metric = OPSPFMAX;
						back->NextLSA = NULL;
					}
					else
					{
						head = (LSA*)malloc(sizeof(LSA));
						back->NextLSA = head;
						back = head;
						back->DestinationSatellite_ID = (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8;
						back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
						for(j=0;j<6;j++){
							if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
							break;
							}
						}
						back->DestinationSatellite_interface = j;
						for(j=0;j<6;j++){
							if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
							break;
							}
						}
						back->SourceSatellite_interface = j;
						back->Metric = 1;
						back->NextLSA = NULL;
					}
				}
				else
				{
					head = (LSA*)malloc(sizeof(LSA));
					back->NextLSA = head;
					back = head;
					back->DestinationSatellite_ID =0;
					back->DestinationSatellite_interface = 1;
					back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
					back->SourceSatellite_interface = 3;
					back->Metric = OPSPFMAX;
					back->NextLSA = NULL;
					if ((abs(opspf->st[i+8].STC.lat) > OpspfBeita) && (abs(opspf->st[i+8].STC.lat) <= 90))
					{
						head = (LSA*)malloc(sizeof(LSA));
						back->NextLSA = head;
						back = head;
						back->DestinationSatellite_ID = 0;
						back->DestinationSatellite_interface = 3;
						back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
						back->SourceSatellite_interface = 1;
						back->Metric = OPSPFMAX;
						back->NextLSA = NULL;
					}
					else
					{
						head = (LSA*)malloc(sizeof(LSA));
						back->NextLSA = head;
						back = head;
						back->DestinationSatellite_ID = opspf->st[i].orbitnum * 8 + opspf->st[i].index -1;
						back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
						for(j=0;j<6;j++){
							if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
							break;
							}
						}
						back->DestinationSatellite_interface = j;
						for(j=0;j<6;j++){
							if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
							break;
							}
						}
						back->SourceSatellite_interface = j;
						back->Metric = 1;
						back->NextLSA = NULL;
					}
				}
			}
			else if ((opspf->st[i].orbitnum > 1) && (opspf->st[i].orbitnum < 6))
			{
				if ((((opspf->st[i].STC.lat)- (opspf->oldst[i].STC.lat))>=0)||(opspf->isfirst==1))
					{
						opspf->isfirst=0;
						if ((abs(opspf->st[ (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8].STC.lat) > OpspfBeita) && (abs(opspf->st[ (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8].STC.lat) <= 90))
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = 0;
							back->DestinationSatellite_interface = 3;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							back->SourceSatellite_interface = 1;
							back->Metric = OPSPFMAX;
							back->NextLSA = NULL;
						}
						else
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = (opspf->st[i].orbitnum)*8+ (opspf->st[i].index+6)%8;
							back->SourceSatellite_ID =(opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
								break;
								}
							}
							back->DestinationSatellite_interface = j;
							for(j=0;j<6;j++){
							if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
								break;
								}
							}
							back->SourceSatellite_interface = j;
							back->Metric = 1;
							back->NextLSA = NULL;
						}
						if ((abs(opspf->st[ (opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8].STC.lat) > OpspfBeita) && (abs(opspf->st[ (opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8].STC.lat) <= 90))
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = 0;
							back->DestinationSatellite_interface = 3;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							back->SourceSatellite_interface = 1;
							back->Metric = OPSPFMAX;
							back->NextLSA = NULL;
						}
						else
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = (opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8;
							back->SourceSatellite_ID =(opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
								break;
								}
							}
							back->DestinationSatellite_interface = j;
							for(j=0;j<6;j++){
							if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
								break;
								}
							}
							back->SourceSatellite_interface = j;
							back->Metric = 1;
							back->NextLSA = NULL;
						}
					}
				else
					{
						if ((abs(opspf->st[i+8].STC.lat) > OpspfBeita) && (abs(opspf->st[ i+8].STC.lat) <= 90))
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = 0;
							back->DestinationSatellite_interface = 3;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							back->SourceSatellite_interface = 1;
							back->Metric = OPSPFMAX;
							back->NextLSA = NULL;	
						}
						else
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = (opspf->st[i].orbitnum)  * 8 + opspf->st[i].index - 1;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
								break;
								}
							}
							back->DestinationSatellite_interface = j;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
								break;
								}
							}
							back->SourceSatellite_interface = j;
							back->Metric = 1;
							back->NextLSA = NULL;
						}
						if ((abs(opspf->st[i-8].STC.lat) > OpspfBeita) && (abs(opspf->st[i-8].STC.lat) <= 90))
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = 0;
							back->DestinationSatellite_interface = 1;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
							back->SourceSatellite_interface = 3;
							back->Metric = OPSPFMAX;
							back->NextLSA = NULL;
						}
						else
						{					
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID =(opspf->st[i].orbitnum-2)*8 + opspf->st[i].index -1;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum - 1) * 8 + opspf->st[i].index - 1;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
								break;
								}
							}
							back->DestinationSatellite_interface = j;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
								break;
								}
							}
							back->SourceSatellite_interface = j;
							back->Metric = 1;
							back->NextLSA = NULL;
						}
				}
			}
			else
			{
				head = (LSA*)malloc(sizeof(LSA));
				back->NextLSA = head;
				back = head;
				back->DestinationSatellite_ID = 0;
				back->DestinationSatellite_interface = 3;
				back->SourceSatellite_ID = (opspf->st[i].orbitnum-1 )*8+ opspf->st[i].index-1;
				back->SourceSatellite_interface =1;
				back->Metric = OPSPFMAX;
				back->NextLSA = NULL;

				if ((((opspf->st[i].STC.lat)- (opspf->oldst[i].STC.lat))>=0)||(opspf->isfirst==1))
					{
						opspf->isfirst=0;
						if ((abs(opspf->st[ (opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8].STC.lat) > OpspfBeita) && (abs(opspf->st[ (opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8].STC.lat) <= 90))
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID = 0;
							back->DestinationSatellite_interface = 1;
							back->SourceSatellite_ID =( opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							back->SourceSatellite_interface = 3;
							back->Metric = OPSPFMAX;
							back->NextLSA = NULL;
						}
						else
						{
							head = (LSA*)malloc(sizeof(LSA));
							back->NextLSA = head;
							back = head;
							back->DestinationSatellite_ID =(opspf->st[i].orbitnum-2)*8+ (opspf->st[i].index)%8;
							back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
								break;
								}
							}
							back->DestinationSatellite_interface = j;
							for(j=0;j<6;j++){
								if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
								break;
								}
							}
							back->SourceSatellite_interface = j;
							back->Metric = 1;
							back->NextLSA = NULL;
						}
					}
				else
					{				
							if ((abs(opspf->st[i-8].STC.lat) > OpspfBeita) && (abs(opspf->st[ i-8].STC.lat) <= 90))
							{
								head = (LSA*)malloc(sizeof(LSA));
								back->NextLSA = head;
								back = head;
								back->DestinationSatellite_ID = 0;
								back->DestinationSatellite_interface = 1;
								back->SourceSatellite_ID =( opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
								back->SourceSatellite_interface = 3;
								back->Metric = OPSPFMAX;
								back->NextLSA = NULL;
							}
							else
							{
								head = (LSA*)malloc(sizeof(LSA));
								back->NextLSA = head;
								back = head;
								back->DestinationSatellite_ID = (opspf->st[i].orbitnum - 2) * 8 + opspf->st[i].index-1;
								back->SourceSatellite_ID = (opspf->st[i].orbitnum-1)*8 + opspf->st[i].index-1;
								for(j=0;j<6;j++){
									if(opspf->linktable[back->DestinationSatellite_ID][j]==back->SourceSatellite_ID){
									break;
									}
								}
								back->DestinationSatellite_interface = j;
								for(j=0;j<6;j++){
									if(opspf->linktable[back->SourceSatellite_ID][j]==back->DestinationSatellite_ID){
									break;
									}
								}
								back->SourceSatellite_interface = j;
								back->Metric = 1;
								back->NextLSA = NULL;
							}
					}
				}
			}
		}

	for (int i = 0; i < 48; i++)
	{
		opspf->oldst[i].STC.lat = opspf->st[i].STC.lat;
	}
}

void FindShortestPath(Node* node){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	ReadLSAToGraph(opspf->lsdb,opspf->GraphLinkList);
	Dijstra(opspf->GraphLinkList,node->nodeId-1,&(opspf->opspfShortestVertex));
}

void freeLSAList(LSA* LSAListhead)
{
	LSA* LSAListp;
	LSA* LSAListq;
	if (LSAListhead == NULL)
	{
		return ;
	}
	LSAListp = LSAListhead;
	while (LSAListp != NULL)
	{
		LSAListq = LSAListp;
		LSAListp = LSAListp->NextLSA;
		free(LSAListq);
		
	}
	LSAListhead=NULL;
}

LSA* OpspfLookupLSAList(LSDB* lsdb, int SatelliteIndex)
{
	LSA* tempLsa;
	tempLsa = lsdb->LSAList;
	while (tempLsa)
	{
		if (tempLsa->SourceSatellite_ID == SatelliteIndex)
		{
			return tempLsa;
		}
		tempLsa = tempLsa->NextLSA;
	}
	return NULL;

}

void ReadLSAToGraph(LSDB* lsdb, OpspfVertex GraphLinkList[TotalNode])
{
	LSA* SatelliteLSA;
	OpspfVertex* LinkVertex;
	OpspfVertex* tempLink;
	int i;
	int index;

	for (i = 0; i < TotalNode; i++)
	{
		index = i;
		SatelliteLSA = OpspfLookupLSAList(lsdb, index);
		// init the GraphLinkList
		GraphLinkList[i].distance = 0;
		GraphLinkList[i].vertexId = i;
		GraphLinkList[i].NextVertex = NULL;
		tempLink = GraphLinkList + i;
		if (SatelliteLSA)
		{
			while (SatelliteLSA)
			{
				if((SatelliteLSA->SourceSatellite_ID == index)&&(SatelliteLSA->Metric != OPSPFMAX)){
					LinkVertex = (OpspfVertex*)malloc(sizeof(OpspfVertex));
					LinkVertex->distance = SatelliteLSA->Metric;
					LinkVertex->vertexId = SatelliteLSA->DestinationSatellite_ID;
					LinkVertex->NextVertex = NULL;
					tempLink->NextVertex = LinkVertex;
					tempLink = LinkVertex;
				}
				SatelliteLSA = SatelliteLSA->NextLSA;
			}
		}

	}
}

void FreeList(OpspfShortestVertex** List)
{
	OpspfShortestVertex* TempVertexHead;
	OpspfShortestVertex* TempVertex;
	if (*List == NULL)
	{
		return;
	}
	else
	{
		TempVertex = *List;
		TempVertexHead = (*List)->NextShortestVertex;
		if(TempVertex!=TempVertexHead){
			while (TempVertexHead)
				{
				free(TempVertex);
				TempVertex = TempVertexHead;
				TempVertexHead = TempVertexHead->NextShortestVertex;
				}
			}
		free(TempVertex);
		*List = NULL;
	}
}

OpspfShortestVertex* SearchVertex(OpspfShortestVertex* Path, NodeAddress VertexId)
{
	OpspfShortestVertex* TempShortestVertex = Path;
	while (TempShortestVertex)
	{
		if (TempShortestVertex->DestinationVertex == VertexId)
		{
			return TempShortestVertex;
		}
		TempShortestVertex = TempShortestVertex->NextShortestVertex;
	}
	return NULL;
}

void Dijstra(OpspfVertex GraphLinkList[TotalNode], NodeAddress Start, OpspfShortestVertex** Path)
{
	OpspfVertex W;
	OpspfVertex* TempVertex;
	OpspfShortestVertex* TempShortestVertex;
	OpspfShortestVertex* TempShortestVertexHead;

	unsigned int min;
	unsigned int Arcs[TotalNode];
	int i, j;
	int VertexNum;
	bool Known[TotalNode] = { false };

	for (i = 0; i < TotalNode; i++)
	{
		Arcs[i] = OPSPFMAX;
	}
	Arcs[Start] = 0;

	FreeList(Path);

	TempShortestVertex = (OpspfShortestVertex*)malloc(sizeof(OpspfShortestVertex));
	TempShortestVertex->distance = 0;
	TempShortestVertex->DestinationVertex = Start;
	TempShortestVertex->AdvertisingVertext = Start;
	TempShortestVertex->NextShortestVertex = NULL;

	*Path = TempShortestVertex;
	TempShortestVertexHead = *Path;
	for (i = 0; i < TotalNode; i++)
	{
		min = OPSPFMAX;
		for (j = 0; j < TotalNode; j++)
		{
			if (!Known[j])
			{
				if (Arcs[j]<min)
				{
					W = GraphLinkList[j];
					min = Arcs[j];
				}
			}
		}   
		VertexNum = W.vertexId;
		Known[VertexNum] = true;

		TempVertex = GraphLinkList[VertexNum].NextVertex;

		while (TempVertex)
		{
			VertexNum = TempVertex->vertexId;
			if (!Known[VertexNum] && (min + TempVertex->distance)<Arcs[VertexNum])
			{
				Arcs[VertexNum] = min + TempVertex->distance;
				TempShortestVertex = SearchVertex(*Path, VertexNum);
				if (TempShortestVertex != NULL && TempShortestVertex->distance>Arcs[VertexNum])
				{
					TempShortestVertex->distance = Arcs[VertexNum];
					TempShortestVertex->AdvertisingVertext = W.vertexId;
				}
				else
				{
					TempShortestVertex = (OpspfShortestVertex*)malloc(sizeof(OpspfShortestVertex));
					TempShortestVertex->distance = Arcs[VertexNum];
					TempShortestVertex->DestinationVertex = VertexNum;
					TempShortestVertex->AdvertisingVertext = W.vertexId;
					TempShortestVertex->NextShortestVertex = NULL;
					TempShortestVertexHead->NextShortestVertex = TempShortestVertex;
					TempShortestVertexHead = TempShortestVertex;
				}
			}
			TempVertex = TempVertex->NextVertex;
		}
	}
}

void VertexHopForRouting(
	NodeAddress AdvertisingVertex,
	NodeAddress DestinationVertex,
	OpspfShortestVertex* Path,
	OpspfShortestVertex** Router)
{
	OpspfShortestVertex* PathVertex;
	OpspfShortestVertex* HeadVertex;
	OpspfShortestVertex* TempVertex;
	NodeAddress SatelliteIndex;

	SatelliteIndex = DestinationVertex;
	PathVertex = Path;

	FreeList(Router);

	PathVertex = (OpspfShortestVertex*)malloc(sizeof(OpspfShortestVertex));
	TempVertex = SearchVertex(Path, AdvertisingVertex);


	PathVertex->AdvertisingVertext = TempVertex->AdvertisingVertext;
	PathVertex->DestinationVertex = TempVertex->DestinationVertex;
	PathVertex->distance = TempVertex->distance;
	PathVertex->NextShortestVertex = NULL;
	
	*Router = PathVertex;
	HeadVertex = PathVertex;
	while (SatelliteIndex!=AdvertisingVertex)
	{
		TempVertex = SearchVertex(Path, SatelliteIndex);
		if (TempVertex==NULL)
		{
			cout << "Destination cannot be reached." << endl;
			return;
		}

		SatelliteIndex = TempVertex->AdvertisingVertext;

		PathVertex = (OpspfShortestVertex*)malloc(sizeof(OpspfShortestVertex));
		PathVertex->AdvertisingVertext = TempVertex->AdvertisingVertext;
		PathVertex->DestinationVertex = TempVertex->DestinationVertex;
		PathVertex->distance = TempVertex->distance;
		PathVertex->NextShortestVertex = HeadVertex->NextShortestVertex;
		HeadVertex->NextShortestVertex = PathVertex;
	}
	if(HeadVertex->NextShortestVertex==NULL)
	{
		HeadVertex->NextShortestVertex=HeadVertex;
	}
}


void FillOpspfRoutingTable(Node* node){

	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);

	NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_DEFAULT);
	NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OPSPF);

	OpspfShortestVertex* a=NULL;
	OpspfShortestVertex* tempv=NULL;

	LSA* DesLsa;
	LSA* NextHopLsa;

	int DestinationIndex;
	
	// When the destination is satellites links
	for(int i=0;i<128;i++){

		VertexHopForRouting(node->nodeId-1,opspf->linknode[i],opspf->opspfShortestVertex,&a);
		
		DestinationIndex = opspf->linknode[i];

		if(a->NextShortestVertex!=a)
		{
			tempv=a->NextShortestVertex;				
			NextHopLsa = opspf->lsdb->LSAList;
			while (NextHopLsa)
			{
				if ((NextHopLsa->SourceSatellite_ID == tempv->AdvertisingVertext)&&(NextHopLsa->DestinationSatellite_ID == tempv->DestinationVertex))
				{
					break;
				}
				NextHopLsa = NextHopLsa->NextLSA;
			}
			while(tempv->NextShortestVertex != NULL)
			{
				tempv=tempv->NextShortestVertex;
			}

			if(NextHopLsa!=NULL) // when find the path to the link
			{
				NodeAddress DstAddress = (190<<24) + ((opspf->linkid[i])<<8) ;
				NodeAddress NextHopAddress=opspf->idtoaddress[a->NextShortestVertex->DestinationVertex][NextHopLsa->DestinationSatellite_interface];
				NodeAddress NetMask = (255<<24) + (255<<16) + (255<<8);
				NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,NextHopLsa->SourceSatellite_interface,tempv->distance,ROUTING_PROTOCOL_OPSPF);
			}
		}
		else // when self is in the destination subnet
		{
			int j=0;
			for(j=0;j<6;j++)
			{
				if( (((opspf->idtoaddress[node->nodeId-1][j]) & 0xffff) >> 8) == (opspf->linkid[i]) ) break;
			}
			int SourceInterface=j;
			NodeAddress DstAddress = opspf->linktable[node->nodeId-1][SourceInterface];
			for(j=0;j<6;j++)
			{
				if( (((opspf->idtoaddress[DstAddress][j]) & 0xffff) >> 8) == (opspf->linkid[i]) ) break;
			}
			int DstInterface = j;
			NodeAddress NextHopAddress = opspf->idtoaddress[DstAddress][DstInterface];
			DstAddress = (190<<24) + ((opspf->linkid[i])<<8);
			NodeAddress NetMask = (255<<24) + (255<<16) + (255<<8);
			NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,SourceInterface,0,ROUTING_PROTOCOL_OPSPF);
		}
	}
	// When the destination is user
	if(opspf->enableChangeAddress)
	{
		for(int i=0;i<48;i++)
		{
			int areaId = opspf->s_a[i];
			VertexHopForRouting(node->nodeId-1, i ,opspf->opspfShortestVertex,&a);
			if(a->NextShortestVertex!=a)
			{
				tempv=a->NextShortestVertex;				
				NextHopLsa=opspf->lsdb->LSAList;
				while (NextHopLsa)
				{
					if ((NextHopLsa->SourceSatellite_ID == tempv->AdvertisingVertext)&&(NextHopLsa->DestinationSatellite_ID == tempv->DestinationVertex))
					{
						break;
					}
					NextHopLsa = NextHopLsa->NextLSA;
				}
				while(tempv->NextShortestVertex!=NULL)
				{
					tempv=tempv->NextShortestVertex;
				}

				if(NextHopLsa!=NULL)
				{				
					NodeAddress DstAddress = areaId<<24;
					NodeAddress NextHopAddress = opspf->idtoaddress[a->NextShortestVertex->DestinationVertex][NextHopLsa->DestinationSatellite_interface];						
					NodeAddress NetMask = 255<<24;
					NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,NextHopLsa->SourceSatellite_interface,tempv->distance,ROUTING_PROTOCOL_OPSPF);
				}
			}
			else
			{
				NodeAddress DstAddress = areaId<<24;
				NodeAddress NextHopAddress = (areaId<<24) + (255<<16) + (255<<8) + 255;
				NodeAddress NetMask = 255<<24;
				int inf = node->numberInterfaces - 1;
				NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,inf,0,ROUTING_PROTOCOL_OPSPF);
			}
		} // end for(int i=0;i<48;i++)...
	} // end if(opspf->enableChangeAddress)...
	
	//NetworkPrintForwardingTable(node);
}

//------------------------------------------------------------
// api for change address
//------------------------------------------------------------
void UpdateSATable(Node* node,Message* msg){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);	
	
	int sheet_s_a[48] =  {99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99};
 	int sheet_a_s[48] =  {99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99,
                            99,99,99,99,99,99,99,99};
	Coord sats[48];
	int currentTime=(int)(node->getNodeTime() / SECOND);
	Fin(currentTime,sats);	
	FixLon(currentTime, sats);	
	for(int i=0; i<48; i++){
	   sheet_s_a[i] = GetAddressID(sats[i].y,sats[i].x);
	}
	for(int i=0; i<48; i++){
	    int k = sheet_s_a[i];
	    if(k%2==0){
	        sheet_a_s[k] = i;
	    }
	}
	for(int i=0; i<12; i++){
	    int a = sheet_a_s[4*i];
	    int b = sheet_a_s[4*i+2];
	    if(a%8==0&&b%8==7){
	        sheet_a_s[4*i+1] = a+1;
	        sheet_a_s[4*i+3] = b-1;
	    }
	    else if(a%8==7&&b%8==0){
	        sheet_a_s[4*i+1] = a-1;
	        sheet_a_s[4*i+3] = b+1;
	    }
	    else if(a%8==0&&b%8==1){
	        sheet_a_s[4*i+1] = a+7;
	        sheet_a_s[4*i+3] = b+1;
	    }
	    else if(a%8==1&&b%8==0){
	        sheet_a_s[4*i+1] = a+1;
	        sheet_a_s[4*i+3] = b+7;
	    }
	    else if(a%8==6&&b%8==7){
	        sheet_a_s[4*i+1] = a-1;
	        sheet_a_s[4*i+3] = b-7;
	    }
	    else if(a%8==7&&b%8==6){
	        sheet_a_s[4*i+1] = a-7;
	        sheet_a_s[4*i+3] = b-1;
	    }
	    else if(a<b){
	        sheet_a_s[4*i+1] = a-1;
	        sheet_a_s[4*i+3] = b+1;
	    }
	    else if(a>b){
	        sheet_a_s[4*i+1] = a+1;
	        sheet_a_s[4*i+3] = b-1;
	    }
	}
	for(int i=0;i<48;i++){
		opspf->s_a[i]=sheet_s_a[i];
	}
	for(int i=0; i<48; i++){
		int n=sheet_a_s[i];
		opspf->s_a[n] = i + 1;
	}
	
	//for(int i=0;i<48;i++) printf("%d\n",(int)opspf->s_a[i]);
	
	ChangeAdderss(node,msg);
}

double radian(double d)
{
    return d * PI / 180.0;
}

int GetAddressID(double lon, double lat){
	int mat[12][4]={
		0,1,2,3,
		4,5,6,7,
		8,9,10,11,
		12,13,14,15,
		16,17,18,19,
		20,21,22,23,
		24,25,26,27,
		28,29,30,31,
		32,33,34,35,
		36,37,38,39,
		40,41,42,43,
		44,45,46,47
		};
		int r,c;
		//lon
		if (lon == 180){
			r = 5;
		}
		else if (lon == -180){
			r = 11;
		}
		else if (lon < 0){
			r = (-lon / 30) + 6;
		}
		else{
			r = lon / 30;
		}
		//lat
		if (lat == 90){
			c = 1;
		}
		else if (lat == -90){
			c = 3;
		}
		else if (lat < 0){
			c = (-lat / 45) + 2;
		}
		else{
			c = lat / 45;
		}

		return mat[r][c];
}

void Fin(int t,Coord mat[48]){
	int start_time=0;
	ifstream infile;	
	infile.open( OPSPF_MOBILITY_FILE , ios::in|ios::binary );
	if(!infile)
	{
		cerr<<"open error!"<<endl;
		exit(1);
	}
	for(int i=0; i<48; i++){
            start_time = i * DATA_SIZE + t;
        	infile.seekg(sizeof(Coord) * start_time,ios_base::beg);
	        infile.read((char*)(mat+i), sizeof(Coord));
	}
	infile.close();
}

double Dec(double lat){
    double lat_fix = lat;
    if(lat>86) lat_fix =86;
    if(lat<-86) lat_fix =-86;
    double a = radian(4);
    double b = radian(lat_fix);
    double c = tan(a)*tan(b);
    double dec = asin(c);
	if (c >= 1) dec = PI / 2;//add by cyj 2016.9.23 , To avoid overflow!
    return double((dec/PI)*180);
}

void FixLon(int t,Coord sat[]){
    Coord later[48];
    Fin(t+1,later);
    double out;
    for(int i=0; i<48; i++){
        if(sat[i].x>later[i].x){
                out = sat[i].y+Dec(sat[i].x);
                if(out>180){
                    out = -180+(sat[i].y+Dec(sat[i].x)-180);
                }
                else if(out<-180){
                    out = 180+(sat[i].y+Dec(sat[i].x)+180);
                }
                sat[i].y = out;
        }
        else {
                out = sat[i].y-Dec(sat[i].x);
                if(out<-180){
                    out = 180+(out+180);
                }
                else if(out>180){
                    out = -180+(out-180);
                }
                sat[i].y = out;
        }		
    }
}

void ChangeAdderss( Node* node , Message* msg ){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	//Change IP
	NetworkDataIp* ip = node->networkData.networkVar;
	int inf = node->numberInterfaces - 1;
	IpInterfaceInfoType* interfaceInfo= ip->interfaceInfo[inf];
	
	Address oldAddress ;
	Address sendAddress;

	oldAddress.interfaceAddr.ipv4 = interfaceInfo->ipAddress;
	oldAddress.networkType = NETWORK_IPV4;
	
	sendAddress.networkType = NETWORK_IPV4;
	// Change the Adderss by current s_a mapping table
	sendAddress.interfaceAddr.ipv4 = (((int)opspf->s_a[node->nodeId-1])<<24) + 1;

	interfaceInfo->ipAddress = sendAddress.interfaceAddr.ipv4;
	interfaceInfo->numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(0xff000000);

	// refresh the forward table
	NetworkForwardingTable* loopbackFwdTable = &(ip->loopbackFwdTable);
	for (Int32 i = 0; i < loopbackFwdTable->size; i++)
	{
		if (loopbackFwdTable->row[i].destAddress == oldAddress.interfaceAddr.ipv4)
				loopbackFwdTable->row[i].destAddress = interfaceInfo->ipAddress ;
	}
	
	PhyData* phyData = node->phyData[0];
	phyData->networkAddress->interfaceAddr.ipv4 = sendAddress.interfaceAddr.ipv4;  

	MAPPING_InvalidateIpv4AddressForInterface(
		node,
		inf,
		&oldAddress);

	MAPPING_ValidateIpv4AddressForInterface(
		node,
		inf,
		interfaceInfo->ipAddress,
		0xff000000,
		NETWORK_IPV4);

	IpSendNotificationOfAddressChange(
		node,
		inf,
		&oldAddress,
		0xffffff00,
		NETWORK_IPV4);

	GUI_SendAddressChange(
		node->nodeId,
		inf,
		sendAddress,
		NETWORK_IPV4);
}

void ChangeUserAdderss(Node* node){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	//Change IP
	NetworkDataIp* ip = node->networkData.networkVar;
	int inf = 0;
	IpInterfaceInfoType* interfaceInfo= ip->interfaceInfo[inf];
	
	Address oldAddress ;
	Address sendAddress;

	oldAddress.interfaceAddr.ipv4 = interfaceInfo->ipAddress;
	oldAddress.networkType = NETWORK_IPV4;

	double lon = node->mobilityData->current->position.latlonalt.longitude;
	double lat = node->mobilityData->current->position.latlonalt.latitude;
	int areaId = GetAddressID(lon, lat) + 1;
	sendAddress.networkType = NETWORK_IPV4;
	sendAddress.interfaceAddr.ipv4 = (areaId<<24) + node->nodeId - 48 + 1;

	interfaceInfo->ipAddress = sendAddress.interfaceAddr.ipv4;
	interfaceInfo->numHostBits = AddressMap_ConvertSubnetMaskToNumHostBits(0xff000000);	
	
	PhyData* phyData = node->phyData[0];
	phyData->networkAddress->interfaceAddr.ipv4 = sendAddress.interfaceAddr.ipv4;  

	MAPPING_InvalidateIpv4AddressForInterface(
		node,
		inf,
		&oldAddress);

	MAPPING_ValidateIpv4AddressForInterface(
		node,
		inf,
		interfaceInfo->ipAddress,
		0xff000000,
		NETWORK_IPV4);

	IpSendNotificationOfAddressChange(
		node,
		inf,
		&oldAddress,
		0xffffff00,
		NETWORK_IPV4);

	GUI_SendAddressChange(
		node->nodeId,
		inf,
		sendAddress,
		NETWORK_IPV4);
}

//------------------------------------------------------------
// api for register
//------------------------------------------------------------
void OpspfAddRoutingTableRowById(Node* node, NodeAddress nodeId, NodeAddress dstAddr){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OPSPF_TEMP);
	OpspfShortestVertex* a = NULL;
	OpspfShortestVertex* tempv = NULL;

	LSA* DesLsa;
	LSA* NextHopLsa;

	int DestinationIndex;

	int areaId = opspf->s_a[nodeId-1];
	VertexHopForRouting(node->nodeId-1, nodeId-1, opspf->opspfShortestVertex, &a);
	if(a->NextShortestVertex != a)
	{
		tempv = a->NextShortestVertex;
		NextHopLsa = opspf->lsdb->LSAList;
		while (NextHopLsa)
		{
			if ((NextHopLsa->SourceSatellite_ID == tempv->AdvertisingVertext)&&(NextHopLsa->DestinationSatellite_ID == tempv->DestinationVertex))
			{
				break;
			}
			NextHopLsa = NextHopLsa->NextLSA;
		}
		while(tempv->NextShortestVertex != NULL)
		{
			tempv = tempv->NextShortestVertex;
		}
		if(NextHopLsa!=NULL)
		{				
			NodeAddress DstAddress = dstAddr;
			NodeAddress NextHopAddress = opspf->idtoaddress[a->NextShortestVertex->DestinationVertex][NextHopLsa->DestinationSatellite_interface];						
			NodeAddress NetMask = (255<<24) + (255<<16) + (255<<8) + 255;
			NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,NextHopLsa->SourceSatellite_interface,tempv->distance,ROUTING_PROTOCOL_OPSPF_TEMP);
		}
	}
	else{
		NodeAddress DstAddress = dstAddr;
		NodeAddress NextHopAddress = dstAddr;
		int inf = node->numberInterfaces - 1;
		NodeAddress NetMask = (255<<24) + (255<<16) + (255<<8) + 255;
		NetworkUpdateForwardingTable(node,DstAddress,NetMask,NextHopAddress,inf,0,ROUTING_PROTOCOL_OPSPF_TEMP);
	}
}

void InitRegisterList(Node* node){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	opspf->rList = (registerList*)malloc(sizeof(registerList));	
	opspf->rList->head=NULL;
	opspf->rList->size=0;
}

void AddRegisterListRow(Node* node, NodeAddress usrip, bool isRelay, int RelaySTId){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	if (opspf->rList->head != NULL) {
		registerRow* tempRow = opspf->rList->head;
		while (tempRow->next != NULL) tempRow = tempRow->next;
		registerRow* newRow = (registerRow*)malloc(sizeof(registerRow));
		newRow->isRelay=isRelay;
		newRow->RelaySTId=RelaySTId;
		newRow->usrip=usrip;
		newRow->next = NULL;
		tempRow->next = newRow;
	}
	else {
		opspf->rList->head = (registerRow*)malloc(sizeof(registerRow));
		opspf->rList->head->isRelay=isRelay;
		opspf->rList->head->RelaySTId=RelaySTId;
		opspf->rList->head->usrip=usrip;
		opspf->rList->head->next = NULL;
	}
	opspf->rList->size++;
}

registerRow* FindRegisterRowByUsrIp(Node* node, NodeAddress usrip){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	registerRow* tempRow = opspf->rList->head;
	while (tempRow != NULL) {
		if (tempRow->usrip==usrip) return tempRow;
		tempRow = tempRow->next;
	}
	return NULL;
}

void FreeRegisterList(Node* node){
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);
	registerRow* p = opspf->rList->head;
	if (p != NULL) {
		while (p->next != NULL) {
			opspf->rList->head = p->next;
			free(p);
			p = opspf->rList->head;
		}
		free(opspf->rList->head);
	}
	opspf->rList->head = NULL;
	opspf->rList->size = 0;
}

void OpspfAddReigsterRow( Node* node , registerInfo* addInfo )
{
	OpspfData* opspf = (OpspfData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OPSPF);

	if(addInfo->isRelay==0)
	{// when this register packet has no relay
		registerRow* usrRow = FindRegisterRowByUsrIp(node,addInfo->usrip);
		if(usrRow!=NULL)
		{// already have a register row of this user, update it
			usrRow->isRelay=addInfo->isRelay;
			usrRow->RelaySTId=addInfo->RelaySTId;
		}
		else
		{// don't have such row , add a new row for this user
			AddRegisterListRow(node,addInfo->usrip,addInfo->isRelay,addInfo->RelaySTId);
		}
		// save the last time that the user has no relay
		opspf->LastNoRelay = node->getNodeTime();
	}
	else
	{// when the register packet has relay
		registerRow* usrRow = FindRegisterRowByUsrIp(node,addInfo->usrip);
		if(usrRow!=NULL)
		{//already have a register row of this user
			if(usrRow->isRelay!=0)
			{// this row has relay , update it
				usrRow->isRelay=addInfo->isRelay;
				usrRow->RelaySTId=addInfo->RelaySTId;
			}
			else
			{// this row has no relay
				if(node->getNodeTime()-opspf->LastNoRelay>1*SECOND)
				{ // when the no relay row has time out , update it
					usrRow->isRelay=addInfo->isRelay;
					usrRow->RelaySTId=addInfo->RelaySTId;
				}
			}
		}
		else
		{// don't have such row , add a new row for this user
			AddRegisterListRow(node,addInfo->usrip,addInfo->isRelay,addInfo->RelaySTId);
		}
	}

	
	printf( "Node %d , the updated RegisterList at time %d is :\n", node->nodeId, (int)(node->getNodeTime()/SECOND) );
	if (opspf->rList != NULL){
		registerRow* temp = opspf->rList->head;
		while (temp != NULL){
			printf("usrip: %08X , isRelay: %d , RelaySTId: %d\n", temp->usrip , temp->isRelay , temp->RelaySTId);
			temp = temp->next;
		}
	}
	
	
}

void OpspfInitSTScenario(OpspfData* opspf)
{
	//----------------------------------------------------------------
	// Due to the bug in Exata , we can't self define the interfaces
	// So read it from the config file.
	// See more in struct define of "OpspfData"
	//----------------------------------------------------------------	
	opspf->idtoaddress[0][0]=-1107295997;
	opspf->idtoaddress[0][1]=-1107295742;
	opspf->idtoaddress[0][2]=-1107295487;
	opspf->idtoaddress[0][3]=-1107295231;
	opspf->idtoaddress[0][4]=0;
	opspf->idtoaddress[0][5]=0;
	opspf->idtoaddress[1][0]=-1107295998;
	opspf->idtoaddress[1][1]=-1107294975;
	opspf->idtoaddress[1][2]=-1107294719;
	opspf->idtoaddress[1][3]=-1107294463;
	opspf->idtoaddress[1][4]=0;
	opspf->idtoaddress[1][5]=0;
	opspf->idtoaddress[2][0]=-1107294974;
	opspf->idtoaddress[2][1]=-1107294207;
	opspf->idtoaddress[2][2]=-1107293951;
	opspf->idtoaddress[2][3]=-1107293695;
	opspf->idtoaddress[2][4]=0;
	opspf->idtoaddress[2][5]=0;
	opspf->idtoaddress[3][0]=-1107294206;
	opspf->idtoaddress[3][1]=-1107293439;
	opspf->idtoaddress[3][2]=-1107293183;
	opspf->idtoaddress[3][3]=-1107292927;
	opspf->idtoaddress[3][4]=0;
	opspf->idtoaddress[3][5]=0;
	opspf->idtoaddress[4][0]=-1107293438;
	opspf->idtoaddress[4][1]=-1107292671;
	opspf->idtoaddress[4][2]=-1107292415;
	opspf->idtoaddress[4][3]=-1107292159;
	opspf->idtoaddress[4][4]=0;
	opspf->idtoaddress[4][5]=0;
	opspf->idtoaddress[5][0]=-1107292670;
	opspf->idtoaddress[5][1]=-1107291903;
	opspf->idtoaddress[5][2]=-1107291647;
	opspf->idtoaddress[5][3]=-1107291391;
	opspf->idtoaddress[5][4]=0;
	opspf->idtoaddress[5][5]=0;
	opspf->idtoaddress[6][0]=-1107291902;
	opspf->idtoaddress[6][1]=-1107291135;
	opspf->idtoaddress[6][2]=-1107290623;
	opspf->idtoaddress[6][3]=-1107262462;
	opspf->idtoaddress[6][4]=0;
	opspf->idtoaddress[6][5]=0;
	opspf->idtoaddress[7][0]=-1107295740;
	opspf->idtoaddress[7][1]=-1107291134;
	opspf->idtoaddress[7][2]=-1107290879;
	opspf->idtoaddress[7][3]=-1107262719;
	opspf->idtoaddress[7][4]=0;
	opspf->idtoaddress[7][5]=0;
	opspf->idtoaddress[8][0]=-1107295486;
	opspf->idtoaddress[8][1]=-1107294718;
	opspf->idtoaddress[8][2]=-1107290367;
	opspf->idtoaddress[8][3]=-1107290111;
	opspf->idtoaddress[8][4]=-1107289855;
	opspf->idtoaddress[8][5]=-1107289599;
	opspf->idtoaddress[9][0]=-1107294462;
	opspf->idtoaddress[9][1]=-1107293950;
	opspf->idtoaddress[9][2]=-1107290366;
	opspf->idtoaddress[9][3]=-1107289343;
	opspf->idtoaddress[9][4]=-1107289087;
	opspf->idtoaddress[9][5]=-1107288831;
	opspf->idtoaddress[10][0]=-1107293694;
	opspf->idtoaddress[10][1]=-1107293182;
	opspf->idtoaddress[10][2]=-1107289342;
	opspf->idtoaddress[10][3]=-1107288575;
	opspf->idtoaddress[10][4]=-1107288319;
	opspf->idtoaddress[10][5]=-1107288063;
	opspf->idtoaddress[11][0]=-1107292926;
	opspf->idtoaddress[11][1]=-1107292414;
	opspf->idtoaddress[11][2]=-1107288574;
	opspf->idtoaddress[11][3]=-1107287807;
	opspf->idtoaddress[11][4]=-1107287551;
	opspf->idtoaddress[11][5]=-1107287295;
	opspf->idtoaddress[12][0]=-1107292158;
	opspf->idtoaddress[12][1]=-1107291646;
	opspf->idtoaddress[12][2]=-1107287806;
	opspf->idtoaddress[12][3]=-1107287039;
	opspf->idtoaddress[12][4]=-1107286783;
	opspf->idtoaddress[12][5]=-1107286527;
	opspf->idtoaddress[13][0]=-1107291390;
	opspf->idtoaddress[13][1]=-1107287038;
	opspf->idtoaddress[13][2]=-1107286271;
	opspf->idtoaddress[13][3]=-1107286015;
	opspf->idtoaddress[13][4]=-1107285759;
	opspf->idtoaddress[13][5]=-1107262463;
	opspf->idtoaddress[14][0]=-1107290622;
	opspf->idtoaddress[14][1]=-1107286270;
	opspf->idtoaddress[14][2]=-1107285503;
	opspf->idtoaddress[14][3]=-1107285247;
	opspf->idtoaddress[14][4]=-1107284991;
	opspf->idtoaddress[14][5]=-1107262718;
	opspf->idtoaddress[15][0]=-1107295230;
	opspf->idtoaddress[15][1]=-1107290110;
	opspf->idtoaddress[15][2]=-1107285502;
	opspf->idtoaddress[15][3]=-1107284735;
	opspf->idtoaddress[15][4]=-1107284479;
	opspf->idtoaddress[15][5]=-1107290878;
	opspf->idtoaddress[16][0]=-1107289854;
	opspf->idtoaddress[16][1]=-1107289086;
	opspf->idtoaddress[16][2]=-1107284223;
	opspf->idtoaddress[16][3]=-1107283967;
	opspf->idtoaddress[16][4]=-1107283711;
	opspf->idtoaddress[16][5]=-1107283455;
	opspf->idtoaddress[17][0]=-1107288830;
	opspf->idtoaddress[17][1]=-1107288062;
	opspf->idtoaddress[17][2]=-1107284222;
	opspf->idtoaddress[17][3]=-1107283199;
	opspf->idtoaddress[17][4]=-1107282943;
	opspf->idtoaddress[17][5]=-1107282687;
	opspf->idtoaddress[18][0]=-1107288318;
	opspf->idtoaddress[18][1]=-1107287550;
	opspf->idtoaddress[18][2]=-1107283198;
	opspf->idtoaddress[18][3]=-1107282431;
	opspf->idtoaddress[18][4]=-1107282175;
	opspf->idtoaddress[18][5]=-1107281919;
	opspf->idtoaddress[19][0]=-1107287294;
	opspf->idtoaddress[19][1]=-1107286782;
	opspf->idtoaddress[19][2]=-1107282430;
	opspf->idtoaddress[19][3]=-1107281663;
	opspf->idtoaddress[19][4]=-1107281407;
	opspf->idtoaddress[19][5]=-1107281151;
	opspf->idtoaddress[20][0]=-1107286526;
	opspf->idtoaddress[20][1]=-1107286014;
	opspf->idtoaddress[20][2]=-1107281662;
	opspf->idtoaddress[20][3]=-1107280639;
	opspf->idtoaddress[20][4]=-1107280383;
	opspf->idtoaddress[20][5]=-1107280127;
	opspf->idtoaddress[21][0]=-1107285758;
	opspf->idtoaddress[21][1]=-1107285246;
	opspf->idtoaddress[21][2]=-1107280638;
	opspf->idtoaddress[21][3]=-1107279871;
	opspf->idtoaddress[21][4]=-1107279615;
	opspf->idtoaddress[21][5]=-1107279359;
	opspf->idtoaddress[22][0]=-1107284990;
	opspf->idtoaddress[22][1]=-1107284734;
	opspf->idtoaddress[22][2]=-1107279870;
	opspf->idtoaddress[22][3]=-1107279103;
	opspf->idtoaddress[22][4]=-1107278847;
	opspf->idtoaddress[22][5]=-1107278591;
	opspf->idtoaddress[23][0]=-1107289598;
	opspf->idtoaddress[23][1]=-1107284478;
	opspf->idtoaddress[23][2]=-1107283966;
	opspf->idtoaddress[23][3]=-1107279102;
	opspf->idtoaddress[23][4]=-1107278335;
	opspf->idtoaddress[23][5]=-1107278079;
	opspf->idtoaddress[24][0]=-1107283454;
	opspf->idtoaddress[24][1]=-1107282942;
	opspf->idtoaddress[24][2]=-1107277823;
	opspf->idtoaddress[24][3]=-1107277567;
	opspf->idtoaddress[24][4]=-1107277311;
	opspf->idtoaddress[24][5]=-1107277055;
	opspf->idtoaddress[25][0]=-1107282686;
	opspf->idtoaddress[25][1]=-1107281918;
	opspf->idtoaddress[25][2]=-1107277822;
	opspf->idtoaddress[25][3]=-1107276799;
	opspf->idtoaddress[25][4]=-1107276287;
	opspf->idtoaddress[25][5]=-1107276543;
	opspf->idtoaddress[26][0]=-1107282174;
	opspf->idtoaddress[26][1]=-1107281406;
	opspf->idtoaddress[26][2]=-1107276798;
	opspf->idtoaddress[26][3]=-1107276031;
	opspf->idtoaddress[26][4]=-1107275775;
	opspf->idtoaddress[26][5]=-1107275519;
	opspf->idtoaddress[27][0]=-1107281150;
	opspf->idtoaddress[27][1]=-1107280382;
	opspf->idtoaddress[27][2]=-1107276030;
	opspf->idtoaddress[27][3]=-1107275263;
	opspf->idtoaddress[27][4]=-1107275007;
	opspf->idtoaddress[27][5]=-1107274751;
	opspf->idtoaddress[28][0]=-1107280126;
	opspf->idtoaddress[28][1]=-1107279614;
	opspf->idtoaddress[28][2]=-1107275262;
	opspf->idtoaddress[28][3]=-1107274495;
	opspf->idtoaddress[28][4]=-1107274239;
	opspf->idtoaddress[28][5]=-1107273983;
	opspf->idtoaddress[29][0]=-1107279358;
	opspf->idtoaddress[29][1]=-1107278846;
	opspf->idtoaddress[29][2]=-1107274494;
	opspf->idtoaddress[29][3]=-1107273727;
	opspf->idtoaddress[29][4]=-1107273471;
	opspf->idtoaddress[29][5]=-1107273215;
	opspf->idtoaddress[30][0]=-1107278590;
	opspf->idtoaddress[30][1]=-1107278078;
	opspf->idtoaddress[30][2]=-1107273726;
	opspf->idtoaddress[30][3]=-1107272959;
	opspf->idtoaddress[30][4]=-1107272703;
	opspf->idtoaddress[30][5]=-1107272447;
	opspf->idtoaddress[31][0]=-1107283710;
	opspf->idtoaddress[31][1]=-1107278334;
	opspf->idtoaddress[31][2]=-1107277566;
	opspf->idtoaddress[31][3]=-1107272958;
	opspf->idtoaddress[31][4]=-1107272191;
	opspf->idtoaddress[31][5]=-1107271935;
	opspf->idtoaddress[32][0]=-1107277054;
	opspf->idtoaddress[32][1]=-1107276286;
	opspf->idtoaddress[32][2]=-1107271679;
	opspf->idtoaddress[32][3]=-1107271423;
	opspf->idtoaddress[32][4]=-1107271167;
	opspf->idtoaddress[32][5]=-1107270911;
	opspf->idtoaddress[33][0]=-1107276542;
	opspf->idtoaddress[33][1]=-1107275774;
	opspf->idtoaddress[33][2]=-1107271678;
	opspf->idtoaddress[33][3]=-1107270655;
	opspf->idtoaddress[33][4]=-1107270399;
	opspf->idtoaddress[33][5]=-1107270143;
	opspf->idtoaddress[34][0]=-1107275518;
	opspf->idtoaddress[34][1]=-1107275006;
	opspf->idtoaddress[34][2]=-1107270654;
	opspf->idtoaddress[34][3]=-1107269887;
	opspf->idtoaddress[34][4]=-1107269631;
	opspf->idtoaddress[34][5]=-1107269375;
	opspf->idtoaddress[35][0]=-1107274750;
	opspf->idtoaddress[35][1]=-1107274238;
	opspf->idtoaddress[35][2]=-1107269886;
	opspf->idtoaddress[35][3]=-1107269119;
	opspf->idtoaddress[35][4]=-1107268095;
	opspf->idtoaddress[35][5]=-1107267839;
	opspf->idtoaddress[36][0]=-1107273982;
	opspf->idtoaddress[36][1]=-1107273470;
	opspf->idtoaddress[36][2]=-1107269118;
	opspf->idtoaddress[36][3]=-1107267583;
	opspf->idtoaddress[36][4]=-1107267327;
	opspf->idtoaddress[36][5]=-1107267071;
	opspf->idtoaddress[37][0]=-1107273214;
	opspf->idtoaddress[37][1]=-1107272702;
	opspf->idtoaddress[37][2]=-1107267582;
	opspf->idtoaddress[37][3]=-1107266815;
	opspf->idtoaddress[37][4]=-1107266559;
	opspf->idtoaddress[37][5]=-1107266303;
	opspf->idtoaddress[38][0]=-1107272446;
	opspf->idtoaddress[38][1]=-1107271934;
	opspf->idtoaddress[38][2]=-1107266814;
	opspf->idtoaddress[38][3]=-1107266047;
	opspf->idtoaddress[38][4]=-1107265791;
	opspf->idtoaddress[38][5]=-1107265535;
	opspf->idtoaddress[39][0]=-1107277310;
	opspf->idtoaddress[39][1]=-1107272190;
	opspf->idtoaddress[39][2]=-1107271422;
	opspf->idtoaddress[39][3]=-1107266046;
	opspf->idtoaddress[39][4]=-1107265279;
	opspf->idtoaddress[39][5]=-1107265023;
	opspf->idtoaddress[40][0]=-1107271166;
	opspf->idtoaddress[40][1]=-1107270142;
	opspf->idtoaddress[40][2]=-1107264767;
	opspf->idtoaddress[40][3]=-1107264511;
	opspf->idtoaddress[40][4]=0;
	opspf->idtoaddress[40][5]=0;
	opspf->idtoaddress[41][0]=-1107270398;
	opspf->idtoaddress[41][1]=-1107269630;
	opspf->idtoaddress[41][2]=-1107264766;
	opspf->idtoaddress[41][3]=-1107264255;
	opspf->idtoaddress[41][4]=0;
	opspf->idtoaddress[41][5]=0;
	opspf->idtoaddress[42][0]=-1107269374;
	opspf->idtoaddress[42][1]=-1107268094;
	opspf->idtoaddress[42][2]=-1107264254;
	opspf->idtoaddress[42][3]=-1107263999;
	opspf->idtoaddress[42][4]=0;
	opspf->idtoaddress[42][5]=0;
	opspf->idtoaddress[43][0]=-1107267838;
	opspf->idtoaddress[43][1]=-1107267070;
	opspf->idtoaddress[43][2]=-1107263998;
	opspf->idtoaddress[43][3]=-1107263743;
	opspf->idtoaddress[43][4]=0;
	opspf->idtoaddress[43][5]=0;
	opspf->idtoaddress[44][0]=-1107267326;
	opspf->idtoaddress[44][1]=-1107266558;
	opspf->idtoaddress[44][2]=-1107263742;
	opspf->idtoaddress[44][3]=-1107263487;
	opspf->idtoaddress[44][4]=0;
	opspf->idtoaddress[44][5]=0;
	opspf->idtoaddress[45][0]=-1107266302;
	opspf->idtoaddress[45][1]=-1107265790;
	opspf->idtoaddress[45][2]=-1107263486;
	opspf->idtoaddress[45][3]=-1107263231;
	opspf->idtoaddress[45][4]=0;
	opspf->idtoaddress[45][5]=0;
	opspf->idtoaddress[46][0]=-1107265534;
	opspf->idtoaddress[46][1]=-1107265278;
	opspf->idtoaddress[46][2]=-1107263230;
	opspf->idtoaddress[46][3]=-1107262975;
	opspf->idtoaddress[46][4]=0;
	opspf->idtoaddress[46][5]=0;
	opspf->idtoaddress[47][0]=-1107270910;
	opspf->idtoaddress[47][1]=-1107265022;
	opspf->idtoaddress[47][2]=-1107264510;
	opspf->idtoaddress[47][3]=-1107262974;
	opspf->idtoaddress[47][4]=0;
	opspf->idtoaddress[47][5]=0;

	opspf->linktable[0][0]=1;
	opspf->linktable[0][1]=7;
	opspf->linktable[0][2]=8;
	opspf->linktable[0][3]=15;
	opspf->linktable[0][4]=-1;
	opspf->linktable[0][5]=-1;
	opspf->linktable[1][0]=0;
	opspf->linktable[1][1]=2;
	opspf->linktable[1][2]=8;
	opspf->linktable[1][3]=9;
	opspf->linktable[1][4]=-1;
	opspf->linktable[1][5]=-1;
	opspf->linktable[2][0]=1;
	opspf->linktable[2][1]=3;
	opspf->linktable[2][2]=9;
	opspf->linktable[2][3]=10;
	opspf->linktable[2][4]=-1;
	opspf->linktable[2][5]=-1;
	opspf->linktable[3][0]=2;
	opspf->linktable[3][1]=4;
	opspf->linktable[3][2]=10;
	opspf->linktable[3][3]=11;
	opspf->linktable[3][4]=-1;
	opspf->linktable[3][5]=-1;
	opspf->linktable[4][0]=3;
	opspf->linktable[4][1]=5;
	opspf->linktable[4][2]=11;
	opspf->linktable[4][3]=12;
	opspf->linktable[4][4]=-1;
	opspf->linktable[4][5]=-1;
	opspf->linktable[5][0]=4;
	opspf->linktable[5][1]=6;
	opspf->linktable[5][2]=12;
	opspf->linktable[5][3]=13;
	opspf->linktable[5][4]=-1;
	opspf->linktable[5][5]=-1;
	opspf->linktable[6][0]=5;
	opspf->linktable[6][1]=7;
	opspf->linktable[6][2]=14;
	opspf->linktable[6][3]=13;
	opspf->linktable[6][4]=-1;
	opspf->linktable[6][5]=-1;
	opspf->linktable[7][0]=0;
	opspf->linktable[7][1]=6;
	opspf->linktable[7][2]=15;
	opspf->linktable[7][3]=14;
	opspf->linktable[7][4]=-1;
	opspf->linktable[7][5]=-1;
	opspf->linktable[8][0]=0;
	opspf->linktable[8][1]=1;
	opspf->linktable[8][2]=9;
	opspf->linktable[8][3]=15;
	opspf->linktable[8][4]=16;
	opspf->linktable[8][5]=23;
	opspf->linktable[9][0]=1;
	opspf->linktable[9][1]=2;
	opspf->linktable[9][2]=8;
	opspf->linktable[9][3]=10;
	opspf->linktable[9][4]=16;
	opspf->linktable[9][5]=17;
	opspf->linktable[10][0]=2;
	opspf->linktable[10][1]=3;
	opspf->linktable[10][2]=9;
	opspf->linktable[10][3]=11;
	opspf->linktable[10][4]=18;
	opspf->linktable[10][5]=17;
	opspf->linktable[11][0]=3;
	opspf->linktable[11][1]=4;
	opspf->linktable[11][2]=10;
	opspf->linktable[11][3]=12;
	opspf->linktable[11][4]=18;
	opspf->linktable[11][5]=19;
	opspf->linktable[12][0]=4;
	opspf->linktable[12][1]=5;
	opspf->linktable[12][2]=11;
	opspf->linktable[12][3]=13;
	opspf->linktable[12][4]=19;
	opspf->linktable[12][5]=20;
	opspf->linktable[13][0]=5;
	opspf->linktable[13][1]=12;
	opspf->linktable[13][2]=14;
	opspf->linktable[13][3]=20;
	opspf->linktable[13][4]=21;
	opspf->linktable[13][5]=6;
	opspf->linktable[14][0]=6;
	opspf->linktable[14][1]=13;
	opspf->linktable[14][2]=15;
	opspf->linktable[14][3]=21;
	opspf->linktable[14][4]=22;
	opspf->linktable[14][5]=7;
	opspf->linktable[15][0]=0;
	opspf->linktable[15][1]=8;
	opspf->linktable[15][2]=14;
	opspf->linktable[15][3]=22;
	opspf->linktable[15][4]=23;
	opspf->linktable[15][5]=7;
	opspf->linktable[16][0]=8;
	opspf->linktable[16][1]=9;
	opspf->linktable[16][2]=17;
	opspf->linktable[16][3]=23;
	opspf->linktable[16][4]=31;
	opspf->linktable[16][5]=24;
	opspf->linktable[17][0]=9;
	opspf->linktable[17][1]=10;
	opspf->linktable[17][2]=16;
	opspf->linktable[17][3]=18;
	opspf->linktable[17][4]=24;
	opspf->linktable[17][5]=25;
	opspf->linktable[18][0]=10;
	opspf->linktable[18][1]=11;
	opspf->linktable[18][2]=17;
	opspf->linktable[18][3]=19;
	opspf->linktable[18][4]=26;
	opspf->linktable[18][5]=25;
	opspf->linktable[19][0]=11;
	opspf->linktable[19][1]=12;
	opspf->linktable[19][2]=18;
	opspf->linktable[19][3]=20;
	opspf->linktable[19][4]=26;
	opspf->linktable[19][5]=27;
	opspf->linktable[20][0]=12;
	opspf->linktable[20][1]=13;
	opspf->linktable[20][2]=19;
	opspf->linktable[20][3]=21;
	opspf->linktable[20][4]=27;
	opspf->linktable[20][5]=28;
	opspf->linktable[21][0]=13;
	opspf->linktable[21][1]=14;
	opspf->linktable[21][2]=20;
	opspf->linktable[21][3]=22;
	opspf->linktable[21][4]=28;
	opspf->linktable[21][5]=29;
	opspf->linktable[22][0]=14;
	opspf->linktable[22][1]=15;
	opspf->linktable[22][2]=21;
	opspf->linktable[22][3]=23;
	opspf->linktable[22][4]=29;
	opspf->linktable[22][5]=30;
	opspf->linktable[23][0]=8;
	opspf->linktable[23][1]=15;
	opspf->linktable[23][2]=16;
	opspf->linktable[23][3]=22;
	opspf->linktable[23][4]=31;
	opspf->linktable[23][5]=30;
	opspf->linktable[24][0]=16;
	opspf->linktable[24][1]=17;
	opspf->linktable[24][2]=25;
	opspf->linktable[24][3]=31;
	opspf->linktable[24][4]=39;
	opspf->linktable[24][5]=32;
	opspf->linktable[25][0]=17;
	opspf->linktable[25][1]=18;
	opspf->linktable[25][2]=24;
	opspf->linktable[25][3]=26;
	opspf->linktable[25][4]=32;
	opspf->linktable[25][5]=33;
	opspf->linktable[26][0]=18;
	opspf->linktable[26][1]=19;
	opspf->linktable[26][2]=25;
	opspf->linktable[26][3]=27;
	opspf->linktable[26][4]=33;
	opspf->linktable[26][5]=34;
	opspf->linktable[27][0]=19;
	opspf->linktable[27][1]=20;
	opspf->linktable[27][2]=26;
	opspf->linktable[27][3]=28;
	opspf->linktable[27][4]=34;
	opspf->linktable[27][5]=35;
	opspf->linktable[28][0]=20;
	opspf->linktable[28][1]=21;
	opspf->linktable[28][2]=27;
	opspf->linktable[28][3]=29;
	opspf->linktable[28][4]=35;
	opspf->linktable[28][5]=36;
	opspf->linktable[29][0]=21;
	opspf->linktable[29][1]=22;
	opspf->linktable[29][2]=28;
	opspf->linktable[29][3]=30;
	opspf->linktable[29][4]=36;
	opspf->linktable[29][5]=37;
	opspf->linktable[30][0]=22;
	opspf->linktable[30][1]=23;
	opspf->linktable[30][2]=29;
	opspf->linktable[30][3]=31;
	opspf->linktable[30][4]=37;
	opspf->linktable[30][5]=38;
	opspf->linktable[31][0]=16;
	opspf->linktable[31][1]=23;
	opspf->linktable[31][2]=24;
	opspf->linktable[31][3]=30;
	opspf->linktable[31][4]=39;
	opspf->linktable[31][5]=38;
	opspf->linktable[32][0]=24;
	opspf->linktable[32][1]=25;
	opspf->linktable[32][2]=33;
	opspf->linktable[32][3]=39;
	opspf->linktable[32][4]=40;
	opspf->linktable[32][5]=47;
	opspf->linktable[33][0]=25;
	opspf->linktable[33][1]=26;
	opspf->linktable[33][2]=32;
	opspf->linktable[33][3]=34;
	opspf->linktable[33][4]=41;
	opspf->linktable[33][5]=40;
	opspf->linktable[34][0]=26;
	opspf->linktable[34][1]=27;
	opspf->linktable[34][2]=33;
	opspf->linktable[34][3]=35;
	opspf->linktable[34][4]=41;
	opspf->linktable[34][5]=42;
	opspf->linktable[35][0]=27;
	opspf->linktable[35][1]=28;
	opspf->linktable[35][2]=34;
	opspf->linktable[35][3]=36;
	opspf->linktable[35][4]=42;
	opspf->linktable[35][5]=43;
	opspf->linktable[36][0]=28;
	opspf->linktable[36][1]=29;
	opspf->linktable[36][2]=35;
	opspf->linktable[36][3]=37;
	opspf->linktable[36][4]=44;
	opspf->linktable[36][5]=43;
	opspf->linktable[37][0]=29;
	opspf->linktable[37][1]=30;
	opspf->linktable[37][2]=36;
	opspf->linktable[37][3]=38;
	opspf->linktable[37][4]=44;
	opspf->linktable[37][5]=45;
	opspf->linktable[38][0]=30;
	opspf->linktable[38][1]=31;
	opspf->linktable[38][2]=37;
	opspf->linktable[38][3]=39;
	opspf->linktable[38][4]=45;
	opspf->linktable[38][5]=46;
	opspf->linktable[39][0]=24;
	opspf->linktable[39][1]=31;
	opspf->linktable[39][2]=32;
	opspf->linktable[39][3]=38;
	opspf->linktable[39][4]=46;
	opspf->linktable[39][5]=47;
	opspf->linktable[40][0]=32;
	opspf->linktable[40][1]=33;
	opspf->linktable[40][2]=41;
	opspf->linktable[40][3]=47;
	opspf->linktable[40][4]=-1;
	opspf->linktable[40][5]=-1;
	opspf->linktable[41][0]=33;
	opspf->linktable[41][1]=34;
	opspf->linktable[41][2]=40;
	opspf->linktable[41][3]=42;
	opspf->linktable[41][4]=-1;
	opspf->linktable[41][5]=-1;
	opspf->linktable[42][0]=34;
	opspf->linktable[42][1]=35;
	opspf->linktable[42][2]=41;
	opspf->linktable[42][3]=43;
	opspf->linktable[42][4]=-1;
	opspf->linktable[42][5]=-1;
	opspf->linktable[43][0]=35;
	opspf->linktable[43][1]=36;
	opspf->linktable[43][2]=42;
	opspf->linktable[43][3]=44;
	opspf->linktable[43][4]=-1;
	opspf->linktable[43][5]=-1;
	opspf->linktable[44][0]=36;
	opspf->linktable[44][1]=37;
	opspf->linktable[44][2]=43;
	opspf->linktable[44][3]=45;
	opspf->linktable[44][4]=-1;
	opspf->linktable[44][5]=-1;
	opspf->linktable[45][0]=37;
	opspf->linktable[45][1]=38;
	opspf->linktable[45][2]=44;
	opspf->linktable[45][3]=46;
	opspf->linktable[45][4]=-1;
	opspf->linktable[45][5]=-1;
	opspf->linktable[46][0]=38;
	opspf->linktable[46][1]=39;
	opspf->linktable[46][2]=45;
	opspf->linktable[46][3]=47;
	opspf->linktable[46][4]=-1;
	opspf->linktable[46][5]=-1;
	opspf->linktable[47][0]=32;
	opspf->linktable[47][1]=39;
	opspf->linktable[47][2]=40;
	opspf->linktable[47][3]=46;
	opspf->linktable[47][4]=-1;
	opspf->linktable[47][5]=-1;

	
	opspf->linknode[0]=1;
	opspf->linknode[1]=0;
	opspf->linknode[2]=0;
	opspf->linknode[3]=0;
	opspf->linknode[4]=1;
	opspf->linknode[5]=1;
	opspf->linknode[6]=1;
	opspf->linknode[7]=2;
	opspf->linknode[8]=2;
	opspf->linknode[9]=2;
	opspf->linknode[10]=3;
	opspf->linknode[11]=3;
	opspf->linknode[12]=3;
	opspf->linknode[13]=4;
	opspf->linknode[14]=4;
	opspf->linknode[15]=4;
	opspf->linknode[16]=5;
	opspf->linknode[17]=5;
	opspf->linknode[18]=5;
	opspf->linknode[19]=6;
	opspf->linknode[20]=6;
	opspf->linknode[21]=8;
	opspf->linknode[22]=8;
	opspf->linknode[23]=8;
	opspf->linknode[24]=8;
	opspf->linknode[25]=9;
	opspf->linknode[26]=9;
	opspf->linknode[27]=9;
	opspf->linknode[28]=10;
	opspf->linknode[29]=10;
	opspf->linknode[30]=10;
	opspf->linknode[31]=11;
	opspf->linknode[32]=11;
	opspf->linknode[33]=11;
	opspf->linknode[34]=12;
	opspf->linknode[35]=12;
	opspf->linknode[36]=12;
	opspf->linknode[37]=13;
	opspf->linknode[38]=13;
	opspf->linknode[39]=13;
	opspf->linknode[40]=14;
	opspf->linknode[41]=14;
	opspf->linknode[42]=14;
	opspf->linknode[43]=15;
	opspf->linknode[44]=15;
	opspf->linknode[45]=16;
	opspf->linknode[46]=16;
	opspf->linknode[47]=16;
	opspf->linknode[48]=16;
	opspf->linknode[49]=17;
	opspf->linknode[50]=17;
	opspf->linknode[51]=17;
	opspf->linknode[52]=18;
	opspf->linknode[53]=18;
	opspf->linknode[54]=18;
	opspf->linknode[55]=19;
	opspf->linknode[56]=19;
	opspf->linknode[57]=19;
	opspf->linknode[58]=20;
	opspf->linknode[59]=20;
	opspf->linknode[60]=20;
	opspf->linknode[61]=21;
	opspf->linknode[62]=21;
	opspf->linknode[63]=21;
	opspf->linknode[64]=22;
	opspf->linknode[65]=22;
	opspf->linknode[66]=22;
	opspf->linknode[67]=23;
	opspf->linknode[68]=23;
	opspf->linknode[69]=24;
	opspf->linknode[70]=24;
	opspf->linknode[71]=24;
	opspf->linknode[72]=24;
	opspf->linknode[73]=25;
	opspf->linknode[74]=25;
	opspf->linknode[75]=26;
	opspf->linknode[76]=25;
	opspf->linknode[77]=26;
	opspf->linknode[78]=26;
	opspf->linknode[79]=27;
	opspf->linknode[80]=27;
	opspf->linknode[81]=27;
	opspf->linknode[82]=28;
	opspf->linknode[83]=28;
	opspf->linknode[84]=28;
	opspf->linknode[85]=29;
	opspf->linknode[86]=29;
	opspf->linknode[87]=29;
	opspf->linknode[88]=30;
	opspf->linknode[89]=30;
	opspf->linknode[90]=30;
	opspf->linknode[91]=31;
	opspf->linknode[92]=31;
	opspf->linknode[93]=32;
	opspf->linknode[94]=32;
	opspf->linknode[95]=32;
	opspf->linknode[96]=32;
	opspf->linknode[97]=33;
	opspf->linknode[98]=33;
	opspf->linknode[99]=33;
	opspf->linknode[100]=34;
	opspf->linknode[101]=34;
	opspf->linknode[102]=34;
	opspf->linknode[103]=35;
	opspf->linknode[104]=35;
	opspf->linknode[105]=35;
	opspf->linknode[106]=36;
	opspf->linknode[107]=36;
	opspf->linknode[108]=36;
	opspf->linknode[109]=37;
	opspf->linknode[110]=37;
	opspf->linknode[111]=37;
	opspf->linknode[112]=38;
	opspf->linknode[113]=38;
	opspf->linknode[114]=38;
	opspf->linknode[115]=39;
	opspf->linknode[116]=39;
	opspf->linknode[117]=40;
	opspf->linknode[118]=40;
	opspf->linknode[119]=41;
	opspf->linknode[120]=42;
	opspf->linknode[121]=43;
	opspf->linknode[122]=44;
	opspf->linknode[123]=45;
	opspf->linknode[124]=46;
	opspf->linknode[125]=7;
	opspf->linknode[126]=7;
	opspf->linknode[127]=13;
	 
	opspf->linkid[0]=1;
	opspf->linkid[1]=2;
	opspf->linkid[2]=3;
	opspf->linkid[3]=4;
	opspf->linkid[4]=5;
	opspf->linkid[5]=6;
	opspf->linkid[6]=7;
	opspf->linkid[7]=8;
	opspf->linkid[8]=9;
	opspf->linkid[9]=10;
	opspf->linkid[10]=11;
	opspf->linkid[11]=12;
	opspf->linkid[12]=13;
	opspf->linkid[13]=14;
	opspf->linkid[14]=15;
	opspf->linkid[15]=16;
	opspf->linkid[16]=17;
	opspf->linkid[17]=18;
	opspf->linkid[18]=19;
	opspf->linkid[19]=20;
	opspf->linkid[20]=22;
	opspf->linkid[21]=23;
	opspf->linkid[22]=24;
	opspf->linkid[23]=25;
	opspf->linkid[24]=26;
	opspf->linkid[25]=27;
	opspf->linkid[26]=28;
	opspf->linkid[27]=29;
	opspf->linkid[28]=30;
	opspf->linkid[29]=31;
	opspf->linkid[30]=32;
	opspf->linkid[31]=33;
	opspf->linkid[32]=34;
	opspf->linkid[33]=35;
	opspf->linkid[34]=36;
	opspf->linkid[35]=37;
	opspf->linkid[36]=38;
	opspf->linkid[37]=39;
	opspf->linkid[38]=40;
	opspf->linkid[39]=41;
	opspf->linkid[40]=42;
	opspf->linkid[41]=43;
	opspf->linkid[42]=44;
	opspf->linkid[43]=45;
	opspf->linkid[44]=46;
	opspf->linkid[45]=47;
	opspf->linkid[46]=48;
	opspf->linkid[47]=49;
	opspf->linkid[48]=50;
	opspf->linkid[49]=51;
	opspf->linkid[50]=52;
	opspf->linkid[51]=53;
	opspf->linkid[52]=54;
	opspf->linkid[53]=55;
	opspf->linkid[54]=56;
	opspf->linkid[55]=57;
	opspf->linkid[56]=58;
	opspf->linkid[57]=59;
	opspf->linkid[58]=61;
	opspf->linkid[59]=62;
	opspf->linkid[60]=63;
	opspf->linkid[61]=64;
	opspf->linkid[62]=65;
	opspf->linkid[63]=66;
	opspf->linkid[64]=67;
	opspf->linkid[65]=68;
	opspf->linkid[66]=69;
	opspf->linkid[67]=70;
	opspf->linkid[68]=71;
	opspf->linkid[69]=72;
	opspf->linkid[70]=73;
	opspf->linkid[71]=74;
	opspf->linkid[72]=75;
	opspf->linkid[73]=76;
	opspf->linkid[74]=78;
	opspf->linkid[75]=79;
	opspf->linkid[76]=77;
	opspf->linkid[77]=80;
	opspf->linkid[78]=81;
	opspf->linkid[79]=82;
	opspf->linkid[80]=83;
	opspf->linkid[81]=84;
	opspf->linkid[82]=85;
	opspf->linkid[83]=86;
	opspf->linkid[84]=87;
	opspf->linkid[85]=88;
	opspf->linkid[86]=89;
	opspf->linkid[87]=90;
	opspf->linkid[88]=91;
	opspf->linkid[89]=92;
	opspf->linkid[90]=93;
	opspf->linkid[91]=94;
	opspf->linkid[92]=95;
	opspf->linkid[93]=96;
	opspf->linkid[94]=97;
	opspf->linkid[95]=98;
	opspf->linkid[96]=99;
	opspf->linkid[97]=100;
	opspf->linkid[98]=101;
	opspf->linkid[99]=102;
	opspf->linkid[100]=103;
	opspf->linkid[101]=104;
	opspf->linkid[102]=105;
	opspf->linkid[103]=106;
	opspf->linkid[104]=110;
	opspf->linkid[105]=111;
	opspf->linkid[106]=112;
	opspf->linkid[107]=113;
	opspf->linkid[108]=114;
	opspf->linkid[109]=115;
	opspf->linkid[110]=116;
	opspf->linkid[111]=117;
	opspf->linkid[112]=118;
	opspf->linkid[113]=119;
	opspf->linkid[114]=120;
	opspf->linkid[115]=121;
	opspf->linkid[116]=122;
	opspf->linkid[117]=123;
	opspf->linkid[118]=124;
	opspf->linkid[119]=125;
	opspf->linkid[120]=126;
	opspf->linkid[121]=127;
	opspf->linkid[122]=128;
	opspf->linkid[123]=129;
	opspf->linkid[124]=130;
	opspf->linkid[125]=21;
	opspf->linkid[126]=131;
	opspf->linkid[127]=132;
}

bool isOppositeSatellite(
	NodeAddress selfAddress,
	NodeAddress sourceAddress,
	int srcSatelliteId,
	OpspfData* opspfData)
{
	printf("---->In isOppsiteSatellite.\n");

	int selfAreaId = selfAddress>>24;
	int srcAreaId = sourceAddress>>24;
	if(selfAreaId == srcAreaId)//Get the pkt of selfSatallite.
	{
		opspfData->selfSatelliteId = srcSatelliteId;//update satellite info at user side.
	}
	else if(opspfData->selfSatelliteId)//Get the pkt of other satellites.
	{
		//Get selfOrbitState
		int selfOrbitState = NORMAL_ORBIT;
		for(int i = 0; (i < 2) && (selfOrbitState == NORMAL_ORBIT); i++)
		{
			for(int j = 0; j < 8; j++)
			{
				if(oppSateId[i][j] == opspfData->selfSatelliteId)
				{
					selfOrbitState = i; // 0 for satellite 1-8(RIGHT_ORBIT), 1 for satellite 41-48(LEFT_ORBIT)
					break;
				}
			}
		}

		//SelfOrbitState is not normal, find that whther srcSatellite is oppsite with selfSatallite.	
		if(selfOrbitState != NORMAL_ORBIT)
		{
			int oppOrbitIndex = selfOrbitState ? RIGHT_ORBIT : LEFT_ORBIT;
			for(int i = 0; i < 8; i++)
			{
				if(oppSateId[oppOrbitIndex][i] == srcSatelliteId)
				{
					//srcSatellite is opposite, return true.
					return true;
				}
			}
		}	
	}

	//Normal condition, or selfSatelliteId == 0,
	//which means that the selfSatellite has not come.
	return false;
}
