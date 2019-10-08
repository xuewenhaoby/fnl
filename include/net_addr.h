#ifndef __NET_ADDR_H__
#define __NET_ADDR_H__

#include <iostream>
using namespace std;

string IpStr(int addr);
string IpStr(int addr,int maskSize);
string BIpStr(int addr,int maskSize);

#endif