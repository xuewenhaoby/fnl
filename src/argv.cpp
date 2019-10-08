#include "argv.h"
#include "common.h"

#include <string>
#include <stdlib.h>


void ParseArgv(int argc,char* argv[])
{
	StringMap m;
	for(int i = 1; i < argc; i++){
		string s = argv[i];
		int p0 = s.find_last_of("-");
		int p1 = s.find_last_of("=");
		if(p0 == -1 || p1 == -1){
			cout << "Wrong arg input!" << endl;
		}
		else{
			string opt = s.substr(p0+1,p1-p0-1);
			cout<<opt<<endl;
			string val = s.substr(p1+1,s.size()-1-p1);
			cout<<val<<endl;
			m.insert(StringPair(opt,val));
		}
	}

	cout << "/*********************************/" << endl;

	DataSet<bool> boolArgs[BOOL_OPT_NUM] = {
		{"physical",enablePhysical},
		{"printSatLinkStat",enablePrintSatLinkStat},
		{"printHostLinkStat",enablePrintHostLinkStat},
		{"initPhy",enableInitPhy},
		{"finaPhy",enableFinaPhy},
		{"bgFlows",enableBgFlows}
	};
	for(int i = 0; i < BOOL_OPT_NUM; i++){
		if(m.count(boolArgs[i].s)){
			if(m[boolArgs[i].s] == "true") boolArgs[i].opt = true;
			else if(m[boolArgs[i].s] == "false") boolArgs[i].opt = false;
			else cout << "Wrong value for " << boolArgs[i].s << ",use default." << endl;
		}
		cout << boolArgs[i].s << "=" << boolArgs[i].opt << endl;
	}

	DataSet<set<int>> setArgs[SET_OPT_NUM] = {
		{"HostPhyNodes",HostPhyNodes},
		{"SatPhyNodes",SatPhyNodes},
	};
	for(int i = 0; i < SET_OPT_NUM; i++){
		if(m.count(setArgs[i].s)){
			string nodeStr = m[setArgs[i].s];
			cout << setArgs[i].s << "=" << nodeStr;
			nodeStr.back() = ',';
			int p0 = 1,p1 = 1;
			while((p1 = nodeStr.find(',',p0)) != -1){
				setArgs[i].opt.insert(stoi(nodeStr.substr(p0,p1-p0)));
				p0 = p1+1;
			}
			cout << endl;
		}
	}

		
	cout << "/*********************************/" << endl;
}
void HostArgv(string h_s_file)
{

	StringMap m;
	//for(int i = 1; i < argc; i++){
		string s = h_s_file;
		int p0 = s.find_last_of("host");
		int p1 = s.find_last_of("-");
		int p2 = s.find_last_of("=");
		if(p0== -1 || p1 == -1 || p2 == -1){
			cout << "Wrong arg input!" << endl;
		}
		else{
			string hostNo = s.substr(p0+1,p1-p0-1);
			cout<<"host"<<hostNo<<endl;
			string opt = s.substr(p1+1,p2-p1-1);
			cout<<opt<<endl;
			string val = s.substr(p2+1,s.size()-1-p2);
			cout<<val<<endl;
			m.insert(StringPair(opt,val));
		}
		
		DataSet<set<double>> setHostArgs[SET_HOST_NUM] = {
			{"host_lat" ,  host_lat},
			{"host_lon" ,  host_lon},
			//{"host_IP"  ,  host_IP},
			{"host_satId", host_satId},
		};
		for(int i = 0; i < SET_HOST_NUM; i++){
			if(m.count(setHostArgs[i].s)){
				string nodeStr = m[setHostArgs[i].s];
				cout << setHostArgs[i].s << "=" << nodeStr;
				nodeStr.back() = ',';
				int p0 = 1,p1 = 1;
				while((p1 = nodeStr.find(',',p0)) != -1){
					setHostArgs[i].opt.insert(stoi(nodeStr.substr(p0,p1-p0)));
					p0 = p1+1;
				}
				cout << endl;
			}
		}
	}
//}