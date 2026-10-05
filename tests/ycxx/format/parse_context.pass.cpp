// [format.parse.ctx]/3-7: basic_format_parse_context(fmt) gives begin() == fmt.begin(),
// end() == fmt.end(); advance_to(it) sets begin(); it is not copyable. /8-9: next_arg_id()
// returns 0, 1, ... and throws format_error after manual indexing (check_arg_id) was used;
// /11-12: check_arg_id(id) throws format_error after automatic indexing was used.
// /14-17: check_dynamic_spec<Ts...>(id), check_dynamic_spec_integral, check_dynamic_spec_string
// are usable in a formatter's parse; in a format string checked at compile time, they accept a
// matching argument type ([format.fmt.string]/3: the string is checked as a constant
// expression). [format.context]/6-9: arg(id) returns args_.get(id); out() / advance_to.
// [format.context] Example 1: a formatter parsing a width argument id.
// COUNTERPART: libcxx:utilities/format/format.formatter/format.context/format.context/.*
// COUNTERPART: libcxx:utilities/format/format.formatter/format.parse.ctx/(check_arg_id|ctor|next_arg_id).pass.cpp
// REQUIRES: exceptions
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

// "{:{}}" or "{:{N}}" or "{:[s]}" (a string argument as a prefix)
struct Padded {
  int v;
};
template <>
struct std::formatter<Padded> {
  std::size_t width_id = 0;
  bool string_prefix = false;
  constexpr auto parse(std::format_parse_context& pc) {
    auto it = pc.begin();
    if (it == pc.end() || *it == '}') throw std::format_error("needs a dynamic spec");
    if (*it == '[') {
      ++it;
      if (it == pc.end() || *it != 's') throw std::format_error("bad");
      ++it;
      if (it == pc.end() || *it != ']') throw std::format_error("bad");
      string_prefix = true;
      width_id = pc.next_arg_id();
      pc.check_dynamic_spec_string(width_id);
      return ++it;
    }
    if (*it != '{') throw std::format_error("bad");
    ++it;
    if (it != pc.end() && *it >= '0' && *it <= '9') {
      width_id = static_cast<std::size_t>(*it - '0');
      pc.check_arg_id(width_id);
      ++it;
    } else {
      width_id = pc.next_arg_id();
    }
    pc.check_dynamic_spec_integral(width_id);
    if (it == pc.end() || *it != '}') throw std::format_error("bad");
    return ++it;
  }
  auto format(Padded p, std::format_context& fc) const {
    if (string_prefix) {
      std::string_view pre = fc.arg(width_id).visit([](auto x) -> std::string_view {
        if constexpr (std::is_same_v<decltype(x), std::string_view>) return x;
        else if constexpr (std::is_same_v<decltype(x), const char*>) return x;
        else throw std::format_error("not a string");
      });
      return std::format_to(fc.out(), "{}{}", pre, p.v);
    }
    long long w = fc.arg(width_id).visit([](auto x) -> long long {
      if constexpr (std::is_integral_v<decltype(x)> && !std::is_same_v<decltype(x), bool> &&
                    !std::is_same_v<decltype(x), char>)
        return static_cast<long long>(x);
      else
        throw std::format_error("not an integer");
    });
    return std::format_to(fc.out(), "{:>{}}", p.v, static_cast<int>(w));
  }
};

int main() {
  static_assert(!std::is_copy_constructible_v<std::format_parse_context>);
  static_assert(!std::is_copy_assignable_v<std::format_parse_context>);
  static_assert(std::is_same_v<std::format_parse_context::char_type, char>);
  static_assert(std::is_same_v<std::wformat_parse_context::iterator, std::wstring_view::const_iterator>);
  static_assert(std::is_nothrow_constructible_v<std::format_parse_context, std::string_view>);
  static_assert(!std::is_convertible_v<std::string_view, std::format_parse_context>);

  {
    std::string_view fmt = "abc}";
    std::format_parse_context pc(fmt);
    CHECK(pc.begin() == fmt.begin() && pc.end() == fmt.end());
    pc.advance_to(fmt.begin() + 2);
    CHECK(*pc.begin() == 'c' && pc.end() == fmt.end());
    CHECK(pc.next_arg_id() == 0);
    CHECK(pc.next_arg_id() == 1);
    CHECK(pc.next_arg_id() == 2);
    bool thrown = false;
    try {
      pc.check_arg_id(0);
    } catch (const std::format_error&) {
      thrown = true;
    }
    CHECK(thrown);
  }
  {
    std::format_parse_context pc("");
    pc.check_arg_id(3);
    pc.check_arg_id(0);  // manual indexing may repeat and go in any order
    bool thrown = false;
    try {
      (void)pc.next_arg_id();
    } catch (const std::format_error&) {
      thrown = true;
    }
    CHECK(thrown);
  }
  {
    std::wformat_parse_context wpc(L"x");
    CHECK(*wpc.begin() == L'x' && wpc.next_arg_id() == 0);
  }
  // compile-time checked format strings with dynamic specs
  CHECK(std::format("{:{}}|", Padded{7}, 4) == "   7|");
  CHECK(std::format("{0:{1}}|{0:{2}}", Padded{7}, 3, 2u) == "  7| 7");
  CHECK(std::format("{:{}}", Padded{1}, 3LL) == "  1");
  CHECK(std::format("{:{}}", Padded{1}, static_cast<unsigned short>(2)) == " 1");
  CHECK(std::format("{:[s]}", Padded{5}, "n=") == "n=5");
  CHECK(std::format("{:[s]}", Padded{5}, std::string("v:")) == "v:5");
  // at run time, through vformat
  int w = 5;
  Padded p{9};
  CHECK(std::vformat("{:{}}", std::make_format_args(p, w)) == "    9");
  // a wrong argument kind is detected by format at run time
  std::string s = "x";
  bool thrown = false;
  try {
    (void)std::vformat("{:{}}", std::make_format_args(p, s));
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);
  return 0;
}
