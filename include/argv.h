#ifndef __ARGV_H__
#define __ARGV_H__

#include <map>
#include <iostream>
using namespace std;

typedef map<string,string> StringMap;
typedef pair<string,string> StringPair;

#define BOOL_OPT_NUM 6
#define SET_OPT_NUM 2
#define SET_HOST_NUM 3


template <typename T>
struct DataSet{
	string s;
	T & opt;
};

void ParseArgv(int argc,char* argv[]);
void HostArgv(string h_s_file);
#endif