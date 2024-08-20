#include <iostream>
#include <cstring>
#include <list>
#include <unordered_map>
#include <vector>
#include <map>

using namespace std;

#ifndef __LRU_H__
#define __LRU_H__

struct Node{
	string key;
	int value;
	Node* prev;
	Node* next;
	Node(string _key, int _value): key(_key), value(_value), prev(NULL), next(NULL){}
};

class DoubleLinkedList
{
public:
	DoubleLinkedList(): size(0) {
		head = new Node("",0);
		tail = new Node("",0);
		head->prev = tail;
		tail->next = head;
	}

	int size = 0;

	Node* head;
	Node* tail;

	Node* add(string key, int value) {
		Node* page = new Node(key, value);
		if (size == 0) {
			head->prev = page;
			tail->next = page;
			page->prev = tail;
			page->next = head;
		} else {
			head->prev->next = page;
			page->prev = head->prev;
			head->prev = page;
			page->next = head;
		}
		size++;
		return page;
	}

	void move_to_head(Node* node) {
		if(node->next == head) return;

		node->prev->next = node->next;
		node->next->prev = node->prev;

		head->prev->next = node;
		node->prev = head->prev;
		head->prev = node;
		node->next = head;
		return;
	}

	string evict() {
		if (size == 0) return "";
		string res = tail->next->key;

		tail->next->next->prev = tail;
		tail->next = tail->next->next;
		
		size--;

		return res;
	}

	string getLast() {
		return tail->next->key;
	}

	void print() {
		cout << "head" << endl;
		for(auto temp = head->prev; temp != tail; temp = temp->prev) {
			cout << temp->key << " " << temp->value << endl;
		}
		cout << "tail" << endl;
		return;
	}
	
};

class LRUCache
{
public:
	LRUCache(int cap): _capacity(cap), _time(0){}
	~LRUCache() {
		_cache.clear();
	}

	bool isCached(string key) {
		auto it = _cache.find(key);
		if (it != _cache.end()) return true;
		else return false;
	}

	void get(string key) {
		auto it = _cache.find(key);
		if (it != _cache.end()) {

			_list.move_to_head(_cache[key]);
			_cache[key]->value = _time;
		}
		_time++;
		return;
	}

	bool find(string key) {
		return (_cache.find(key) != _cache.end());
	}

	// return "" if no eviction, or evicted key
	string set(string key) {
		string res = "";
		auto it = _cache.find(key);
		if (it != _cache.end()) {
			_list.move_to_head(_cache[key]);
			_cache[key]->value = _time;
		} else {
			if (_cache.size() == _capacity) {
				res = evict();
			}
			auto node = _list.add(key, _time);
			_cache[key] = node;
		}
		_time++;
		return res;
	}

	string evict() {
		auto evicted_key = _list.evict();
		_cache.erase(evicted_key);
		return evicted_key;
	}

	string getLast() {
		return _list.getLast();
	}

	// for migration
	void erase(string key) {
		auto node = _cache[key];
		node->prev->next = node->next;
		node->next->prev = node->prev;
		_cache.erase(key);
		return;
	}

	void print() {
		// cout << "Printing LRU Cache" << endl;
		// cout << "DList:" << endl;
		_list.print();
		/*cout << "Cache:" << endl;
		for (unordered_map<string, Node*>::iterator it = _cache.begin(); it != _cache.end(); ++it) {
			cout << (*it).second->key << " " << (*it).second->value << endl; 
		}*/
	}

	bool isEmpty() {
		if(_cache.size() == 0) return true;
		return false;
	}

	bool isFull() {
		if(_cache.size() == _capacity) return true;
		return false;
	}

	DoubleLinkedList _list;
	unordered_map<string, Node*> _cache;
	int _capacity;
	int _time;

};

/*
void LRUCacheTest() {
	LRUCache cache(5);
	cache.set("node1");
	cache.set("node2");
	cache.set("node3");
	cache.print();

	cache.get("node2");
	cache.print();

	cache.evict();
	cache.print();
}
*/

#endif
