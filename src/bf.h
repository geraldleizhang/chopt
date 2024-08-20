#include<iostream>
#include<vector>
#include<string>
#include<functional>

#ifndef __BF_H__
#define __BF_H__

using namespace std;

class BF
{
public:
	BF(int length) {
		_length = length;
		v.clear();
		for (int i=0; i<_length; i++) v.push_back(0); 
	}

	~BF() {
		v.clear();
	}

	void insert(string key) {
		for(int i=1; i<=4; i++) {
			v[_hash(key) * i % _length] = 1;
		}
		return;
	}

	bool check(string key) {
		for(int i=1; i<=4; i++) {
			if(v[_hash(key) * i % _length] == 0) return false;
		}
		return true;
	}

	void clear() {
		for (int i=0; i<_length; i++) v[i] = 0;
	}
	
	hash<string> _hash;
	int _length;
	vector<int> v;
};

#endif