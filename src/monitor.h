/* monitor.h: a branch of monitors we use*/

#include <vector>
#include <map>
#ifndef __MONITOR_H__
#define __MONITOR_H__
#include <lru.h>
#include <lfu.h>
#include <belady.h>
#include <beladyAC.h>
#include <static.h>
#include <tinylfu.h>
#include <wtinylfu.h>
#include <tinylfu_only.h>
#include <slru.h>
#include <custom.h>

static vector<int> DEFAULT_VECTOR;

// 	Heterogeneous monitor: cache model
//
//  DRAM 					 DRAM write: 1 		 DRAM read: 1
//	 	 				 o ---------------- o ---------------- o
// 				swap in: | 		  		    | 				   | swap out: 
// 	NVM 			   1 | 	 MVM  write: 5  |    NVM  read: 2  | 5
//	    		o ------ o ---------------- o ---------------- o ------ o
//

// for same read & write analysis: NVM read and write = 5

// cache result:
// 0: not in cache
// 1: in cache
// 2: swap in

template <typename T> long long monitor(T& cache, 
	vector<string> file, 
	vector<bool> option_list, 
	int& cache_size,
	long long& cost,
	int& swap_in_count,
	int& swap_out_count,
	int& NVM_read, 
	int& NVM_write,
	int& DRAM_read,
	int& DRAM_write,
	vector<int>& result,
	bool isSameRW = false) {

	int miss = 0;

	for (int i=0; i<file.size(); i++) {
		auto item = file[i];
		//auto lru_res = lru.set(item);
		auto isCached = cache.isCached(item);
		auto res = cache.set(item);
		//cout << "In monitor, " << isCached << " ret = " << res << endl;
		if (!isCached) {
			miss++;
			//if(lru_res == "") cout <<i << " " <<item << " " << lru_res << " something wrong!\n";
			bool isWrite = option_list[i];
			// cached successfully, swap in
			if (res == "") {
				swap_in_count++;
				if(option_list[i]) {
					DRAM_write++;
					// normal, samerw
					cost += 1;

				} 
				else {
					DRAM_read++;
					
					// dw + nr, normal
					// cost += 2;
					// samerw
					//cost += 6;
					cost += isSameRW? 6 : 2;
				} 
				result.push_back(2);	
			}
			// not cached, falling into an NVM access
			else if (res == item) {
				if (isWrite) {
					// nw, normal, samerw
					cost += 5;
					NVM_write++;
				}
				else {
					// nr, normal
					// cost += 2;
					// samerw
					// cost += 5;
					cost += isSameRW? 5 : 2;
					NVM_read++;
				} 
				result.push_back(0);
			}
			// cached, and another item is evicted, swap in + dram access + swap out (we don't count the DRAM read here since we consider the evicted item written to NVM at its last access)
			else {
				swap_in_count++;
				swap_out_count++;
				if(option_list[i]) {
					DRAM_write++;
					// dw + dr + nw, normal, samerw
					cost += 7;
				}
				else {
					DRAM_read++;
					// nr + dw + dr + nw, normal
					// cost += 8;
					// samerw
					// cost += 12;
					cost += isSameRW? 12 : 8;
				}
				result.push_back(2);
			}
		} else {
			// TODO: There are cases that isCached=true, but res is not "" which should not be the case
			if (option_list[i])  DRAM_write++;
			else DRAM_read++;
			// dr/dw
			cost += 1;
			result.push_back(1);
		}
	}
	cost += 5 * cache_size;
	// cost += swap_in_count * 6;
	swap_out_count += cache_size;
	// cout << "Miss: " << cache_size << " " << miss << " " << file.size() << endl;

	return cost;
}

