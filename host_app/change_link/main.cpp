#include <iostream>
#include <unistd.h>
using namespace std;

string cmd();
template <typename T, typename... Args> string cmd(T t, Args... funcRest);
template <typename... Args> string cmd(int i, Args...funcRest);

void change_link(int uId,int sId,string uIp);

int main(int argc,char* argv[])
{
    if(argc != 4)
        cout << "Wrong Input!" << endl;
    else
        change_link(stoi(argv[1]),stoi(argv[2]),argv[3]);
    return 0;
}

string cmd(){ return ""; }
template <typename T, typename... Args> string cmd(T t, Args... funcRest){return t + cmd(funcRest...);}
template <typename... Args> string cmd(int i, Args...funcRest){return to_string(i) + cmd(funcRest...);}

void change_link(int uId,int sId,string uIp)
{
    // cout << uId << "," << sId << "," << uIp << "," << cmd("host",uId) << endl;
    string br = "pbr";
    system(cmd("ovs-ofctl del-flows ",br," \"ip,nw_src=",uIp,"\"").c_str());
    system(cmd("ovs-ofctl del-flows ",br," \"ip,nw_dst=",uIp,"\"").c_str());
    system(cmd("ovs-ofctl add-flow ",br," \"in_port=",uId+100,",ip,nw_dst=100.0.0.1,priority=",10,"actions=output:",sId,"\"").c_str());
    system(cmd("ovs-ofctl add-flow ",br," \"in_port=",uId+100,",priority=",5,"actions=drop\"").c_str());
    system(cmd("ovs-ofctl add-flow ",br," \"ip,nw_dst=",uIp,",nw_src=100.0.0.1,,priority=",10,"actions=output:",uId+100,"\"").c_str());
    system(cmd("ovs-ofctl add-flow ",br," \"in_port=",100,",ip,nw_src=",uIp,",priority=",0,",actions=output:",sId,"\"").c_str());
    system(cmd("ovs-ofctl add-flow ",br," \"ip,nw_dst=",uIp,",priority=",0,",actions=output:",100,"\"").c_str());

    system(cmd("ip netns exec ",cmd("host",uId)," ./send register").c_str());
}