#include <mcmf.h>
#include <time.h>
#include <unistd.h>


MCMF::MCMF(int n, string logfile, 
    map<string, vector<int>>& _item_history, 
    bool isAcc,
    bool isDebug, 
    bool isOutput) {
    item_history = _item_history;
    swap_in.clear();
    swap_out.clear();

	_n=n;
    _isOutput=isOutput;
    _isDebug=isDebug;
    _isAcc = isAcc;
	num_items = (n-2)/3;
	G = new vector<int>[_n];
	inq = new int[_n];
	d = new int[_n];
	p = new int[_n];
	a = new int[_n];

	for (int i=0; i<_n; i++) {
		G[i].clear();
		d[i] = 0;
		p[i] = 0;
		a[i] = 0;
	} 

    edges.clear();
    // locks = new omp_lock_t[_n];
    // inq_locks = new omp_lock_t[_n];
    // for(int i=0;i<_n;i++) {
    //     if(_isDebug) if (i % 10000 == 0) cout << "init lock " << i  << " of " << _n << endl;
    //     omp_init_lock(&locks[i]);
    //     omp_init_lock(&inq_locks[i]);
    // }
    // qlock = new omp_lock_t[num_items+1];
    // for(int i=0;i<=num_items;i++) {
    //     if(_isDebug) if (i % 10000 == 0) cout << "init lock " << i  << " of " << num_items << endl;
    //     omp_init_lock(&qlock[i]);
    // }
    // omp_init_lock(&end_lock);
    string log_dir = "log_new/";
    string result_dir = "result/";
    string runtime_dir = "runtime/";
    if(!isAcc) {
        log_dir = "log_appr/";
        result_dir = "result_appr/";
        runtime_dir = "runtime_appr/";
    }
    if(_isOutput) {
        out_log.open(log_dir + logfile);
        out_info.open(log_dir + logfile+"_info");
        out_result.open(result_dir + logfile);
    }
    if(_isDebug) {
        runtime_log.open(runtime_dir + logfile);
    }
    
    round = 0;
    visited = 0;
}

MCMF::~MCMF() {
	for (int i=0; i < _n; i++) G[i].clear();
	delete[] G;
	delete[] inq, d, p, a;
	
    // for (int i=0; i<_n; i++) {
    //     omp_destroy_lock(&locks[i]);
    //     omp_destroy_lock(&inq_locks[i]);
    // }
    // for (int i=0; i<=num_items; i++) omp_destroy_lock(&qlock[i]);
    // omp_destroy_lock(&end_lock);

    if(_isOutput) {
        out_info.close();
        out_result.close();
        out_log.close();
    }
	if(_isDebug) {
        runtime_log.close();
    }
    swap_in.clear();
    swap_out.clear();
}

void MCMF::AddEdge(int from,int to,int cap,int cost){
    edges.push_back(Edge(from,to,cap,0,cost));
    edges.push_back(Edge(to,from,0,0,-cost));
    int m=edges.size();
    G[from].push_back(m-2);
    G[to].push_back(m-1);
    return;
}

void MCMF::AddRoundEdge(int from,int to,int cap,int cost){
    edges.push_back(Edge(from,to,cap,0,cost));
    edges.push_back(Edge(to,from,cap,0,cost));
    int m=edges.size();
    G[from].push_back(m-2);
    G[from].push_back(m-1);
    G[to].push_back(m-2);
    G[to].push_back(m-1);
    return;
}

int MCMF::Search(int u) {
	int res = u;
    for(int i=0;i<G[u].size();i++){
        Edge& e=edges[G[u][i]];
        if(e.cap<= e.flow 
            || (e.to == u - num_items && e.to > num_items)
            || (e.to < u && e.to > num_items )
            ) continue;
        if(d[e.to]>d[u]+e.cost) {
            d[e.to]=d[u]+e.cost;
            p[e.to]=G[u][i];
            a[e.to]=min(a[u],e.cap-e.flow);
            res = i % num_items;
        }
    }
    return res;
}

int MCMF::SearchItem(string item) {
    auto history = item_history[item];
    int last_index = 0;
    int next_index = 0;
    int last = 0;
    int next = 0;
    int curr = 0;
    int res;

    while(curr <= num_items){
        int u = history[curr];
        res = Search(u);
        res = Search(u+num_items);
        res = Search(u+2*num_items);
    }
    return 0;
}

