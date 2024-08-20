#include<iostream>
#include<vector>
#include<string>
#include<functional>
#include<climits>
#include<map>

#ifndef __CBF_H__
#define __CBF_H__

using namespace std;

class CBF
{
public:
	CBF(int length) {
		_length = length;
		v.clear();
		for (int i=0; i<_length; i++) v.push_back(0); 
	}

	~CBF() {
		v.clear();
	}

	int insert(string key) {
		// count, hash_value
		map<int, vector<int>> insert_list;
		int lowest = INT_MAX;
		for(int i=1; i<=4; i++) {
			int _h = _hash(key) * i % _length;
			insert_list[v[_h]].push_back(_h);
			if(v[_h] < lowest) lowest = v[_h];
		}

		for(auto it = insert_list[lowest].begin(); it != insert_list[lowest].end(); ++it) {
			v[*it]++;
		}

		insert_list.clear();
		return lowest+1;
	}

	int count(string key) {
		int lowest = INT_MAX;
		for(int i=1; i<=4; i++) {
			int _h = v[_hash(key) * i % _length];
			if (_h < lowest) lowest = _h;
		}
		return lowest;
	}

	void half() {
		for (int i=0; i<_length; i++) v[i] /= 2; 
	}

	void clear() {
		for (int i=0; i<_length; i++) v[i] = 0;
	}
	
	hash<string> _hash;
	int _length = 10001;
	vector<int> v;
};

#endif