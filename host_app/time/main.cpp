#include <ctime>
#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
	cout << static_cast<int>((time(NULL)%100)+100) << endl;
	return 0;
}
