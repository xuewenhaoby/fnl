#include "calculate.h"
#include "common.h"
#include "file.h"
#include "timer.h"
#include <cmath>
#include <cstring>

int GetForeSatIndex(int idx,int num)
{
	int obtIdx = GetOrbitIndex(idx);
	int sIdx = GetSatIndexInOrbit(idx);
	return obtIdx*TOPO_N + (sIdx+num+TOPO_N)%TOPO_N;
}

int GetRearSatIndex(int idx, int num)
{
	int obtIdx = GetOrbitIndex(idx);
	int sIdx = GetSatIndexInOrbit(idx);
	return obtIdx*TOPO_N + (sIdx-num+TOPO_N)%TOPO_N;
}

int GetSideSatIndex(int idx,bool isEast,bool isNorth)
{
	int obtIdx = GetOrbitIndex(idx);
	int sIdx = GetSatIndexInOrbit(idx);
	int v = -1;

	if(obtIdx == 0){
		if(isEast){
			if(isNorth)
				v = (obtIdx+1)*TOPO_N + (sIdx-1+TOPO_N)%TOPO_N;
			else
				v = (obtIdx+1)*TOPO_N + sIdx;
		}
	}else if(obtIdx == 5){
		if(!isEast){
			if(isNorth)
				v = (obtIdx-1)*TOPO_N + (sIdx+1)%TOPO_N;
			else
				v = (obtIdx-1)*TOPO_N + sIdx;			
		}
	}else{
		if(isEast){
			if(isNorth)
				v = (obtIdx+1)*TOPO_N + (sIdx+7)%TOPO_N;
			else
				v = (obtIdx+1)*TOPO_N + sIdx;
		}else{
			if(isNorth)
				v = (obtIdx-1)*TOPO_N + (sIdx+1)%TOPO_N;
			else
				v = (obtIdx-1)*TOPO_N + sIdx;	
		}
	}

	return v;
}

NodePos RandomPos()
{
	srand((unsigned)time(NULL));
	Coord loc = {RANDOM(-90,90),RANDOM(-180,180)};
	NodePos pos = {loc,false};
	return pos;	
}

double CalculateDistance(Coord x, Coord y)
{
	double wx = RADIAN(x.lat);
	double wy = RADIAN(y.lat);
	double a = wx - wy;
	double b = RADIAN(x.lon) - RADIAN(y.lon);
	double distance = pow(sin(a/2),2) + cos(wx)*cos(wy)*pow(sin(b/2),2);
	return 2*EARTH_RADIUS*asin(sqrt(distance));
}

void UpdateSATableByFile()
{
	// save the state of old time 
	memcpy(S_A_old, S_A, sizeof(S_A));
	ReadSAFile(GetTime());	
}

