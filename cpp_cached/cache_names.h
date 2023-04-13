#if _MSC_VER
#if __clang__
#define CACHE_NAME_POSTFIX "_CLANGCL"
#else
#define CACHE_NAME_POSTFIX "_MSC"
#endif
#else
#define CACHE_NAME_POSTFIX "_XXX"
#endif

#if _DEBUG
#define CACHE_NAME_PREFIX "_DEBUG"
#else
#define CACHE_NAME_PREFIX "_RELEASE"
#endif
