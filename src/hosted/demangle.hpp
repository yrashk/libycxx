// libycxx hosted runtime: an Itanium C++ ABI demangler (5.1 "External Names"), for
// stacktrace_entry::description() and abi::__cxa_demangle (<cxxabi.h>). Internal to the runtime.
#pragma once

#include <string>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The demangled form of a mangled name ("_Z..."), in the style of the toolchains' c++filt
// ("ns::f<int>(char const*) const"); a clone suffix (".cold", ".isra.0", ...) is shown as
// " [clone .cold]". Returns false, leaving out unspecified, for a name that is not mangled or
// uses a construct this demangler does not know. Throws bad_alloc only.
bool __demangle(const char* __mangled, std::string& out);

// The same for a mangled <type> on its own (5.1.5), the form of type_info::name(): "i" is "int",
// "St13runtime_error" "std::runtime_error", "PFvvE" "void (*)()".
bool __demangle_type(const char* __mangled, std::string& out);

}} // namespace __ycxx::__detail
