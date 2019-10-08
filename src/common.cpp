#include "common.h"

SatNode Sats[SAT_NUM];
vector<HostNode> Users;
StaticTopo G;
int S_A[SAT_NUM] = {0};
int S_A_old[SAT_NUM] = {0};
// int A_S[SAT_NUM] = {0};
ThreadPool Pool(POOL_SIZE);
set<int> HostPhyNodes;
set<int> SatPhyNodes;
/*************************argvs*************************/
bool enablePhysical = DEFAULT_PHYSICAL;
bool enableInitPhy = DEFAULT_INIT_PHY;
bool enableFinaPhy = DEFAULT_FINA_PHY;
bool enablePrintSatLinkStat = DEFAULT_PRINT_SATLINK_STAT;
bool enablePrintHostLinkStat = DEFAULT_PRINT_HOSTLINK_STAT;
bool enableBgFlows = DEFAULT_BGFLOWS;
/************************end argvs**********************/
set<double> host_lat;
set<double> host_lon;
set<double> host_satId;