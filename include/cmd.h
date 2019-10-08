#ifndef __CMD_H__
#define __CMD_H__

#include <iostream>
#include <unistd.h>
using namespace std;

inline string cmd(){ return ""; };
template <typename T, typename... Args> string cmd(T t, Args... funcRest);
template <typename... Args> string cmd(int i, Args...funcRest);

template <typename T, typename... Args> 
string cmd(T t, Args... funcRest)
{
    return t + cmd(funcRest...);
}
template <typename... Args> 
string cmd(int i, Args...funcRest)
{
    return to_string(i) + cmd(funcRest...);
}

inline void run(string command){system(command.c_str());}
inline void run_q(string command){system(cmd(command," 1> /dev/null 2> /dev/null").c_str());}

void add_SatBridge(int id);
void del_SatBridge(int id);

void add_HostBridge(int id);
void del_HostBridge(int id);

void add_HostNs(int id, bool real, int addr, int mask);
void del_HostNs(int id);

void change_DevMaster(int id, int oldSbrId, int newbrSId);

void CloseBgFlows();

void host_exec(int id, string command);

#endif