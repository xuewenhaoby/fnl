#ifndef __COMMON_H__
#define __COMMOM_H__

#include "node.h"
#include "thread_pool.h"
#include <set>

#define DEFAULT_PHYSICAL true
#define DEFAULT_INIT_PHY true
#define DEFAULT_FINA_PHY true
#define DEFAULT_PRINT_SATLINK_STAT true
#define DEFAULT_PRINT_HOSTLINK_STAT true
#define DEFAULT_BGFLOWS false

inline int ID(int index){return index+1;}
inline int INDEX(int id){return id-1;}

extern SatNode Sats[SAT_NUM];
extern vector<HostNode> Users;
extern StaticTopo G;
extern int S_A[SAT_NUM];
extern int S_A_old[SAT_NUM];
// extern int A_S[SAT_NUM];
extern ThreadPool Pool;
extern set<int> HostPhyNodes;
extern set<int> SatPhyNodes;


inline SatNode & SAT(int index){return Sats[index];}
inline HostNode & USER(int index){return Users[index];}

/*************************argvs*************************/
extern bool enablePhysical;
extern bool enableInitPhy;
extern bool enableFinaPhy;
extern bool enablePrintSatLinkStat;
extern bool enablePrintHostLinkStat;
extern bool enableBgFlows;
/************************end argvs**********************/
extern set<double> host_lat;
extern set<double> host_lon;
extern set<double> host_satId;
//extern int    host_IP;

#endif