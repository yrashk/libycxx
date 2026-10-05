// [string.op.append]: operator+= for basic_string, string-view-like T, const charT*, charT,
// initializer_list. [string.append]: append(str), append(str, pos, n), append(t),
// append(t, pos, n), append(s, n), append(s), append(s, pos, n), append(n, c),
// append(first, last), append_range(rg), append(il), push_back(c). All forms return *this
// (push_back returns void). The pos forms go through substr and so throw out_of_range when
// pos > size().
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::string&>().push_back('a')), void>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>() += 'a'), std::string&>);

constexpr bool test() {
  std::string s;
  const std::string tail = "tail";
  if (&(s += tail) != &s || s != "tail") return false;
  s += std::string_view("-sv");
  s += "-ptr";
  s += '!';
  s += {'{', '}'};
  if (s != "tail-sv-ptr!{}") return false;

  std::string t = "x";
  if (&t.append(tail) != &t) return false;
  t.append(tail, 1, 2);
  t.append(tail, 3);
  t.append(tail, 4);  // pos == size(): appends nothing
  if (t != "xtailail") return false;
  t = "";
  t.append(std::string_view("view"));
  t.append(std::string_view("view"), 2);
  t.append(std::string_view("view"), 0, 1);
  if (t != "viewewv") return false;
  t = "";
  t.append("ab\0cd", 5);
  if (t.size() != 5 || t[4] != 'd') return false;
  t.append("ef");
  t.append("ghijk", 1, 2);
  if (t.size() != 9 || t.substr(5) != "efhi") return false;
  t = "";
  t.append(3, 'z');
  t.append(0, 'y');
  t.append({'1', '2'});
  if (t != "zzz12") return false;
  char buf[] = "iter";
  t.append(InputIter<char>(buf), InputIter<char>(buf + 4));
  t.append(buf, buf + 2);
  if (t != "zzz12iterit") return false;
  t = "";
  t.append_range(InputRange<char>{buf, buf + 3});
  if (&t.append_range(std::string_view("+")) != &t) return false;
  t.push_back('$');
  if (t != "ite+$") return false;

  // Appending to itself, growing past any small-buffer size.
  std::string g = "ab";
  for (int i = 0; i < 6; ++i) g.append(g);
  if (g.size() != 128) return false;
  for (std::size_t i = 0; i < g.size(); ++i)
    if (g[i] != (i % 2 ? 'b' : 'a')) return false;
  std::string h = "0123456789";
  h.append(h.data() + 2, 5);
  if (h != "012345678923456") return false;
  h.append(h, 0, 3);
  if (h != "012345678923456012") return false;
  std::string k = "abc";
  k += k;
  if (k != "abcabc") return false;
  for (int i = 0; i < 100; ++i) k.push_back(static_cast<char>('a' + i % 26));
  if (k.size() != 106 || k[105] != static_cast<char>('a' + 99 % 26) || k.c_str()[106] != '\0') return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  bool threw = false;
  try {
    s.append(std::string("xy"), 3);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw && s == "abc");
  threw = false;
  try {
    s.append("xy", 3, 1);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw && s == "abc");
  return 0;
}
