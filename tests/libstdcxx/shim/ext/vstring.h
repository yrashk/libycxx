// Test-harness shim (see bits/c++config.h), included by testsuite_container_traits.h, which
// specializes its traits for libstdc++'s extension string __gnu_cxx::__versa_string. The template
// is declared (so the helper parses) but not defined: the extension is not provided, and tests
// that name __gnu_cxx stay skipped (libstdcxx_format.py).
#pragma once

namespace __gnu_cxx {
template <class CharT, class Traits, class Alloc, template <class, class, class> class Base>
class __versa_string;
} // namespace __gnu_cxx
