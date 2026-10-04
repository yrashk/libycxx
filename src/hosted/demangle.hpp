// libycxx hosted runtime: an Itanium C++ ABI demangler (5.1 "External Names"), for
// stacktrace_entry::description(). Internal to the runtime.
#pragma once

#include <string>

namespace ycxx::detail {

// The demangled form of a mangled name ("_Z..."), in the style of the toolchains' c++filt
// ("ns::f<int>(char const*) const"); a clone suffix (".cold", ".isra.0", ...) is shown as
// " [clone .cold]". Returns false, leaving out unspecified, for a name that is not mangled or
// uses a construct this demangler does not know. Throws bad_alloc only.
bool demangle(const char* mangled, std::string& out);

} // namespace ycxx::detail
