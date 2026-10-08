// <cxxabi.h>'s abi::__cxa_demangle, as the Itanium C++ ABI specifies it (3.4, "Demangler API";
// not a standard header, but the run-time interface of the ABI the compilers implement, which
// programs use to name types: GoogleTest's type printer, {fmt}'s "{:t}"):
// - mangled_name is a mangled name: a symbol's (5.1.2, "_Z" <encoding>) or a type's, the form
//   of type_info::name() (5.1.5); the result names it as the compilers' demanglers do;
// - output_buffer: memory from malloc of *length bytes, expanded with realloc when too small, or
//   null for a new region from malloc; length, if not null, receives the buffer's length;
// - *status: 0 success, -1 allocation failure, -2 not a valid name, -3 an invalid argument; the
//   result is null when demangling fails, and the caller frees it.
#include <cxxabi.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include "check.hpp"

namespace outer {
namespace inner {
struct widget {};
template <class T>
struct box {};
} // namespace inner
} // namespace outer

std::string demangled(const char* name) {
  int status = 1;
  char* s = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  CHECK(status == 0);
  CHECK(s != nullptr);
  std::string r = s ? s : "";
  std::free(s);
  return r;
}

int main() {
  // Types, as type_info::name() spells them.
  CHECK(demangled(typeid(int).name()) == "int");
  CHECK(demangled(typeid(unsigned short).name()) == "unsigned short");
  CHECK(demangled(typeid(const char*).name()) == "char const*");
  CHECK(demangled(typeid(outer::inner::widget).name()) == "outer::inner::widget");
  CHECK(demangled(typeid(outer::inner::box<long>).name()) == "outer::inner::box<long>");
  CHECK(demangled(typeid(void (*)(int)).name()) == "void (*)(int)");
  // The standard classes may live in an inline namespace of std (libycxx: std::__y1, DECISIONS
  // §20.4), which the mangled name, and so the demangled one, carries.
  {
    std::string r = demangled(typeid(std::runtime_error).name());
    CHECK(r == "std::runtime_error" || (r.starts_with("std::") && r.ends_with("::runtime_error")));
  }
  // A function's symbol.
  CHECK(demangled("_ZN5outer5inner1fEi") == "outer::inner::f(int)");

  // The caller's buffer: used when large enough, else expanded with realloc.
  {
    std::size_t n = 64;
    char* buf = static_cast<char*>(std::malloc(n));
    int status = 1;
    char* s = abi::__cxa_demangle(typeid(outer::inner::widget).name(), buf, &n, &status);
    CHECK(status == 0 && s != nullptr && std::strcmp(s, "outer::inner::widget") == 0);
    CHECK(n >= std::strlen(s) + 1);
    std::free(s);
  }
  {
    std::size_t n = 4;
    char* buf = static_cast<char*>(std::malloc(n));
    int status = 1;
    char* s = abi::__cxa_demangle(typeid(outer::inner::widget).name(), buf, &n, &status);
    CHECK(status == 0 && s != nullptr && std::strcmp(s, "outer::inner::widget") == 0);
    CHECK(n >= std::strlen("outer::inner::widget") + 1);
    std::free(s);
  }
  // Not a valid mangled name: -2, and a null result.
  {
    int status = 0;
    CHECK(abi::__cxa_demangle("_Z", nullptr, nullptr, &status) == nullptr && status == -2);
    CHECK(abi::__cxa_demangle("not a mangled name", nullptr, nullptr, &status) == nullptr && status == -2);
  }
  // Invalid arguments: -3.
  {
    int status = 0;
    CHECK(abi::__cxa_demangle(nullptr, nullptr, nullptr, &status) == nullptr && status == -3);
    char buf[8];
    status = 0;
    CHECK(abi::__cxa_demangle("i", buf, nullptr, &status) == nullptr && status == -3);
  }
  // status may be null.
  char* s = abi::__cxa_demangle("i", nullptr, nullptr, nullptr);
  CHECK(s != nullptr && std::strcmp(s, "int") == 0);
  std::free(s);
  return 0;
}
