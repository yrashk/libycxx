// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// FLAGS: -freflection
// REQUIRES: gcc, exceptions
// Reflection (P2996): the compiler evaluates the metafunctions declared in std::meta and builds
// their results (std::vector<info>, string_view, ...), and throws std::meta::exception. Compiled
// with -freflection where the compiler has it (GCC 16).
#include <meta>
#include <cstdio>
struct S { int a; long b; };
consteval bool throws() {
  try { (void)std::meta::identifier_of(^^int); } catch (const std::meta::exception&) { return true; }
  return false;
}
int main() {
  constexpr auto n = std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::current()).size();
  constexpr auto id = std::meta::identifier_of(^^S);
  bool r = n == 2 && id == "S" && throws();
  std::puts(r ? "ok" : "FAIL");
  return r ? 0 : 1;
}
