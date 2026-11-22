// libycxx hosted runtime: an Itanium C++ ABI demangler (5.1 "External Names"), for
// stacktrace_entry::description() and abi::__cxa_demangle (<cxxabi.h>). Internal to the runtime.
#pragma once

#include <memory>
#include <string>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

enum class __demangle_status { success = 0, no_memory = -1, invalid = -2 };

struct __demangle_result {
  __demangle_status status;
  std::unique_ptr<char[]> bytes;
  std::size_t size = 0;
  const char* c_str() const noexcept { return bytes ? bytes.get() : ""; }
};

// A symbol ("_Z...") or a standalone type ("i", "St13runtime_error", ...). On success the
// result owns the complete NUL-terminated text independently of the input.
__demangle_result __try_demangle(const char* __mangled, bool __type) noexcept;

// Stacktrace adapter for symbols. Leaves out untouched on failure. Scratch allocation failure
// throws bad_alloc with exceptions enabled and otherwise lets the caller use the raw name.
bool __demangle(const char* __mangled, std::string& out);

}} // namespace __ycxx::__detail
