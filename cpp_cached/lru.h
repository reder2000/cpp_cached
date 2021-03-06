#pragma once 

#include <unordered_map>
#include <any>
#include <list>
#include <cpp_rutils/memory_size.h>
#include <cpp_rutils/require.h>
#include "cache_imp_exp.h"

class cpp_cached_API LRU_cache {

public:

	LRU_cache(size_t maximum_memory = 1024 * 1024);

	void resize(size_t maximum_memory);

	template <class T>
	void set(const std::string& key, const T& value);

	bool has(const std::string& key);

	void erase(const std::string& key);

	template <class T>
	const T& get(const std::string& key);

private:

	struct entry {
		entry() = default;
		template <class T>
		entry(const T& value);

		std::any _value;
		size_t _mem_used;
	};

	using map_type = std::unordered_map<std::string, entry>;
	using lru_type = std::list<std::string>;

	size_t _maximum_memory;
	size_t _current_memory;

	const entry& _get(const std::string& key) ;

	void reclaim(size_t memory)  ;

	map_type _map;
	lru_type _list;
};

inline const  LRU_cache::entry & LRU_cache::_get(const std::string& key) 
{
	MREQUIRE( has(key) , "{} does not exits", key);
	return const_cast<map_type*>(&_map)->operator[](key);
}


template<class T>
inline LRU_cache::entry::entry(const T& value) :
	_value(value)
{
	_mem_used = sizeof(std::any) + memory_size(value);
}

template<class T>
inline void LRU_cache::set(const std::string& key, const T& value)
{
	MREQUIRE(!has(key), " {} already exists ", key);
	entry new_entry(value);
	reclaim(new_entry._mem_used);
	_current_memory += new_entry._mem_used;
	_map[key] = new_entry;
	_list.push_front(key);
}

template<class T>
inline const T& LRU_cache::get(const std::string& key)
{
	return std::any_cast<const T&>(_get(key)._value);
}
