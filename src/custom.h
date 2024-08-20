#include<iostream>
#include<map>
#include<algorithm>
#include<vector>
#include<omp.h>
#ifndef __CUSTOM_H__
#define __CUSTOM_H__
#include "lru.h"
#include "slru.h"
#include "belady.h"

using namespace std;

class Counter
{
public:
	Counter() {}
	
	~Counter() {
		key2freq.clear();
		for(auto it = freq2key.begin(); it != freq2key.end(); ++it) it->second->clear();
		freq2key.clear();
	}
	
	int set(string key) {
		if(key2freq.find(key) == key2freq.end()) {
			key2freq[key] = 1;
			if(freq2key.find(1) == freq2key.end()) freq2key[1] = new vector<string>();
			freq2key[1]->push_back(key);
			return 1;
		} else {
			auto freq = key2freq[key];
			auto p = find(freq2key[freq]->begin(), freq2key[freq]->end(), key);
			if(p == freq2key[freq]->end()) {
				cout << "wrong! the counter is inconsistent" << endl;
				cout << key << " with freq " << freq << " is not in ";
				for(auto it = freq2key[freq]->begin(); it != freq2key[freq]->end(); ++it) {
					cout << *it << " ";
				} cout << "." << endl;
				print();
			}
			freq2key[freq]->erase(p);
			if(freq2key[freq]->size()==0) freq2key.erase(freq);
			if(freq2key.find(freq + 1) == freq2key.end()) freq2key[freq + 1] = new vector<string>();
			freq2key[freq+1]->push_back(key);
			key2freq[key]++;
			return freq + 1;
		}
		return 0;
	}

	int getValue(string key) {
		if(key2freq.find(key) != key2freq.end()) return key2freq[key];
		return 0;
	}

	void half() {
		for(auto it = key2freq.begin(); it != key2freq.end(); ++it) if(it->second > 1) key2freq[it->first] = it->second/2;
		for(auto it = freq2key.begin(); it != freq2key.end(); ++it) {
			auto cache_size = it->first;
			if(cache_size == 1) continue;
			for(auto iit = it->second->begin(); iit != it->second->end(); ++iit) {
				if(freq2key.find(cache_size/2) == freq2key.end()) freq2key[cache_size/2] = new vector<string>();
				freq2key[cache_size/2]->push_back(*iit);
			}
			it->second->clear();
		}
		return;

	}

	void remove(string key) {
		if(key2freq.find(key) != key2freq.end()) {
			auto freq = key2freq[key];
			key2freq.erase(key);
			auto p = find(freq2key[freq]->begin(), freq2key[freq]->end(), key);
			freq2key[freq]->erase(p);
			if(freq2key[freq]->size()==0) freq2key.erase(freq);
		}
		return;
	}

	string getLowestKey() {
		return *(freq2key.begin()->second)->begin();
	}

	void print() {
		cout << "printing counter" << endl;
		for(auto it = freq2key.begin(); it != freq2key.end(); ++it) {
			cout << "cache size " << it->first << endl;;
			for(auto iit = it->second->begin(); iit != it->second->end(); ++iit) cout << *iit << " ";
			cout << endl;
		}
		for (auto it = key2freq.begin(); it != key2freq.end(); ++it) cout << it->first << ":" << it->second << " ";
		cout << endl;
		cout << "print done" << endl;
	}

	map<string, int> key2freq;
	map<int, vector<string>*> freq2key;
};

void CounterTest() {
	omp_set_num_threads(100);
	#pragma omp parallel for 
	for(auto j = 0; j < 100; j++) {
		Counter* counter = new Counter();
		for (auto i=0; i<5; i++) counter->set("1");
		for (auto i=0; i<10; i++) counter->set("2");
		counter->print();
		counter->half();
		counter->print();
		for (auto i=0; i<15; i++) counter->set("1");
		for (auto i=0; i<20; i++) counter->set("2");
		counter->print();
	}
	
	return;
}


// Customize the three-part cache
// Window -> counter -> Main Cache
// Window can be LRU for metadata or item
// counter is an infinite size memory
// Main Cache can be LRU, SLRU, Belady
template <typename T> class Custom
{
public:
	Custom(int window_option, int counter_option, int window_size, int cache_size, int w = 0) {
		_window_option = window_option;
		_counter_option = counter_option;

		wcache = new LRUCache(window_size);
		counter = new Counter();

		// int main_prob_size = cache_size / 5 == 0? 1: cache_size / 5;
		// int main_protect_size = cache_size - main_prob_size - w_size;
		// main_cache = new main_cache<LRUCache>(main_prob_size, main_protect_size);
		_time = 0;
		_w = w;
 	}
	~Custom() {
		delete wcache;
		delete main_cache;
		delete counter;
	}

	string set(string key) {
		// firstly update the counter
		// check if item is in window
		// 		if not, insert into window, return
		// 		if yes, check if item is in main cache
		// 			if yes, update, return
		// 			if not, check if it should be inserted
		
		_time++;
		if(_w && !(_time%_w)) counter->half();
		if(_counter_option) counter->set(key);
		if(_window_option) {
			if(wcache->isCached(key)) {
				wcache->set(key);
			} else {
				auto wcache_key = wcache->set(key);
				if (wcache_key == "") return "";
				key = wcache_key;
			}
		}

		if(!_counter_option) counter->set(key);		

		if(main_cache->isCached(key) || !main_cache->isFull()) {
			main_cache->set(key);
			return "";
		} else {
			if(counter->getValue(key)) {
				int curr_count = counter->getValue(key);
				int old_count = counter->getValue(main_cache->getLast());
				if(curr_count <= old_count) return key;
				else {
					string res = main_cache->set(key);
					return res;
				}
			} else {
				counter->set(key);
				return key;
			}
		}
	}

	bool isCached(string key) {
		if(wcache->isCached(key) & _window_option) return true;
		else if(main_cache->isCached(key)) return true;
		return false;
	}

	void print() {
		if(_window_option) {
			cout << "window cache" << endl;
			wcache->print();
		}
		cout << "main cache:" << endl;
		main_cache->print();
	}
	
	int _window_option;
	int _counter_option;
	LRUCache* wcache;
	T* main_cache;
	Counter* counter;

	int _time;
	int _w;
};

class Custom_LRU : public Custom<LRUCache>
{
public:
	using Custom::Custom;
	Custom_LRU(int window_option, int counter_option, int window_size, int cache_size, int w) : Custom(window_option, counter_option, window_size, cache_size, w) {
		main_cache = new LRUCache(cache_size);
	}
	
	~Custom_LRU() {}
};

class Custom_SLRU : public Custom<SLRU>
{
public:
	using Custom::Custom;
	Custom_SLRU(int window_option, int counter_option, int window_size, int prob_size, int protect_size, int w) : Custom(window_option, counter_option, window_size, prob_size+protect_size, w) {
		main_cache = new SLRU(prob_size, protect_size);
	}
	
	~Custom_SLRU() {}
};

class Custom_OPT : public Custom<BeladyCache>
{
public:
	using Custom::Custom;
	Custom_OPT(int window_option, int counter_option, int window_size, vector<string>& file, vector<int>& next_access_list, int cache_size, int w) : Custom(window_option, counter_option, window_size, cache_size, w) {
		main_cache = new BeladyCache(file, next_access_list, cache_size);
	}
	
	~Custom_OPT() {}
};





#endif