#include <iostream>
#ifndef __SLRU_H__
#define __SLRU_H__
#include "lru.h"

class SLRU
{
public:
	SLRU(int prob_size, int protect_size) {
		Probation = new LRUCache(prob_size);
		Protection = new LRUCache(protect_size);
	}
	~SLRU() {
		delete Protection;
		delete Probation;
	}

	string set(string key) {
		// check if in Protection
		// 		if not, check if in Probation
		// 				if not, new item, insert in Probation
		// 				if yes, move to Protection
		// 		if yes, update in Protection
		string res = "";
		if(Protection->isCached(key)) {
			res = Protection->set(key);
		} else {
			if(Probation->isCached(key)) {
				res = Protection->set(key);
				Probation->erase(key);
				if(res != "") res = Probation->set(res);
			}
			else {
				res = Probation->set(key);
			}
		}
		return res;

	}

	void erase(string key) {
		if(Probation->isCached(key)) Probation->erase(key);
		if(Protection->isCached(key)) Protection->erase(key);
		return;
	}

	string evict() {
		if (!Probation->isEmpty()) return Probation->evict();
		else if (!Protection->isEmpty()) return Protection->evict();
		else return "no cached item";
	}

	string getLast() {
		if (!Probation->isEmpty()) return Probation->getLast();
		else if (!Protection->isEmpty()) return Protection->getLast();
		else return "";
	}

	int isCached(string key) {
		if(Protection->isCached(key)) return 2;
		else if (Probation->isCached(key)) return 1;
		return 0;
	}

	bool isFull() {
		return Probation->isFull();
	}

	void print() {
		if (!Probation->isEmpty()) {
			cout << "Probation:" << endl;
			Probation->print();
		}
			
		if (!Protection->isEmpty()) {
			cout << "Protection:" << endl;
			Protection->print();
		}
		return;		
	}

	LRUCache* Probation;
	LRUCache* Protection; 
};

#endif
