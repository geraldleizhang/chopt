#include <queue>
#include <cstdio>
#include <vector>
#include <map>
#include <queue>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <omp.h>
#include <climits>
using namespace std;

const int maxn=1<<10;
const int INF=1<<20;

struct Edge{
    int from,to,cap,flow,cost;
    Edge(int u,int v,int c,int f,int w):from(u),to(v),cap(c),flow(f),cost(w){}
};

class MCMF{
public:
    MCMF(int n, string logfile, map<string, vector<int>>& _item_history, bool isIcc = true,  bool isDebug = false, bool isOutput = false);
    ~MCMF();

    void AddEdge(int from,int to,int cap,int cost);

    void AddRoundEdge(int from,int to,int cap,int cost);

    bool BellmanFord(int s,int t,int &flow,long long&cost);

    void MincostMaxflow(int s,int t, long long& cost);

    int Search(int u);

    int SearchItem(string item);

    void Print();

	int _n;
	int num_items;
    vector<Edge> edges;
    vector<int> *G;
    int *inq;
    int *d;
    int *p;
    int *a;
    bool _isAcc;

    bool _isDebug;
    bool _isOutput;

    ofstream out_log;
    ofstream out_info;
    ofstream out_result;
    ofstream runtime_log;
    int round;
    int visited;

    map<string, vector<int>> item_history;

    map<int, int> swap_in;
    map<int, int> swap_out;

};
