// [format.functions]/13-17: format_to(out, fmt, args...) and vformat_to(out, fmt, args) accept
// any Out satisfying output_iterator<const charT&> -- pointers, back_insert_iterator over
// string, vector, deque, list, ostreambuf_iterator, a program-defined proxy iterator and a
// move-only one (output_iterator does not require copyability, [iterator.concept.output],
// [iterator.concept.winc]) -- write exactly the N characters of the representation into
// [out, out + N) and return out + N. /19-23: format_to_n writes the first M = clamp(n, 0, N)
// characters and returns {out + M, N}, also for a move-only iterator. Also with wchar_t.
#include <cstddef>
#include <deque>
#include <format>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

// A move-only output iterator appending to a string through a proxy reference.
struct MoveOnlyOut {
  using difference_type = std::ptrdiff_t;
  std::string* s = nullptr;
  int* increments = nullptr;
  explicit MoveOnlyOut(std::string* p, int* inc) : s(p), increments(inc) {}
  MoveOnlyOut(MoveOnlyOut&&) = default;
  MoveOnlyOut& operator=(MoveOnlyOut&&) = default;
  struct Ref {
    std::string* s;
    const Ref& operator=(char c) const {
      s->push_back(c);
      return *this;
    }
    const Ref& operator*() const { return *this; }
  };
  Ref operator*() const { return {s}; }
  MoveOnlyOut& operator++() {
    ++*increments;
    return *this;
  }
  Ref operator++(int) {
    ++*increments;
    return {s};
  }
};
static_assert(std::output_iterator<MoveOnlyOut, const char&>);
static_assert(!std::copyable<MoveOnlyOut>);

// A copyable output iterator that is not a pointer, writing into a fixed buffer.
struct BufOut {
  using difference_type = std::ptrdiff_t;
  char* p = nullptr;
  char& operator*() const { return *p; }
  BufOut& operator++() {
    ++p;
    return *this;
  }
  BufOut operator++(int) {
    BufOut t = *this;
    ++p;
    return t;
  }
};
static_assert(std::output_iterator<BufOut, const char&>);

int main() {
  {  // pointer: exactly N characters, returns out + N
    char buf[16];
    for (char& c : buf) c = '#';
    char* e = std::format_to(buf, "{}-{:>3}", 12, 'x');
    CHECK(e == buf + 6 && std::string(buf, e) == "12-  x" && buf[6] == '#');
    int v = 7;
    e = std::vformat_to(buf, "<{}>", std::make_format_args(v));
    CHECK(e == buf + 3 && std::string(buf, e) == "<7>");
  }
  {  // back_insert_iterator over several containers
    std::string s = "a";
    auto it = std::format_to(std::back_inserter(s), "{}{}", 'b', 3);
    *it = '!';
    CHECK(s == "ab3!");
    std::vector<char> v;
    std::format_to(std::back_inserter(v), "{:*^5}", "x");
    CHECK(std::string(v.begin(), v.end()) == "**x**");
    std::deque<char> d;
    std::format_to(std::back_inserter(d), "{}", 1.5);
    CHECK(std::string(d.begin(), d.end()) == "1.5");
    std::list<char> l;
    std::format_to(std::front_inserter(l), "{}", "abc");  // front insertion reverses
    CHECK(std::string(l.begin(), l.end()) == "cba");
  }
  {  // ostreambuf_iterator
    std::ostringstream os;
    std::format_to(std::ostreambuf_iterator<char>(os), "{:#x}", 255);
    CHECK(os.str() == "0xff");
  }
  {  // program-defined iterators
    char buf[8] = {};
    BufOut o = std::format_to(BufOut{buf}, "{}", 42);
    CHECK(o.p == buf + 2 && std::string(buf) == "42");
    std::string s;
    int incs = 0;
    MoveOnlyOut m = std::format_to(MoveOnlyOut(&s, &incs), "{} {}", "move", 1);
    CHECK(s == "move 1" && m.s == &s);
    std::string s2;
    MoveOnlyOut m2 = std::vformat_to(MoveOnlyOut(&s2, &incs), "[{}]", std::make_format_args(s));
    CHECK(s2 == "[move 1]" && m2.s == &s2);
  }
  {  // format_to_n
    char buf[8];
    for (char& c : buf) c = '#';
    auto r = std::format_to_n(buf, 3, "{}", 123456);
    CHECK(r.out == buf + 3 && r.size == 6 && std::string(buf, 4) == "123#");
    std::string s;
    int incs = 0;
    std::format_to_n_result<MoveOnlyOut> mr = std::format_to_n(MoveOnlyOut(&s, &incs), 4, "{}", "abcdefg");
    CHECK(s == "abcd" && mr.size == 7 && mr.out.s == &s);
    std::string t;
    auto all = std::format_to_n(std::back_inserter(t), 100, "{}{}", 'x', 'y');
    CHECK(t == "xy" && all.size == 2);
  }
  {  // wide characters
    std::wstring w;
    std::format_to(std::back_inserter(w), L"{}:{}", 1, L"z");
    CHECK(w == L"1:z");
    wchar_t wb[4] = {};
    auto wr = std::format_to_n(wb, 2, L"{}", 987);
    CHECK(wr.out == wb + 2 && wr.size == 3 && wb[0] == L'9' && wb[1] == L'8' && wb[2] == 0);
  }
  return 0;
}
