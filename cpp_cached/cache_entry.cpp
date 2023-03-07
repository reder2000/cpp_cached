#include "cache_entry.h"

std::string machine_name()
{
#if _MSC_VER
#if __clang__
#define MACHIN_PREFIX "CLANGCL"
#else
#define MACHIN_PREFIX "MSC"
#endif
#else
#define MACHIN_PREFIX "XXX"
#endif

#if _DEBUG
	return MACHIN_PREFIX "_DEBUG";
#else
	return MACHIN_PREFIX "_RELEASE";
#endif
}
