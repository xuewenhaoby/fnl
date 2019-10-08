#include "net_addr.h"
#include "cmd.h"
#include "common.h"

string IpStr(int addr)
{
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	return cmd(a1,".",a2,".",a3,".",a4);
}

string IpStr(int addr,int maskSize)
{
	return cmd(IpStr(addr),"/",maskSize);
}

string BIpStr(int addr,int maskSize)
{
	int tmp = 0;
	for(int i = 0; i < 32-maskSize;i++){
		tmp <<= 1;
		tmp |= 0x00000001;
	}
	addr |= tmp;
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	return cmd(a1,".",a2,".",a3,".",a4);
}