bool MCMF::BellmanFord(int s,int t,int &flow,long long&cost){
    
    //original method
    for(int i=0;i<_n;i++) {
     	d[i]=INF;
     	inq[i] = false;
    }
    d[s]=0;inq[s]=1;p[s]=0;a[s]=INF;

    // fast app. version
    if(_isAcc) {
        // single thread version    
        int count = 0;
        
        map<int, vector<int>> Q;
        Q[0].push_back(0);
        int _count = 0;

        while(Q.size()) {
            // cout << _count << " " << Q.size() << endl;
            auto it = Q.begin();
            int u = (it->second)[0];
            it->second.erase(it->second.begin());
            if(it->second.size() == 0) Q.erase(it);
            inq[u]=0;

            for(int i=0;i<G[u].size();i++){
                Edge& e=edges[G[u][i]];
                if(e.cap>e.flow&&d[e.to]>d[u]+e.cost){
                    d[e.to]=d[u]+e.cost;
                    p[e.to]=G[u][i];
                    a[e.to]=min(a[u],e.cap-e.flow);
    		        if(!inq[e.to]){
    		        	if(e.to == 3*num_items+1) Q[e.to].push_back(e.to);
                        else Q[(e.to-1)%num_items+1].push_back(e.to);
                        inq[e.to]=1;
    		        }
                }
            }
            _count++;

        }

        // // parallel
        // cout << "begin" << endl;
        // int count = 0;
        // map<int, queue<int>> Q;
        // Q[0].push(0);
        // int _count = 0;
        // while(Q.size()) {
        //     vector<int> Q_temp;
        //     for(auto it = Q.begin(); it != Q.end(); ++it) {
        //         // for(int i=0; i < it->second.size(); i++) {
        //         //     Q_temp.push_back(it->second.front());
        //         //     it->second.pop();
        //         //     count_temp++;
        //         // }
        //         if(it->second.size()==0) continue;
        //         Q_temp.push_back(it->second.front());
        //         it->second.pop();
        //         if(Q_temp.size() >= 100) break;
        //     }
        //     cout << _count << " " << Q_temp.size() << endl;
        //     omp_set_num_threads(Q_temp.size());

        //     for(int j=0; j<Q_temp.size(); j++) {
        //         inq[Q_temp[j]] = 0;
        //     }

        //     #pragma omp parallel for
        //     for(int j=0; j<Q_temp.size(); j++) {
        //         int u = Q_temp[j];
                
        //         for(int i=0;i<G[u].size();i++){
        //             Edge& e=edges[G[u][i]];
        //             if(e.cap>e.flow&&d[e.to]>d[u]+e.cost){
        //                 omp_set_lock(&locks[e.to]);
        //                 d[e.to]=d[u]+e.cost;
        //                 p[e.to]=G[u][i];
        //                 a[e.to]=min(a[u],e.cap-e.flow);
        //                 omp_unset_lock(&locks[e.to]);
                        
        //                 if(!inq[e.to]){
        //                     auto lock_index = (e.to == 3*num_items+1? num_items : (e.to-1)%num_items);
        //                     omp_set_lock(&qlock[lock_index]);
        //                     if(inq[e.to]) {
        //                         omp_unset_lock(&qlock[lock_index]);
        //                     } else {
        //                         if(e.to == 3*num_items+1) {
        //                             inq[e.to]=1;
        //                             Q[e.to].push(e.to);
        //                         } 
        //                         else {
        //                             inq[e.to]=1;
        //                             Q[(e.to-1)%num_items+1].push(e.to);
        //                         } 
        //                         omp_unset_lock(&qlock[lock_index]);
        //                     }
        //                 }
        //             }
        //         }
        //     }

        //     #pragma omp barrier

        //     _count++;
        // }

        if(_isDebug) runtime_log << "round " << round << " takes " << _count << " searches ";
        Q.clear();

    } else {
        Search(s);
        for(int i=1; i<=num_items; i++) {
            Search(i);
            Search(i+num_items);
            Search(i+2*num_items);
            Search(i);
        }
        Search(t);
        if(_isDebug) runtime_log << "round " << round << " ";
    }
    
    if(d[t]==INF) return false;
    flow+=a[t];
    cost+=(long long)d[t]*(long long)a[t];

    for(int u=t;u!=s;u=edges[p[u]].from){
    	edges[p[u]].flow+=a[t];
        edges[p[u]^1].flow-=a[t];
        
        if(_isOutput) {
            //out_log << edges[p[u]].from << " " << edges[p[u]].to << " " << edges[p[u]].cap << " " << edges[p[u]].flow << " " << edges[p[u]].cost << endl;
            //out_log << edges[p[u]^1].from << " " << edges[p[u]^1].to << " " << edges[p[u]^1].cap << " " << edges[p[u]^1].flow << " " << edges[p[u]^1].cost << endl; 
            // new log: only output swap in/out each time
            
            auto _from = edges[p[u]].from; 
            auto _to = edges[p[u]].to;    
            auto _flow = edges[p[u]].flow;
            // if a swap edge is not in swap in/out, and flow > 0, insert
            // else if it's in, but flow = 0, remove
            if((_to-_from) % num_items == 0 && _from <= num_items) {
                if (_flow > 0) {
                    if (swap_in.count(_from) == 0 ) swap_in[_from] = 1;
                } else if(_flow == 0) {
                    if (swap_in.count(_from)) swap_in.erase(_from);
                }
            }

            else if((_from-_to) % num_items == 0 && _to <= num_items) {
                if (_flow > 0) {
                    if(swap_out.count(_to) == 0 ) swap_out[_to] = 2;
                } else if(_flow == 0) {
                    if(swap_out.count(_to)) swap_out.erase(_to);
                }
            }


            _from = edges[p[u]^1].from; 
            _to = edges[p[u]^1].to;    
            _flow = edges[p[u]^1].flow;
            if((_to-_from) % num_items == 0 && _from <= num_items) {
                if (_flow > 0) {
                    if (swap_in.count(_from) == 0 ) swap_in[_from] = 1;
                } else if(_flow == 0) {
                    if (swap_in.count(_from)) swap_in.erase(_from);
                }
            }

            else if((_from-_to) % num_items == 0 && _to <= num_items) {
                if (_flow > 0) {
                    if(swap_out.count(_to) == 0 ) swap_out[_to] = 2;
                } else if(_flow == 0) {
                    if(swap_out.count(_to)) swap_out.erase(_to);
                }
            }

        }
    
        visited+=2;
    }
    if(swap_in.size() != swap_out.size()) cout << "something wrong at swap in out! " << swap_in.size() << " " << swap_out.size() << endl;

    if(_isOutput) {
        for(auto it = swap_in.begin(); it != swap_in.end(); ++it) {
            out_log << it->first << " ";
        } 
        out_log << endl;
        for(auto it = swap_out.begin(); it != swap_out.end(); ++it) {
            out_log << it->first << " ";
        }
        out_log << endl;
    }

    // if(_isOutput) {
    //         // after updating swap in/out, print
    //         int _swap_in = 0;
    //         int _swap_out = 0;
    //         for(auto it = swap.begin(); it != swap.end(); ++it) {
    //             out_log << it->first - 1 << " " << it->second << endl;
    //             if(it->second == 1) _swap_in++;
    //             else if(it->second == 2) _swap_out++; 
    //         }
    //         if(swap_in != swap_out) cout << "something wrong at swap in out! " << swap_in << " " << swap_out << endl;
    // }

    
    return true;
}

