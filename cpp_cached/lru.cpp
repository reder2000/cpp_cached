#include "lru.h"

LRUCache::LRUCache(size_t maximum_memory) :
	_maximum_memory(maximum_memory), _current_memory(0) {}

void LRUCache::resize(size_t maximum_memory) {
	if (maximum_memory < _maximum_memory)
		reclaim(_maximum_memory - maximum_memory);
	_maximum_memory = maximum_memory;
}

bool LRUCache::has(const std::string& key) {
	return _map.find(key) != _map.end();
}

void LRUCache::erase(const std::string& key) {
	if (!has(key)) return;
	_map.erase(key);
}


void LRUCache::reclaim(size_t memory) {
	while ((memory + _current_memory) > _maximum_memory && !_list.empty()) {
		const std::string& key = _list.back();
		auto i = _map.find(key);
		_current_memory -= i->second._mem_used;
		_map.erase(i);
		_list.pop_back();
	};
}