// CHOPT monitor: given source file, swap_in and swap_out, calculate the cost, and each cache line's history
long long chopt_monitor(vector<string> file, 
	vector<bool> option_list, 
	vector<int>& swap_in, 
	vector<int>& swap_out,
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {

	map<string, int> cache;
	long long count = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;
	
	int next_swap_in = 0;
	int next_swap_out = 0;

	int res;

	int miss = 0;

	for (int i=0; i<file.size(); i++) {
		auto item = file[i];
		if (cache.find(item) != cache.end()) {
			if (i == swap_out[next_swap_out] && next_swap_out != -1) {
				if (next_swap_out >= swap_out.size()) next_swap_out=-1;
				else next_swap_out++;
				cache.erase(item);
				
				swap_out_count ++;
				
				if(option_list[i]) {
					DRAM_write++;
					// nw, normal, samerw
					count += 5;
				} 
				else {
					DRAM_read++;
					// dr + nw, normal, samerw
					count += 6;
				}

				res = 3;
			} else {
				if (option_list[i]) DRAM_write++;
				else DRAM_read++;
				// dr/dw, normal, samerw
				count += 1;
				res = 1;
			} 
		} else {
			miss++;
			if (i == swap_in[next_swap_in] && next_swap_in != -1) {
				if (next_swap_in >= swap_in.size()) next_swap_in=-1;
				else next_swap_in++;
				
				cache[item] = 1;
				// swap in, and don't count DRAM read/write
				
				if (option_list[i]) {
					DRAM_write++;
					// dw, normal
					count += 1;
				}

				else{
					DRAM_read++;
					// nr + dw, normal
					// count += 2;
					// samerw
					// count += 6;
					count += isSameRW? 6 : 2;
				} 
				swap_in_count++;
				res = 2;
			} else {

				if(option_list[i]) {
					// nw, normal, samerw
					count += 5;
					NVM_write++;
				}
				else {
					// nr, normal
					// count += 2;
					// samerw
					// count += 6;
					count += isSameRW? 6 : 2;
					NVM_read++;
				}
				res = 0;
			} 
		}	
		result.push_back(res);
	}

	// cache.clear();
	
	if (isDebug) {
		cout << "CHOPT Cache:" << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
		cout << "Miss: " << miss << " " << file.size() << endl;
		cout << "Overall access: " << DRAM_write + DRAM_read + NVM_write + NVM_read << endl;
	}

	return count;
}

// offline monitor: given offline cache behavior, calculate the cost, and each cache line's history
long long offline_monitor(vector<string> file, 
	vector<bool> option_list, 
	vector<int> operation,
	int cache_size,
	vector<int>& result,
	bool isDebug = false) {

	LRUCache cache(cache_size);
	// map<string, int> cache;
	long long count = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;
	
	int res;
	for (int i=0; i<file.size(); i++) {
		auto item = file[i];
		auto isCached = cache.isCached(item);

		if (isCached) {
			if (!operation[i]) {
				cache.erase(item);
				count += 5;
				swap_out_count ++;
				res = 3;
			} else {
				cache.set(item);
				if (option_list[i]) DRAM_write++;
				else DRAM_read++;
				count += 1;
				res = 1;
			} 
		} else {
			if (operation[i]) {
				auto _res = cache.set(item);
				
				// cached successfully, swap in
				if (_res == "") {
					count += 1;
					swap_in_count++;
					res = 2;	
				}
				// not cached, falling into an NVM access
				else if (_res == item) {
					if (option_list[i]) {
						count += 5;
						NVM_write++;
					}
					else {
						count += 2;
						NVM_read++;
					} 
					res = 0;
				}
				// cached, and another item is evicted, swap in + swap out
				else {
					count += 6;
					swap_in_count++;
					swap_out_count++;
					res = 2;
				}
			} else {
				if(option_list[i]) {
					count += 5;
					NVM_write++;
				}
				else {
					count += 2;
					NVM_read++;
				}
				res = 0;
			} 
		}	
		result.push_back(res);
	}

	count += 5 * cache_size;
	swap_out_count += cache_size;
	
	if (isDebug) {
		cout << "Offline Cache:" << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return count;
}

long long belady_monitor(vector<string> file, 
	vector<bool> option_list, 
	vector<int> next_access_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	BeladyCache cache(file, next_access_list, cache_size);
	monitor<BeladyCache>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "Belady Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long beladyac_monitor(vector<string> file, 
	vector<bool> option_list, 
	vector<int> next_access_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	BeladyACCache cache(file, next_access_list, cache_size);
	monitor<BeladyACCache>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "BeladyAC Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long static_monitor(vector<string> file, 
	vector<bool> option_list, 
	map<string, int> all_frequency, 
	int cache_size,
	vector<int>& result, 
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	StaticCache cache(all_frequency, cache_size);
	monitor<StaticCache>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "Static Cache:" << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long lru_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	LRUCache cache(cache_size);
	monitor<LRUCache>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		#pragma omp critical
		{
			cout << "LRU Cache: " << cache_size << endl;
			cout << swap_in_count << " " << swap_out_count << endl;
			cout << DRAM_write << " " << DRAM_read << endl;
			cout << NVM_write << " " << NVM_read << endl;
		}
		
	}

	return cost;
}

long long lfu_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size,
	vector<int>& result, 
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	LFUCache cache(cache_size);
	monitor<LFUCache>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "LFU Cache:" << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long tinylfu_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	TinyLFU cache(cache_size, file.size());
	monitor<TinyLFU>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "TinyLFU Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long wtinylfu_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	WTinyLFU cache(cache_size, file.size());
	monitor<WTinyLFU>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		#pragma omp critical
		{
			cout << "W-TinyLFU Cache: " << cache_size << endl;
			cout << swap_in_count << " " << swap_out_count << endl;
			cout << DRAM_write << " " << DRAM_read << endl;
			cout << NVM_write << " " << NVM_read << endl;
			cout << "Overall access: " << DRAM_write + DRAM_read + NVM_write + NVM_read << endl;
		}
		
	}

	return cost;
}

