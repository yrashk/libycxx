// FLAGS: -O1
// Arrays of aggregates whose members are containers of different allocator types, each
// constructed with the default allocator argument ([vector.cons], [deque.cons],
// [string.cons]: const Allocator& = Allocator()), as in table-driven tests (Abseil's
// str_split_test, CLI11's TransformTest). GCC 16 constant-folds such initializers at -O1 and
// crashed (ICE in cxx_eval_indirect_ref) when the container copied its allocator from the
// default argument, which it bound to another member's allocator temporary.
#include <deque>
#include <string>
#include <vector>
#include "check.hpp"

struct Spec {
  std::string in;
  std::vector<std::string> out;
};

struct WideSpec {
  std::string in;
  std::wstring out;
};

struct DequeSpec {
  std::string in;
  std::deque<std::string> out;
};

inline int wide_size(const std::wstring& s) { return static_cast<int>(s.size()); }

struct Mixed {
  int n;
  std::string s;
};

int main() {
  const Spec specs[] = {
      {"", {""}},
      {",", {"", ""}},
      {"foo,bar", {"foo", "bar"}},
  };
  CHECK(specs[0].in.empty() && specs[0].out.size() == 1 && specs[0].out[0].empty());
  CHECK(specs[1].out.size() == 2);
  CHECK(specs[2].in == "foo,bar" && specs[2].out[1] == "bar");

  const WideSpec wide[] = {{"", {L'a'}}, {"x", {L'b', L'c'}}};
  CHECK(wide[0].out == L"a" && wide[1].out == L"bc" && wide[1].in == "x");

  const DequeSpec deques[] = {{"", {""}}, {"a,b", {"a", "b"}}};
  CHECK(deques[0].out.size() == 1 && deques[1].out.back() == "b");

  static const Mixed mixed[] = {{wide_size(L"xy"), std::string("ab")}};
  CHECK(mixed[0].n == 2 && mixed[0].s == "ab");
  return 0;
}
