#ifndef __FILE_H__
#define __FILE_H__

#include "topo.h"
#include <iostream>
using namespace std;

#define MOBILITY_FILE "../topofile/orbit.bin"
//The size of mobility data : 24*60*60+1 24 hours data per second
#define DATA_SIZE 86401 
#define ISL_FILE "../topofile/linktable.txt"
#define SA_FILE "../topofile/SA.dat"

void ReadIslFile(string fileName);

NodePos ReadLocFile(int id,int time);

//void ReadLocFile(Coord mat[48], int t);

void ReadSAFile(int t);

void WriteSAFile();

#endif