long long wtinylfu_test_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size,
	int window_size,
	int bf_size,
	int cbf_size,
	int prob_size,
	int protect_size,
	int w,
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	//int window_size, int bf_size, int cbf_size, int prob_size, int protect_size, int w

	WTinyLFU_test cache(window_size, bf_size, cbf_size, prob_size, protect_size, w);
	monitor<WTinyLFU_test>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "W-TinyLFU test Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long tinylfu_only_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	TinyLFUOnly cache(cache_size);
	monitor<TinyLFUOnly>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "TinyLFU Only Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long slru_monitor(vector<string> file, 
	vector<bool> option_list, 
	int cache_size, 
	vector<int>& result,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	int main_prob_size = cache_size / 5 == 0? 1: cache_size / 5;
	int main_protect_size = cache_size - main_prob_size;
	SLRU cache(main_prob_size, main_protect_size);
	monitor<SLRU>(cache, file, option_list, cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);

	if (isDebug) {
		cout << "SLRU Cache: " << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}

long long custom_monitor(vector<string> file, 
	vector<bool> option_list,
	int cache_type,
	int window_option,
	int counter_option,
	int window_size,
	int cache_size,
	int w,
	vector<int>& result,
	vector<int>& next_access_list = DEFAULT_VECTOR,
	bool isSameRW = false,
	bool isDebug = false) {
	long long cost = 0;

	int swap_in_count = 0, swap_out_count = 0;
	int NVM_read = 0, NVM_write = 0;
	int DRAM_read = 0, DRAM_write = 0;

	string cache_name = "";
	auto _cache_size = window_size + cache_size;
	switch(cache_type) {
		case 0: // LRU
		{
			cache_name = "Custom LRU Cache: ";
			Custom_LRU cache(window_option, counter_option, window_size, cache_size, w);
			monitor<Custom_LRU>(cache, file, option_list, _cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);
			break;
		}

		case 1: // SLRU
		{
			cache_name = "Custom SLRU Cache: ";
			int prob_size = cache_size / 5 == 0? 1: cache_size / 5;
			int protect_size = cache_size - prob_size;
			Custom_SLRU cache(window_option, counter_option, window_size, prob_size, protect_size, w);
			monitor<Custom_SLRU>(cache, file, option_list, _cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);
			break;
		}
		case 2: // OPT
		{
			cache_name = "Custom OPT Cache: ";
			Custom_OPT cache(window_option, counter_option, window_size, file, next_access_list, cache_size, w);
			monitor<Custom_OPT>(cache, file, option_list, _cache_size, cost, swap_in_count, swap_out_count, NVM_read, NVM_write, DRAM_read, DRAM_write, result, isSameRW);
			break;
		}
			
	}


	if (isDebug) {
		cout << cache_name << cache_size << endl;
		cout << swap_in_count << " " << swap_out_count << endl;
		cout << DRAM_write << " " << DRAM_read << endl;
		cout << NVM_write << " " << NVM_read << endl;
	}

	return cost;
}



long long online_monitor(vector<string> file) {
	
}

#endif