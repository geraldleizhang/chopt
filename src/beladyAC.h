#ifndef __BELADYAC_H__
#define __BELADYAC_H__

#include <iostream>
#include <cstring>
#include <list>
#include <unordered_map>
#include <vector>
#include <map>
#include <climits>
#include <algorithm>

using namespace std;

class BeladyACCache
{
public:
	BeladyACCache(vector<string>& file, vector<int>& next_access_list, int cap): _file(file), _next_access_list(next_access_list), _capacity(cap), _time(0) {}
	~BeladyACCache() {
		_next_access_list.clear();
		_cache.clear();
		_order.clear();
	}

	void _order_erase(int _time, string key) {
		_order[_time].erase(key);
		if(_order[_time].size() == 0) {
			_order.erase(_time);
		}
		return;
	}

	void _order_insert(int _time, string key) {
		_order[_next_access_list[_time]][key] = 0;
		return;
	}

	void get(string key) {
		auto _isCached = isCached(key);
		if (_isCached) {
			auto old_time = _cache[key];
			_cache[key] = _next_access_list[_time];
			_order_erase(old_time, key);
			_order_insert(_time, key);
		}
		_time++;
		return;
	}

	bool isCached(string key) {
		return (_cache.find(key) != _cache.end());
	}

	// return value:
	// 		"" if inserted and no eviction
	// 		key if not inserted
	// 		evicted key if an old key is evicted
	string set(string key) {
		string res = "";

		bool _isCached = isCached(key);
		if (_isCached) {
			auto old_time = _cache[key];
			_cache[key] = _next_access_list[_time];
			_order_erase(old_time, key);
			_order_insert(_time, key);
		} else {
			// item is not in cache, need to **DECIDE** if accept the new item or not

			if (_cache.size() == _capacity) {
				auto lastKey = getLast();
				auto new_time = _next_access_list[_time];
				auto old_time = _cache[lastKey];
				if (old_time < new_time) {
					res = key;
				} else {
					res = evict(key);
				}				
			} else {
				_cache[key] = _next_access_list[_time];
				_order_insert(_time, key);
			}
		}

		_time++;
		return res;
	}

	// Evict longest accessed item, including the new coming key
	// return "" if not inserted, or evicted key 
	string evict(string key) {

		//cout << "evicting " << _capacity << " " << _time << endl;
		// _order.begin() is always the largest next access time
		
		//if (_order.begin()->first < _next_access_list[_time]) return key;

		auto evicted_key = _order.begin()->second.begin()->first;
		_cache.erase(evicted_key);
		_order_erase(_order.begin()->first, evicted_key);

		_cache[key] = _next_access_list[_time];
		_order_insert(_time, key);
		return evicted_key;
	}

	string getLast() {
		return _order.begin()->second.begin()->first;
	}

	bool isFull() {
		if(_cache.size() == _capacity) return true;
		return false;
	}

	void print() {
		cout << "Printing optimal cache" << endl;
		cout << "Cache" << endl;
		for (auto it = _cache.begin(); it != _cache.end(); ++it) {
			cout << it->first << " " << it->second << endl; 
		}
		cout << "Order map" << endl;
		for (auto it = _order.begin(); it != _order.end(); ++it) {
			cout << (*it).first << " ";
			for (map<string, int>::iterator iit = (*it).second.begin(); iit != (*it).second.end(); ++iit) {
				cout << (*iit).first << " ";
			}
			cout << endl;
		}
	}

	// Question: how to read a time window of the future
	// in _next_access_list (time -> time) we know the item at a time's next accessed time -> 2nd or further access
	// in _cache  			(key -> time)  we know each item's next access time
	// in _order 			(time -> key)  we know a time's coming items (which are those item's first access from now)

	
	vector<string> _file;
	// current _time -> next accessed time
	vector<int> _next_access_list;
	// key -> node
	unordered_map<string, int> _cache;
	// next accessed time -> all possible keys
	map<int, map<string, int>, greater<int> > _order;
	
	int _capacity;
	int _time;
};


void BeladyACCacheTest() {
	vector<string> file = {"a", "b", "c", "a", "a", "b"};
	vector<int> next_access_list = {3, 5, 100, 4, 100, 100};
	BeladyACCache cache(file, next_access_list, 1);
	for (auto it = file.begin(); it != file.end(); ++it) {
		auto item = *it;
		cout << cache.set(item) << endl;
		cache.print();
	}	

}

#endif