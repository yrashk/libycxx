// DECISIONS §2 with modules: a program that imports std exports none of the library's
// definitions, the modules' own (the module initializers _ZGIW3std, _ZGIW3std6compat) included:
// what the importer instantiates is declared hidden by the headers in the modules' global module
// fragment, as through #include (linkage/no_exported_library_symbols.pass.cpp).
// The test lists the symbols its executable exports (ELF: `nm -D --defined-only`, which
// -rdynamic fills with every default-visibility symbol; Mach-O: `nm -gU`) and fails on any
// mangled name that is not the test's own (namespace `own`).
// MODULES: std.compat
// FLAGS: -rdynamic -pthread
// UNSUPPORTED-SANITIZER: asan,ubsan,tsan  the sanitizer runtimes export symbols (and allocation functions) of their own
// REQUIRES: exceptions
import std.compat;
#include "module_check.hpp"

namespace own {
struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct T {
  int v;
  auto operator<=>(const T&) const = default;
};
int run() {
  int r = 0;
  try {
    throw error("own");
  } catch (const std::exception& e) {
    r += std::string_view(e.what()) == "own";
  }
  std::vector<T> v{{3}, {1}};
  std::ranges::sort(v);
  std::map<T, std::string> m{{{1}, "one"}};
  std::function<int(int)> f = [](int x) { return x + 1; };
  std::any a = T{4};
  auto sp = std::make_shared<T>(T{7});
  std::ostringstream os;
  os << std::format("{}", v.size()) << m.at(T{1}) << f(1);
  std::cout << "";
  std::thread th([&] { r += std::any_cast<T>(a).v == 4; });
  th.join();
  r += sp->v == 7 && os.str() == "2one2" && v.front().v == 1;
  r += typeid(v) != typeid(int);
  return r;
}
} // namespace own

int main(int, char** argv) {
  CHECK(own::run() == 4);
  // popen is POSIX, not exported by std.compat: the output goes through a file next to the program.
  std::string exe = std::string("'") + argv[0] + "'";
  std::string listing = std::string(argv[0]) + ".symbols";
  std::string cmd = "{ uname -s; if [ \"$(uname -s)\" = Darwin ]; then nm -gU " + exe + "; else nm -D --defined-only " +
                    exe + "; fi; } > '" + listing + "'";
  CHECK(std::system(cmd.c_str()) == 0);
  std::ifstream nm(listing);
  CHECK(nm.is_open());
  std::string os, s;
  int symbols = 0, foreign = 0;
  while (std::getline(nm, s)) {
    if (os.empty()) {
      os = s;
      continue;
    }
    std::string name = s.substr(s.rfind(' ') + 1);
    ++symbols;
    if (name.starts_with("__Z"))
      name.erase(0, 1); // Mach-O's C prefix
    bool fundamental_type_info = (name.starts_with("_ZTI") || name.starts_with("_ZTS")) && name.size() <= 8;
    bool library = (name.starts_with("_Z") && !name.contains("3own")) || name.contains("__cxa_") ||
                   name.contains("__gxx_personality") || name.contains("ycxx_pal_");
    if (library && os == "Darwin" && fundamental_type_info)
      continue; // default visibility on Darwin with GCC only (DECISIONS §2)
    if (library) {
      std::println("exported: {}", s);
      ++foreign;
    }
  }
  CHECK(symbols > 0);
  CHECK(foreign == 0);
  return 0;
}
