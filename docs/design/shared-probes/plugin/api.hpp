// The interface between a host and a plugin built against libycxx in different modes (one static,
// one shared): both see std::__y1, so the C++ types cross. Exported with an explicit attribute,
// which only the static mode needs with GCC (a function whose signature names a hidden library
// type is hidden, DECISIONS §2); the shared plugin's build checks that it is exported without it.
#pragma once
#include <string>
#include <vector>
#if defined(PLUGIN_EXPORT_ATTRIBUTE)
#  define PLUGIN_API extern "C" [[gnu::visibility("default")]]
#else
#  define PLUGIN_API extern "C"
#endif
PLUGIN_API std::vector<std::string>* plugin_make(const std::string& seed, int n);
PLUGIN_API std::size_t plugin_consume(std::vector<std::string>* v);   // deletes v
PLUGIN_API void plugin_throw(const std::string& what);
PLUGIN_API int plugin_uncaught();
// Runtime state the host and the plugin share only when both use libycxx.so's runtime (design §5):
PLUGIN_API bool plugin_sees_current_exception();   // std::current_exception() != nullptr
PLUGIN_API void* plugin_terminate_handler();        // std::get_terminate()
PLUGIN_API void plugin_rethrow();                   // throw; (called inside the host's handler)
using plugin_make_t = std::vector<std::string>* (*)(const std::string&, int);
using plugin_consume_t = std::size_t (*)(std::vector<std::string>*);
using plugin_throw_t = void (*)(const std::string&);
using plugin_uncaught_t = int (*)();
using plugin_sees_t = bool (*)();
using plugin_terminate_t = void* (*)();
using plugin_rethrow_t = void (*)();