void MCMF::MincostMaxflow(int s,int t, long long& cost){
    int flow=0;cost=0;

    if(_isOutput) out_info << num_items << endl;
    clock_t overall_time = 0;
    while(true) {
    	visited = 0;
        if(_isOutput) {
            out_log << round << endl;
        }
        
        time_t begin, end;
        time(&begin);
    	if (!BellmanFord(s,t,flow,cost)) break;
    	if(_isOutput) out_info << round << " " << visited << " " << a[t] << " " << (long long)d[t]*(long long)a[t] << endl;
        time(&end);
        overall_time += (end - begin);
        if(_isDebug) runtime_log << "takes " << (end-begin) << " s, overall time is " << (overall_time)/3600 << " h " << (overall_time)/60 - 60*(overall_time/3600) << " m " << (overall_time)%60 << " s" << endl;
        round++;

        if(round == edges[0].cap) break;
    }

    if(_isOutput) {
        out_result << num_items << endl;

        for(auto it = edges.begin(); it != edges.end(); ++it) {
            if(it->flow <= 0) continue;
            out_result << it->from << " " << it->to << " " << it->cap << " " << it->flow << " " << it->cost << endl;
        }
    } 
    return;
}

void MCMF::Print() {
	for(auto it = edges.begin(); it != edges.end(); ++it) {
		cout << it->from << " " << it->to << " " << it->cap << " " << it->flow << " " << it->cost << endl;
	}
}