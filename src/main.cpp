#include "timer.h"
#include "common.h"
#include "argv.h"
#include "debug.h"
#include "cmd.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>
using namespace std;

int main(int argc,char *argv[])
{
	//*

	ParseArgv(argc,argv);
	cout<<"Please input the number of host from 1--10"<<endl;
	int host_num =-1;
	string h_s;
	do{
		
		cin>>h_s;
		
		host_num=atoi(h_s.c_str());
		if(host_num<1||host_num>10)
		{
			cout<<"Please input the number again"<<endl;
		}

	}while((host_num<1)||(host_num>=10));

	TopoInitialize(n);
	
	cout << "Waiting for input ... <'start','stop','exit','configure'>" << endl;
	
	string s;
	do{
		cin >> s;
		if(s == "start"){
			StartTimer();
		}
		else if(s == "stop" || s == "exit"){
			EndTimer();	
		}
		else if(s == "stop" || s == "exit"){
			EndTimer();	
			Hst
		}
		else{
			cout << "Wrong input! <'start','stop','exit'>" << endl;
		}
	}while(s != "exit");
	
	TopoFinalize(n);
	//*/
	
	return 0;
}
