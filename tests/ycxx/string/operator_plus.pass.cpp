// [string.op.plus]: operator+ for every combination of basic_string (const& and &&),
// const charT*, charT and type_identity_t<basic_string_view<charT, traits>>; the rvalue
// forms reuse their operand. /14 note: the type_identity_t parameters let a type implicitly
// convertible to basic_string_view be concatenated with a basic_string.
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

struct ToView {
  constexpr operator std::string_view() const { return "tv"; }
};

constexpr bool test() {
  const std::string a = "ab";
  const std::string b = "cd";
  if (a + b != "abcd") return false;
  if (std::string("x") + b != "xcd") return false;
  if (a + std::string("y") != "aby") return false;
  if (std::string("x") + std::string("y") != "xy") return false;
  if ("p" + a != "pab" || "p" + std::string("q") != "pq") return false;
  if ('c' + a != "cab" || 'c' + std::string("q") != "cq") return false;
  if (a + "p" != "abp" || std::string("q") + "p" != "qp") return false;
  if (a + 'c' != "abc" || std::string("q") + 'c' != "qc") return false;
  std::string_view sv = "sv";
  if (a + sv != "absv" || std::string("q") + sv != "qsv") return false;
  if (sv + a != "svab" || sv + std::string("q") != "svq") return false;
  if (a + ToView{} != "abtv" || ToView{} + a != "tvab") return false;
  if (std::string("q") + ToView{} != "qtv" || ToView{} + std::string("q") != "tvq") return false;
  // chains
  std::string c = a + "-" + b + '-' + sv + std::string("!");
  if (c != "ab-cd-sv!") return false;
  // long operands
  std::string longs(100, 'L');
  std::string r = longs + longs;
  if (r.size() != 200 || r[199] != 'L') return false;
  r = std::move(r) + "end";
  if (r.size() != 203 || !r.ends_with("end")) return false;
  r = "begin" + std::move(r);
  if (r.size() != 208 || !r.starts_with("beginL")) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::string() + std::string_view()), std::string>);
static_assert(std::is_same_v<decltype(L"x" + std::wstring()), std::wstring>);
static_assert(std::is_same_v<decltype(u'x' + std::u16string()), std::u16string>);

int main() {
  CHECK(test());
  using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;
  S x("x", IdAlloc<char>(4));
  S y = x + "y";
  CHECK(y == "xy");
  CHECK(y.get_allocator().id == 4);  // r = lhs copy-constructs (select_on... returns *this)
  S z = "z" + x;
  CHECK(z == "zx" && z.get_allocator().id == 4);
  return 0;
}
