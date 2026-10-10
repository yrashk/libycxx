// A program built with libycxx exports none of the library's definitions (DECISIONS §2) in static
// mode, and in shared mode none but its instantiations of std::__y1 and __ycxx (§20.12): not
// those the headers emit in the program (inline functions, template instantiations, type_info
// objects and vtables, inline variables), not those of the archives (the runtime, the ABI
// runtime). Another C++ library in the process (Apple's libc++/libc++abi, which every Darwin
// process loads; libstdc++ in a shared object) can then neither take over libycxx's definitions
// nor be taken over by them; that includes libycxx's default allocation functions (the images
// that link libycxx share them through the allocation table, __ycxx_allocation_functions, a name of
// libycxx's own).
//   [replacement.functions]/2: a program's own replacement of operator new is still the one used,
//   and is exported as the program's other functions are (the test's own: _Znwm, _ZdlPv, _ZdlPvm).
// The test lists the symbols its executable exports (ELF: `nm -D`, the dynamic symbol table,
// which -rdynamic fills with every default-visibility symbol; Mach-O: `nm -gU`) and fails on any
// mangled name that is not the test's own (namespace `own`) and on the ABI runtime's names.
// FLAGS: -rdynamic -pthread
// UNSUPPORTED-SANITIZER: asan,ubsan,tsan  the sanitizer runtimes export symbols (and allocation functions) of their own
// REQUIRES: exceptions
#include <any>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#include <variant>
#include <vector>
#include "check.hpp"

namespace own {
struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct T {
  int v;
  auto operator<=>(const T&) const = default;
};
int replaced_new_calls = 0;
int run() {
  int r = 0;
  try {
    throw error("own");
  } catch (const std::exception& e) {
    std::exception_ptr p = std::current_exception();
    r += p && std::strcmp(e.what(), "own") == 0;
  }
  try {
    std::rethrow_exception(std::make_exception_ptr(std::out_of_range("x")));
  } catch (const std::logic_error&) {
    ++r;
  }
  try {
    try {
      throw 1;
    } catch (int) {
      std::throw_with_nested(error("outer"));
    }
  } catch (const std::nested_exception& n) {
    r += n.nested_ptr() != nullptr;
  }
  std::vector<T> v{{3}, {1}};
  std::map<T, std::string> m{{{1}, "one"}};
  std::function<int(int)> f = [](int x) { return x + 1; };
  std::any a = T{4};
  std::optional<T> o = T{5};
  std::variant<int, T> var = T{6};
  auto sp = std::make_shared<T>(T{7});
  std::ostringstream os;
  os << std::format("{}", v.size()) << m.at(T{1}) << f(1);
  std::cout << "";
  std::thread th([&] { r += std::any_cast<T>(a).v == 4; });
  th.join();
  std::promise<int> pr;
  pr.set_value(8);
  r += pr.get_future().get() == 8;
  r += o->v == 5 && std::get<T>(var).v == 6 && sp->v == 7 && os.str() == "2one2";
  r += typeid(v) != typeid(int);
  delete new T{9};
  return r;
}
} // namespace own

// A replacement (in the program, default visibility) replaces libycxx's hidden default.
void* operator new(std::size_t n) {
  ++own::replaced_new_calls;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

// Compiled with -DYCXX_SHARED when the test links libycxx's shared library (lit's shared
// configuration, tools/ycxx-cxx --shared).
#if defined(YCXX_SHARED) && YCXX_SHARED
constexpr bool shared_mode = true;
#else
constexpr bool shared_mode = false;
#endif

int main(int, char** argv) {
  CHECK(own::run() == 7);
  CHECK(own::replaced_new_calls > 0);

  // The first line is the system's name.
  std::string exe = std::string("'") + argv[0] + "'";
  std::string cmd = "uname -s; if [ \"$(uname -s)\" = Darwin ]; then nm -gU " + exe + "; else nm -D --defined-only " +
                    exe + "; fi";
  std::FILE* nm = ::popen(cmd.c_str(), "r");
  CHECK(nm != nullptr);
  char line[4096];
  std::string os;
  int symbols = 0, foreign = 0;
  while (std::fgets(line, sizeof line, nm)) {
    std::string s(line);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
      s.pop_back();
    if (os.empty()) {
      os = s;
      continue;
    }
    std::string name = s.substr(s.rfind(' ') + 1);
    ++symbols;
    if (name.rfind("__Z", 0) == 0)
      name.erase(0, 1); // Mach-O's C prefix
    // The test's own replacements (size_t is m or j).
    bool own_replacement = name == "_Znwm" || name == "_Znwj" || name == "_ZdlPv" || name == "_ZdlPvm" ||
                           name == "_ZdlPvj";
    // Shared mode (DECISIONS §20.12): the program exports what it instantiated of std::__y1 and
    // __ycxx, which the dynamic linker unifies with libycxx.so's (one definition per process);
    // those names are libycxx's alone, so no other C++ library can meet them. Every other name
    // stays hidden, as in static mode.
    bool shared_mode_export = shared_mode && (name.find("St4__y1") != std::string::npos ||
                                              name.find("6__ycxx") != std::string::npos);
    bool library = (name.rfind("_Z", 0) == 0 && name.find("3own") == std::string::npos && !own_replacement &&
                    !shared_mode_export) ||
                   name.find("__cxa_") != std::string::npos || name.find("__gxx_personality") != std::string::npos ||
                   name.find("ycxx_pal_") != std::string::npos || name.find("__ycxx_abi_") != std::string::npos ||
                   name.find("__dynamic_cast") != std::string::npos;
    if (library) {
      std::printf("exported: %s\n", s.c_str());
      ++foreign;
    }
  }
  CHECK(::pclose(nm) == 0);
  CHECK(symbols > 0); // nm ran and listed the program's own symbols (main at least)
  std::fflush(stdout);
  CHECK(foreign == 0);
  return 0;
}
