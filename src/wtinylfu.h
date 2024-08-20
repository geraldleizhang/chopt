#include<iostream>

#ifndef __WTINYLFU_H__
#define __WTINYLFU_H__
#include "bf.h"
#include "cbf.h"
#include "slru.h"
#include "lru.h"
#include <unistd.h>

using namespace std;


// W-TinyLFU:
// Window cache ->  TinyLFU   ->   Main Cache
//     LRU 			BF + CBF 		  SLRU
class WTinyLFU
{
public:
	WTinyLFU(int cache_size, int window_size) {
		int w_size = cache_size / 100 == 0? 1 : cache_size/100;
		if(cache_size == 2) w_size = 0;
		wcache = new LRUCache(w_size);
		doorkeeper = new BF(1024);
		admission = new CBF(1024);
		int main_prob_size = cache_size / 5 == 0? 1: cache_size / 5;
		int main_protect_size = cache_size - main_prob_size - w_size;
		
		//#pragma omp critical
		//{
		//	cout << "initialization " << cache_size << " " << w_size << " " << main_prob_size << " " << main_protect_size << endl;
		//}

		main_cache = new SLRU(main_prob_size, main_protect_size);
		_time = 0;
		// _W = 100000;
 		_W = 20 * cache_size;
 	}
	~WTinyLFU() {
		delete doorkeeper;
		delete admission;
		delete main_cache;
	}

	string set(string key) {
		// W-TinyLFU pseudo code:
		// if in wcache:
		// 		set wcache, return ""
		// else:
	// 			set to wcache, key = evicted item from wcache
	// 			if key == null:
	// 				return
	// 			else: ** gotta through tinylfu
	// 				if not in doorkeeper:
	// 					insert into doorkeeper, return key
	// 				else:			
	// 					remove from doorkeeper, insert to admission
	// 					if more frequent than last in main_cache:
	// 						set to main cache, return the old key;
	// 					else:
	// 						return the key
		
		// previous version:			
		// check if the item is in main cache. 
		// 		if yes, just update
		//  	if not, check if in doorkeeper
		//	 		if yes, remove from doorkeeper, insert in admission
		//	 		if not, insert in doorkeeper, return
		// 		check count in admission, compare with the possibly evicted item. 
		// 			if larger, then insert the item, return the evicted item (can be none)
		// 			if smaller, return this item
		
		// _time++;
		// if(! (_time % _W)) {
		// 	doorkeeper->clear();
		// 	admission->half();
		// } 

		// if(wcache->isCached(key)) {
		// 	return wcache->set(key);
		// } else {
		// 	auto wcache_key = wcache->set(key);
		// 	if (wcache_key == "") return "";
		// 	key = wcache_key;

		// 	if(!doorkeeper->check(key)) {
		// 		doorkeeper->insert(key);
				
		// 		return key;
		// 	} else {
		// 		int curr_count = admission->insert(key);
		// 		int old_count = admission->count(main_cache->getLast());
		// 		if(!main_cache->isFull() || main_cache->isCached(key) > 0) {
		// 			auto res = main_cache->set(key);
					
		// 			return "";
		// 		} //return main_cache->set(key);
		// 		if (curr_count < old_count) return key;
		// 		auto res = main_cache->set(key);
				
		// 		return res;
		// 	}		
		// }

		// return "";

		//print();
		//cout << _time << " setting " << key << endl;

		_time++;
		if(! (_time % _W)) {
			doorkeeper->clear();
			admission->half();
		} 


		if(wcache->isCached(key)) {
			//cout << "already in wcache\n";
			return wcache->set(key);
		}

		if(main_cache->isCached(key)) {
			//cout << "already in main cache\n";
			return main_cache->set(key);
		}

		auto wcache_key = wcache->set(key);
		//cout << "wcache set, ret = " << wcache_key << endl;
		if(wcache_key == "") return "";
		key = wcache_key;


		//cout << "checking " << key << " in tinylfu\n";
		if(!doorkeeper->check(key)) {
			//cout << "tinylfu doesn't have " << key << endl;
			doorkeeper->insert(key);
			return key;
		} else {

			int curr_count = admission->insert(key);

			int old_count = admission->count(main_cache->getLast());
			if(!main_cache->isFull() || main_cache->isCached(key) > 0) {
				
				auto res = main_cache->set(key);
				//cout << "main cache not full, insert " << key << " res is " << res << endl;
				return "";
			} //return main_cache->set(key);
			//cout << "checking, " << curr_count << " " << old_count << endl;
			if (curr_count < old_count) return key;
			auto res = main_cache->set(key);
			
			return res;
		}
		return "";
		
	}

