# cpp_cached

cpp_cached is a collection of data caching schemes:
- memory LRU 
- disk one file per object
- sqlite backend
- postgre backend
- rocksdb backend
- a combination of two of the previous caches (usually mem first)

##	use

caches are key/value stores

they all conform to the following interface

```C++
// c is some cache object
// o some object of class O
// key is at least a std::string_view
// fn is a function returning an O
c.set(key,o);
bool b = c.has(key);
if (b)
	o = c.get(key);
// this line
auto oo = c.get(key,fn);
// is equivalent to
O ooo = c.has(key) ? c.get(key) : fn();
```

The concept `is_a_cache` checks that an object conforms to the cache interface.
