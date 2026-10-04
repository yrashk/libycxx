// [support.srcloc.cons]/1 Table 43: line_ is the presumed line number; file_name_ the presumed
// file name; function_name_ "A name of the current function such as in __func__ if any, an
// empty string otherwise" (so a call outside any function, e.g. in a namespace-scope
// initializer, yields ""). /2 Remarks: a call appearing as a default member initializer "should
// correspond to the location of the constructor definition or aggregate initialization that uses
// the default member initializer"; as a default argument "should correspond to the location of
// the invocation of the function that uses the default argument". /3 Example 1.
// Exercised in templates, lambdas, aggregate initialization and nested default arguments.
#include <source_location>
#include <cstring>
#include "check.hpp"

using SL = std::source_location;

constexpr SL at_namespace_scope = SL::current();
static_assert(at_namespace_scope.line() == __LINE__ - 1);
static_assert(at_namespace_scope.function_name()[0] == '\0');

struct Agg {
  int x;
  SL loc = SL::current();
};

struct S {
  SL member = SL::current();
  int other = 0;
  S(SL loc = SL::current()) : member(loc) {}
  S(int v) : other(v) {}   // the default member initializer refers to this line
  S(double) {}             // ... and to this one
};
constexpr unsigned S_int_line = __LINE__ - 3;
constexpr unsigned S_double_line = __LINE__ - 3;

template <class T> unsigned line_in_template(SL loc = SL::current()) { return loc.line(); }
template <class T> SL here_in_template() { return SL::current(); }
constexpr unsigned here_line = __LINE__ - 1;

unsigned inner(SL loc = SL::current()) { return loc.line(); }
unsigned outer(unsigned l) { return l; }

constexpr unsigned constexpr_line(SL loc = SL::current()) { return loc.line(); }
static_assert(constexpr_line() == __LINE__);

int main() {
  Agg a{1};
  CHECK(a.loc.line() == __LINE__ - 1);   // aggregate initialization uses the default member initializer here
  CHECK(std::strcmp(a.loc.file_name(), __FILE__) == 0);

  S s1;
  CHECK(s1.member.line() == __LINE__ - 1);   // default argument: the invocation
  S s2(5);
  CHECK(s2.member.line() == S_int_line);
  S s3(1.0);
  CHECK(s3.member.line() == S_double_line);

  CHECK(line_in_template<int>() == __LINE__);
  CHECK(line_in_template<double>() == __LINE__);
  SL t = here_in_template<char>();
  CHECK(t.line() == here_line);
  CHECK(std::strstr(t.function_name(), "here_in_template") != nullptr);

  auto lam = [](SL loc = SL::current()) { return loc; };
  SL l1 = lam();
  CHECK(l1.line() == __LINE__ - 1);
  auto lam2 = [] { return SL::current(); };
  CHECK(lam2().line() == __LINE__ - 1);
  CHECK(lam2().function_name()[0] != '\0');   // inside the closure's operator()

  unsigned direct = outer(inner());
  CHECK(direct == __LINE__ - 1);

#line 1000
  SL m = SL::current();
  CHECK(m.line() == 1000);
  CHECK(line_in_template<long>() == 1002);
  return 0;
}
