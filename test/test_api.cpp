#include <fstream>
#include <iostream>
#include <sys/time.h>
using namespace std;

#define SAT_NUM 48
#define SA_FILE "../../topofile/SA.dat"

int S_A[SAT_NUM] = {0};

void ReadSAFile(int t)
{
	ifstream fin(SA_FILE,ios::binary);

	fin.seekg(sizeof(S_A)*t, ios_base::beg);
	fin.read((char*)S_A,sizeof(S_A));
	
	fin.close();
}

int main(int argc, char *argv[])
{
	if(argc < 3){
		cout << "Wrong Input!( " << "./getId  { sat AREA_ID> | area SAT_ID } [time TIME] )" << endl;
		return 0;
	}

	int t = (argc == 4) ? stoi(argv[3]) : static_cast<int>(time(NULL)%86400);
	ReadSAFile(t);
	if(string(argv[1]) == "sat")
		cout << "area is:" << S_A[stoi(argv[2])-1] << endl;
	else if(string(argv[1]) == "area"){
		for(int i = 0; i < 48; i++){
			if(S_A[i] == stoi(argv[2]))
				cout << "sat is:" << i+1 << endl;
		}
	}
	else
		cout << "Wrong Input!( " << "./getId  { sat AREA_ID> | area SAT_ID } [time TIME] )" << endl;

    return 0;
}
