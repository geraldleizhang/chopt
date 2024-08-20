#include <iostream>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

class StaticCache
{
public:
	StaticCache(map<string, int>& frequency, int cap): _frequency(frequency), _capacity(cap), _time(0){
		_get_frequent_items();
	}

	~StaticCache() {
		_frequency.clear();
		_cache.clear();
		_frequent_items.clear();
	}
	
	void _get_frequent_items() {
		int count = 0;
		map<int, map<string, int>, greater<int>> _ordered_frequency;
		for(auto it = _frequency.begin(); it != _frequency.end(); ++it) {
			_ordered_frequency[it->second][it->first] = 1;
		}

		for (auto it = _ordered_frequency.begin(); it != _ordered_frequency.end(); ++it) {
			for (auto iit = it->second.begin(); iit != it->second.end(); ++iit) {
				_frequent_items[iit->first] = 1;
				count ++;
				if (count == _capacity) break;
			}
			if(count == _capacity) break;
		}
		_ordered_frequency.clear();
		return;
	}

	bool isCached(string key) {
		return (_cache.find(key) != _cache.end());
	}

	void get(string key) {
		if(isCached(key)) _cache[key] = _time;
		_time++;
		return;
	}

	// return value:
	// 		"" if inserted and no eviction
	// 		key if not inserted
	// 		evicted key if an old key is evicted
	string set(string key) {
		string res = "";
		res = naive(key);
		
		_time++;
		return res;
	}

	string naive(string key) {
		if (_frequent_items.find(key) != _frequent_items.end()) {
			_cache[key] = _time;
			return "";
		} else {
			return key;
		}
		return key;
	}
	
	map<string, int> _frequency;
	// key -> node
	unordered_map<string, int> _cache;
	// pre-calculate most frequent keys
	map<string, int> _frequent_items;
	
	int _capacity;
	int _time;
};