	bool isCached(string key) {
		if(wcache->isCached(key)) {
			//cout << key << " in wcache" << endl;
			return true;
		}
		else if(main_cache->isCached(key)) {
			//cout << key << " in main cache" << endl;
			return true;
		}
		return false;
	}

	bool isWCache(string key) {
		return wcache->isCached(key);
	}

	bool isMainCache(string key) {
		return main_cache->isCached(key);
	}

	void print() {
		cout << "******************************************\n";
		cout << "wcache:" << endl;
		wcache->print();
		cout << "main cache:" << endl;
		main_cache->print();
		cout << "******************************************\n";
	}
	
	LRUCache* wcache;
	BF* doorkeeper;
	CBF* admission;
	SLRU* main_cache;
	int _time;
	int _W;

};

void handle(WTinyLFU& cache, string item) {
	cout << "before: " << cache.isCached(item) << " ";
	cout << "res: " << cache.set(item) << " ";
	cout << "after: " << cache.isCached(item) << endl;
}

void TESTwtinylfu() {
	WTinyLFU cache(3, 2);

	handle(cache, "a");
	handle(cache, "a");
	cache.print();
	handle(cache, "b");
	cache.print();
	handle(cache, "a");
	cache.print();

	return;
}


class WTinyLFU_test
{
public:
	WTinyLFU_test(int window_size, int bf_size, int cbf_size, int prob_size, int protect_size, int w) {
		/*
		int w_size = cache_size / 100 == 0? 1 : cache_size/100;
		if(cache_size == 2) w_size = 0;
		wcache = new LRUCache(w_size);
		doorkeeper = new BF(10001);
		admission = new CBF(10001);
		int main_prob_size = cache_size / 5 == 0? 1: cache_size / 5;
		int main_protect_size = cache_size - main_prob_size - w_size;
		main_cache = new SLRU(main_prob_size, main_protect_size);
		_time = 0;
		_W = 1000;*/
		wcache = new LRUCache(window_size);
		doorkeeper = new BF(bf_size);
		admission = new CBF(cbf_size);
		main_cache = new SLRU(prob_size, protect_size);
		_W = w;
		_time = 0;
 	}
	~WTinyLFU_test() {
		delete doorkeeper;
		delete admission;
		delete main_cache;
	}

	string set(string key) {
		// TinyLFU:
		// check if the item is in main cache. 
		// 		if yes, just update
		//  	if not, check if in doorkeeper
		//	 		if yes, remove from doorkeeper, insert in admission
		//	 		if not, insert in doorkeeper, return
		// 		check count in admission, compare with the possibly evicted item. 
		// 			if larger, then insert the item, return the evicted item (can be none)
		// 			if smaller, return this item
		
		_time++;
		if(! (_time % _W)) doorkeeper->clear();
		if(wcache->isCached(key)) {
			wcache->set(key);
		} else {
			auto wcache_key = wcache->set(key);
			if (wcache_key == "") return "";
			key = wcache_key;
		}
		if(main_cache->isCached(key) || !main_cache->isFull()) {
			main_cache->set(key);
			return "";
		} else {
			if(!doorkeeper->check(key)) {
				doorkeeper->insert(key);
				return key;
			} else {
				int curr_count = admission->insert(key);
				int old_count = admission->count(main_cache->getLast());
				if (curr_count < old_count) return key;
				else {
					string res = main_cache->set(key);
					return res;
				}
			}
		}
	}

	bool isCached(string key) {
		if(main_cache->isCached(key)) return true;
		else if(wcache->isCached(key)) return true;
		return false;
	}

	void print() {
		cout << "main cache:" << endl;
		main_cache->print();
	}
	
	LRUCache* wcache;
	BF* doorkeeper;
	CBF* admission;
	SLRU* main_cache;
	int _time;
	int _W;
};

#endif