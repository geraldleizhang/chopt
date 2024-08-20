#include <queue>
#include <cstdio>
#include <vector>
#include <map>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <omp.h>
#include <climits>
using namespace std;

struct Edge{
    int from,to,cap,flow,cost;
    Edge(int u,int v,int c,int f,int w):from(u),to(v),cap(c),flow(f),cost(w){}
};

class CHOPT
{
public:
	CHOPT();
	~CHOPT();

	void addEdge() {
		
	}
	
};