void UpdateSATable()
{
	// save the state of old time 
	memcpy(S_A_old, S_A, sizeof(S_A));
	// Fix longtitude
	
	// Set basement orbit-area map
	int dif = 0;
	for(int i = 0; i < SAT_NUM; i++){
		NodePos pos = SAT(i).getPos();
		FixLon(pos.loc, pos.isNorth);
		int areaIdx = GetAreaIndex(pos.loc);
		if(areaIdx%4 == 0 && areaIdx < 4*TOPO_M){
			dif = GetOrbitIndex(i) - areaIdx/4;
			break;
		}
	}

	int a_sIdx[SAT_NUM] = {0};

	// Set basement area-sat map
	for(int i = 0; i < TOPO_M; i++){
		int obtIdx = (i + dif + TOPO_M) % TOPO_M; // orbit index;
		int bsIdx = obtIdx * TOPO_N; // base satellite index;
		Coord loc = SAT(bsIdx).getPos().loc;
		Coord a_loc = {.lat = 22.5, .lon = static_cast<double>(15+30*i)};
		double minDis = CalculateDistance(loc,a_loc); // latitude distance
		int tsIdx = bsIdx; // target satellite index;
		for(int j = 1; j < TOPO_N; j++){
			loc = SAT(j+bsIdx).getPos().loc;
			int tmp_dis = CalculateDistance(loc,a_loc);
			if(tmp_dis < minDis){
				tsIdx = j + bsIdx;
				minDis = tmp_dis;
			}
		}
		if(tsIdx == -1){
			cout << "No Sat found!" << endl;
			exit(0);
		}
		
		a_sIdx[4*i] = tsIdx;

		if(SAT(tsIdx).getPos().isNorth){
			a_sIdx[44-4*i] = GetForeSatIndex(tsIdx,3);
		}
		else{
			a_sIdx[44-4*i] = GetRearSatIndex(tsIdx,3);
		}
	}
	// Set other area-sat map
	for(int i = 0; i < 12; i++){
		int aIdx = 4*i;
		int sIdx = a_sIdx[aIdx];
		if(SAT(a_sIdx[aIdx]).getPos().isNorth){
			a_sIdx[aIdx+1] = GetForeSatIndex(sIdx,1);
			a_sIdx[aIdx+2] = GetRearSatIndex(sIdx,1);
			a_sIdx[aIdx+3] = GetRearSatIndex(sIdx,2);
		}
		else{
			a_sIdx[aIdx+1] = GetRearSatIndex(sIdx,1);
			a_sIdx[aIdx+2] = GetForeSatIndex(sIdx,1);
			a_sIdx[aIdx+3] = GetForeSatIndex(sIdx,2);
		}	
	}
	// Assign to S_A
	for(int i = 0; i < SAT_NUM; i++){
		// A_S[i] = ID(a_sIdx[i]);
		S_A[ a_sIdx[i] ] = ID(i);
	}
}

// the value returned is area index (0~47)
int GetAreaIndex(Coord loc)
{
	int mat[12][4] = {
		 0, 1, 2, 3,
		 4, 5, 6, 7,
		 8, 9,10,11,
		12,13,14,15,
		16,17,18,19,
		20,21,22,23,
		24,25,26,27,
		28,29,30,31,
		32,33,34,35,
		36,37,38,39,
		40,41,42,43,
		44,45,46,47
	};

	int r,c;
	//lon
	if(loc.lon == 180) {r = 5;}
	else if(loc.lon == -180) {r = 11;}
	else if(loc.lon < 0) {r = -loc.lon/30 + 6;}
	else {r = loc.lon/30;}

	//lat
	if(loc.lat == 90) {c = 1;}
	else if(loc.lat == -90) {c = 3;}
	else if(loc.lat < 0) {c = -loc.lat/45 + 2;}
	else {c = loc.lat/45;}

	return mat[r][c];
}

double Dec(double lat)
{
    double lat_fix = lat;
    if(lat > 86) lat_fix = 86;
    if(lat < -86) lat_fix = -86;
    double a = RADIAN(4);
    double b = RADIAN(lat_fix);
    double c = tan(a)*tan(b);
    double dec = asin(c);
	if(c >= 1) {dec = PI/2;}//add by cyj 2016.9.23 , To avoid overflow!
    return double((dec/PI)*180);
}

void FixLon(Coord &loc, bool isN)
{
    double out;
    if(!isN){
        out = loc.lon + Dec(loc.lat);
        if(out > 180){
            out = -180+(loc.lon+Dec(loc.lat)-180);
        }
        else if(out < -180){
            out = 180+(loc.lon+Dec(loc.lat)+180);
        }
        loc.lon = out;
    }
    else{
        out = loc.lon - Dec(loc.lat);
        if(out < -180){
            out = 180+(out+180);
        }
        else if(out > 180){
            out = -180+(out-180);
        }
        loc.lon = out;
    }		
}

int GetSatIdByAreaId(int areaId)
{
	for(int i = 0; i < SAT_NUM; i++)
		if(S_A[i] == areaId)
			return i+1;
	return -1;
}