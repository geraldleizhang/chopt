#include<iostream>

#ifndef __TINYLFU_H__
#define __TINYLFU_H__
#include "bf.h"
#include "cbf.h"
#include "slru.h"

using namespace std;


// TinyLFU:
// Window cache ->  TinyLFU   ->   Main Cache
//     LRU 			BF + CBF 		  SLRU
class TinyLFU
{
public:
	TinyLFU(int cache_size, int w_size) {
		// int w_size = cache_size/100;
		// wcache = new LRUCache(w_size);
		doorkeeper = new BF(1024);
		admission = new CBF(1024);
		int main_prob_size = cache_size / 5 == 0? 1: cache_size / 5;
		int main_protect_size = cache_size - main_prob_size;

		main_cache = new SLRU(main_prob_size, main_protect_size);
		_time = 0;
		// _W = 1000;
		// _W = w_size
		_W = 20 * cache_size;
 	}
	~TinyLFU() {
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
		// if(! (_time % _W)) doorkeeper->clear();
		if(! (_time % _W)) {
			doorkeeper->clear();
			admission->half();
		} 
		if(main_cache->isCached(key) || !main_cache->isFull()) {
			main_cache->set(key);
			return "";
		} else {
			if(!doorkeeper->check(key)) {
				doorkeeper->insert(key);
				return key;
			} else {
				int curr_count = admission->insert(key) + 1;
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
		return false;
	}

	void print() {
		cout << "main cache:" << endl;
		main_cache->print();
	}
	
	// LRUCache* wcache;
	BF* doorkeeper;
	CBF* admission;
	SLRU* main_cache;
	int _time;
	int _W;
};

#endif