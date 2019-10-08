#include "debug.h"
#include "common.h"
#include "calculate.h"
#include "file.h"
#include "timer.h"
#include "cmd.h"
#include <iostream>
#include <fstream>
using namespace std;

#define debug_time 18663

void debug(char* s)
{
	cout << cmd("ls",12,"-l") << endl;
}