#pragma once

// used for building a shared library

#pragma warning(disable:4251)

#if (defined(_WIN32) || defined(WIN32)) && defined(BUILD_SHARED_LIBS)
    #ifdef cpp_cached_EXPORTS
        #define cpp_cached_API __declspec(dllexport)
    #else
        #define cpp_cached_API __declspec(dllimport)
    #endif
 #else
    #define cpp_cached_API
#endif

