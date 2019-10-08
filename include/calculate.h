#ifndef __CALCULATE_H__
#define __CALCULATE_H__

#include "topo.h"

#define PI 3.14159265358979323846264338328
#define EARTH_RADIUS 6378.137 // km
#define COVER_RADIUS 2500.0 // km

inline double RANDOM(double a,double b){return rand()/((double)RAND_MAX/(b-a))+a;} 

inline double RADIAN(double m) {return m * PI / 180.0;}

inline int GetOrbitIndex(int idx) {return idx/8;}

inline int GetSatIndexInOrbit(int idx) {return idx%8;}

int GetForeSatIndex(int idx,int num = 1);

int GetRearSatIndex(int idx,int num = 1);

int GetSideSatIndex(int idx,bool isEast,bool isNorth);

NodePos RandomPos();

double CalculateDistance(Coord x,Coord y); 

void UpdateSATableByFile();

void UpdateSATable();

int GetAreaIndex(Coord loc);

double Dec(double lat);

void FixLon(Coord &loc, bool isN);

int GetSatIdByAreaId(int areaId);

#endif
