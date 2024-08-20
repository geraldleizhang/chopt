#ifndef __LFU_H__
#define __LFU_H__
#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
#include <vector>
#include <map>
#include <climits>

using namespace std;

struct LFUNode{
	string key;
	int value;
	int freq = 0;
	LFUNode* prev;
	LFUNode* next;
	LFUNode(string _key, int _value, int _freq): key(_key), value(_value), freq(_freq), prev(NULL), next(NULL){}
};

struct NodeQueue{
	NodeQueue* prev;
	NodeQueue* next;
	LFUNode* head;
	LFUNode* tail;
	int freq = 0;
	int size = 0;
	NodeQueue(int _freq): prev(NULL), next(NULL), freq(_freq), size(0){
		head = new LFUNode("", 0, _freq);
		tail = new LFUNode("", 0, _freq);
	}

	int addNode(LFUNode* node) {
		if (size == 0) {
			head->next = node;
			tail->prev = node;
			node->prev = head;
			node->next = tail;
		} else {
			node->prev = tail->prev;
			tail->prev->next = node;
			node->next = tail;
			tail->prev = node;
		}
		size++;
		return size;
	}

	int removeNode(LFUNode* node) {
		if (size==0) return 0;
		node->prev->next = node->next;
		node->next->prev = node->prev;
		size--;
		return size;
	}

	void print() {
		cout << "  queue " << freq << " " << size << endl; 
		if(size == 0) return;

		for (auto it = head->next; it != tail; it = it->next) {
			cout << "    " << it->key << " " << it->value << endl;
		}
	}

};

class QueueList
{
public:
	QueueList(): size(0){
		head = new NodeQueue(INT_MAX);
		tail = new NodeQueue(0);
		_queue_map[INT_MAX] = head;
		_queue_map[0] = tail;
	}
	~QueueList(){
		_queue_map.clear();
	}
	
	// return if QueueList has NodeQueue of _freq
	bool isInList(int _freq) {
		return _queue_map.find(_freq) != _queue_map.end();
	}

	// find the lower and upper existed frequency in QueueList for _freq
	pair<int, int> findEdge(int _freq){
		int lower = 0;
		int higher = INT_MAX;
		for(auto it = _queue_map.begin(); it != _queue_map.end(); ++it) {
			if (it->first > _freq) {
				higher = it->first;
				break;
			} else {
				lower = it->first;
			}
		}
		return make_pair(lower, higher);
	}

	NodeQueue* getQueue(int _freq) {
		if (isInList(_freq)) return _queue_map[_freq];
		else {
			NodeQueue* queue = new NodeQueue(_freq);
			addQueue(queue);
		}
	}

	// add queue to QueueList
	void addQueue(NodeQueue* queue) {
		if (size == 0) {
			head->next = queue;
			tail->prev = queue;
			queue->next = tail;
			queue->prev = head;
		} else {
			int _freq = queue->freq;
			auto edge = findEdge(_freq);
			auto _head = _queue_map[edge.second];
			auto _tail = _queue_map[edge.first];

			_head->next = queue;
			_tail->prev = queue;
			queue->next = _tail;
			queue->prev = _head;
		}
		_queue_map[queue->freq] = queue;
		size++;
		return;
	}

	void removeQueue(NodeQueue* queue) {
		int _freq = queue->freq;
		if (_queue_map.find(_freq) == _queue_map.end()) return;
		queue->prev->next = queue->next;
		queue->next->prev = queue->prev;
		_queue_map.erase(_freq);
		size--;
		return;
	}

	void removeQueue(int _freq) {
		if (_queue_map.find(_freq) == _queue_map.end()) return;
		auto queue = _queue_map[_freq];
		queue->prev->next = queue->next;
		queue->next->prev = queue->prev;
		_queue_map.erase(_freq);
		size--;
		return;
	}

