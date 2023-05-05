#include "cache_entry.h"

std::string machine_name()
{
#if _MSC_VER
#if __clang__
#define MACHINE_PREFIX "CLANGCL"
#else
#define MACHINE_PREFIX "MSC"
#endif
#else
#define MACHINE_PREFIX "XXX"
#endif

#if _DEBUG
	return MACHINE_PREFIX "_DEBUG";
#else
	return MACHINE_PREFIX "_RELEASE";
#endif
}
