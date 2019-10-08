#include "file.h"
#include "common.h"
#include "calculate.h"
#include <cstring>
#include <fstream>

void ReadIslFile(string file_name)
{
	memset(&G,0,sizeof(StaticTopo));
	ifstream in;
	in.open(file_name);
	string s;
	while (getline(in, s)){
		int p0 = s.find(",", 0);
		string sub = s.substr(1, p0);
		int id = atoi(sub.c_str());
		G.isl[id - 1].linkId = id;

		int * value = (int*) G.isl[id - 1].endpoint;
		for (int i = 0; i < 2; i++){
			int p1 = s.find(":",p0);
			sub = s.substr(p0 + 1, p1 - p0 - 1);
			*value = atoi(sub.c_str());

			int p2 = s.find("|", p1);
			sub = s.substr(p1 + 1, p2 - p1 - 1);
			*(value+1) = atoi(sub.c_str());
			
			int p3 = s.find(",", p2);
			sub = s.substr(p2 + 1, p3 - p2 - 1);
			*(value+2) = atoi(sub.c_str());

			p0 = p3;
			value += 3;
		}
		value = NULL;

		int pos2 = s.find("]");
		sub = s.substr(p0 + 1, pos2 - p0 - 1);
		G.isl[id - 1].weight = atoi(sub.c_str());

		G.isl[id - 1].subnetIp = G.isl[id - 1].endpoint[0].ip & 0xffffff00;
	}
	in.close();
}

NodePos ReadLocFile(int id,int time)
{
	ifstream fileIn(MOBILITY_FILE, ios_base::binary);

	int t[2];
	int startTime[2];
	Coord location[2];
	t[0] = time;
	t[1] = t[0] + 1;
	int v = id-1;
	for(int i = 0; i < 2; i++)
	{
		startTime[i] = v * DATA_SIZE + t[i] % DATA_SIZE;
		fileIn.seekg(sizeof(Coord) * startTime[i], ios_base::beg);
		fileIn.read((char *)(location + i), sizeof(Coord));
	}
	NodePos pos;
	memcpy(& pos.loc, location, sizeof(Coord));
	pos.isNorth = (location[0].lat < location[1].lat);

	fileIn.close();
	return pos;
}

void ReadSAFile(int t)
{
	ifstream fin(SA_FILE,ios::binary);

	fin.seekg(sizeof(S_A)*t, ios_base::beg);
	fin.read((char*)S_A,sizeof(S_A));
	
	fin.close();	
}

void WriteSAFile()
{
	cout << "Start writing 'SA.dat'." << endl;	
	ofstream fout("../topofile/SA.dat",ios::binary);
	for(int t = 0; t < DATA_SIZE; t++){
		for(int i = 0; i < SAT_NUM; i++){
			SAT(i).setPos(ReadLocFile(ID(i),t));
		}
		UpdateSATable();
		fout.write((char*)S_A,sizeof(S_A));
	}
	fout.close();
	cout << "Finished." << endl;	
}