	void print() {
		/*for (auto it = _queue_map.begin(); it != _queue_map.end(); ++it) {
			if (it->second == head) cout << it->first <<  " head "<< it->second->prev->freq << endl;
			else if (it->second == tail) cout << it->first << it->second->next->freq << " tail" << endl;
			else cout << it->first << " " << it->second->prev->freq << " " << it->second->next->freq << endl;
		}*/
		for (auto it = head->next; it != tail; it = it->next) {
			it->print();
		}
		return;
	}

	int size = 0;
	NodeQueue* head;
	NodeQueue* tail;

	map<int, NodeQueue*> _queue_map;
};

void QueueListTest() {
	QueueList _qlist;
	NodeQueue* queue1 = new NodeQueue(1);
	NodeQueue* queue3 = new NodeQueue(3);
	_qlist.addQueue(queue3);
	_qlist.addQueue(queue1);
	NodeQueue* queue2 = new NodeQueue(2);
	_qlist.addQueue(queue2);

	_qlist.print();
	_qlist.removeQueue(1);
	_qlist.removeQueue(queue2);
	_qlist.print();
	return;
}


class LFUCache
{
public:
	LFUCache(int _cap): _capacity(_cap), _size(0){
		_qlist = new QueueList;
	}
	~LFUCache() {
		_nodeList.clear();
	}

	NodeQueue* getNodeQueue(int _freq) {
		if (_qlist->_queue_map.find(_freq) != _qlist->_queue_map.end()) return _qlist->_queue_map[_freq];
		else {
			NodeQueue* queue = new NodeQueue(_freq);
			_qlist->addQueue(queue);
			return queue;
		}
		return NULL;
	}


	LFUNode* moveUp(LFUNode* node) {
		int _freq = node->freq;
		LFUNode* new_node = new LFUNode(node->key, node->value, _freq+1);
		auto old_queue = getNodeQueue(_freq);
		auto new_queue = getNodeQueue(_freq+1);
		auto old_size = old_queue->removeNode(node);
		if (!old_size) _qlist->removeQueue(old_queue);
		new_queue->addNode(new_node);

		_nodeList[node->key] = new_node;
		return new_node;
	}

	bool isCached(string key) {
		return _nodeList.find(key) != _nodeList.end();
	}

	int get(string key) {
		auto node = _nodeList[key];
		auto new_node = moveUp(node);
		return new_node->value;
	}

	int getValue(string key) {
		if(_nodeList.find(key) == _nodeList.end()) return 0;
		return _nodeList[key]->value;
	}	

	string set(string key) {
		string res = "";
		if(_nodeList.find(key) != _nodeList.end()) {
			auto node = _nodeList[key];
			node->value = 0;
			auto new_node = moveUp(node);
			res = key;
		} else {
			if (_size == _capacity) {
				res = evict();
			} 
			LFUNode* node = new LFUNode(key, 0, 1);
			auto queue = getNodeQueue(1);
			queue->addNode(node);
			_nodeList[key] = node;
			_size++;
		}
		// return "" if inserted and nothing evicted
		// key if not inserted
		// another key if something is evicted
		return res;
	}

	string evict() {
		auto _lowest_queue = _qlist->tail->prev;
		auto node = _lowest_queue->head->next;
		int old_size = _lowest_queue->removeNode(node);
		if (!old_size) _qlist->removeQueue(_lowest_queue);
		_nodeList.erase(node->key);
		_size--;
		return node->key;
	}

	void print() {
		cout << "cap " << _capacity << " size " << _size << endl;
		cout << "node list" << endl;
		for (auto it = _nodeList.begin(); it != _nodeList.end(); ++it) cout << it->first << " " << it->second->value << endl;
		cout << "queue view " << _qlist->size << endl;
		_qlist->print();	
	}

	map<string, LFUNode*> _nodeList;
	QueueList* _qlist;
	int _capacity;
	int _size;
	
};

void LFUCacheTest() {
	LFUCache cache(3);
	cache.set("a");
	cache.set("b");
	cache.set("c");
	cache.set("c");
	cache.set("c");
	cache.set("c");
	cache.print();

	cache.set("d");
	cache.print();
	cache.evict();
	cache.evict();
	
	cache.print();
}

#endif

