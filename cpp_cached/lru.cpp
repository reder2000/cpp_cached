#include "lru.h"

LRU_cache::LRU_cache(size_t maximum_memory) :
        _maximum_memory(maximum_memory), _current_memory(0) {}

void LRU_cache::resize(size_t maximum_memory) {
    if (maximum_memory < _maximum_memory)
        reclaim(_maximum_memory - maximum_memory);
    _maximum_memory = maximum_memory;
}

bool LRU_cache::has(const std::string &key) {
    return _map.find(key) != _map.end();
}

void LRU_cache::erase(const std::string &key) {
    if (!has(key)) return;
    _map.erase(key);
}


void LRU_cache::reclaim(size_t memory) {
    while ((memory + _current_memory) > _maximum_memory && !_list.empty()) {
        const std::string &key = _list.back();
        auto i = _map.find(key);
        _current_memory -= i->second._mem_used;
        _map.erase(i);
        _list.pop_back();
    };
}


