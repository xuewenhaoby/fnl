#include "common.h"
#include "task.h"
#include "message.h"
#include "timer.h"
#include "calculate.h"
#include <typeinfo>
#include <unistd.h>

void *func1(void *arg)
{
	return 0;
}

void HandleNodeEvent(void *arg)
{
	Message *msg = (Message *)arg;
	Node *node = (Node *)msg->getNode();
	
	switch(msg->getEventType())
	{
		case MSG_NODE_Initialize:
		{
			node->nodeInitialize();
			break;
		}
		case MSG_NODE_Finalize:
		{
			node->nodeFinalize();
			break;
		}
		case MSG_NODE_UpdateLink:
		{
			node->updateLink();
			break;
		}
		case MSG_NODE_UpdatePos:
		{
			node->updatePos();
			break;
		}
		case MSG_HOST_BgFlows:
		{
			HandleHostEvent(arg);
			break;
		}
		case MSG_NODE_Test:
		{
			if(typeid(*node) == typeid(SatNode)){
				cout << "SatNode!" << endl; 
			}
			else if(typeid(*node) == typeid(HostNode)){
				cout << "HostNode!" << endl; 
			}
			break;
		}
		default:{
			break;
		}
	}
	MESSAGE_FREE(msg);
}

void HandleTimerEvent(void *arg)
{
	Message *msg = (Message *)arg;

	switch(msg->getEventType())
	{
		case MSG_TIME_Timer:
		{
			// Update-Sats-pos should be done first.
			// Other tasks should be assigned after update-Sats-pos.
			for(int i = 0; i < SAT_NUM; i++){
				SAT(i).updatePos();
			}
			// Update S_A table should be done second.
			UpdateSATableByFile();

			// for(int i = 0; i < SAT_NUM; i++){
			// 	Pool.addTask(new NodeTask(new Message(
			// 		&Sats[i],MSG_NODE_UpdateLink)));
			// }
			for(int i = 0; i < SAT_NUM; i++){
				SAT(i).updateLink();
			}
			
			// for(int i = 0; i < HOST_NUM; i++){
			// 	Pool.addTask(new NodeTask(new Message(
			// 		&Users[i],MSG_NODE_UpdateLink)));
			// }
			// for(int i = 0; i < HOST_NUM; i++){
			// 	USER(i).updateLink();
			// }
			break;
		}
		default:{
			break;
		}
	}
	MESSAGE_FREE(msg);
}

void HandleHostEvent(void *arg)
{
	Message *msg = (Message *)arg;
	HostNode *host = (HostNode *)msg->getNode();

	switch(msg->getEventType())
	{
		case MSG_HOST_BgFlows:
		{
			host->iperf_client();
			break;
		}
		default:{
			break;
		}
	}
}

void HandleSatEvent(void *arg)
{

}