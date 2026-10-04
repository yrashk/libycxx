// basic_string operations whose source is (part of) the string itself, including when the
// operation reallocates. Nothing in the draft restricts the source of these operations to be
// outside *this:
// [string.append]: append(const basic_string& str) == append(str.data(), str.size());
//   append(str, pos, n); append(const T& t) via basic_string_view; append(const charT* s,
//   size_type n): "Appends a copy of the range [s, s + n) to the string"; append(const charT*
//   s); append(InputIterator first, last): "Equivalent to: return append(basic_string(first,
//   last, get_allocator()));" append_range(rg): "Equivalent to: return append(basic_string(
//   from_range, std::forward<R>(rg), get_allocator()));" operator+= likewise.
// [string.insert]: insert(pos, str), insert(pos1, str, pos2, n), insert(pos, s, n): "Inserts a
//   copy of the range [s, s + n) immediately before the character at position pos";
//   insert(p, first, last) and insert_range(p, rg): equivalent to inserting a temporary
//   basic_string built from the range.
// [string.replace]: replace(pos1, n1, s, n2): "replaces the characters in the range
//   [begin() + pos1, begin() + pos1 + xlen) with a copy of the range [s, s + n2)";
//   replace(i1, i2, j1, j2) and replace_with_range: via a temporary basic_string.
// [string.assign]: assign(s, n): "Replaces the string controlled by *this with a copy of the
//   range [s, s + n)"; assign(str), assign(str, pos, n); assign_range via a temporary.
// The expected contents are computed on a separate plain character array.
#include <string>
#include <cstring>
#include <ranges>
#include <string_view>
#include "check.hpp"

struct Ref {
  char buf[8192];
  std::size_t n = 0;
  void set(const std::string& s) {
    std::memcpy(buf, s.data(), s.size());
    n = s.size();
  }
  // replace [pos, pos + len) with a copy of src[0, m) (src copied first)
  void replace(std::size_t pos, std::size_t len, const char* src, std::size_t m) {
    char tmp[8192];
    std::memcpy(tmp, src, m);
    std::memmove(buf + pos + m, buf + pos + len, n - pos - len);
    std::memcpy(buf + pos, tmp, m);
    n = n - len + m;
  }
  bool equals(const std::string& s) const {
    return s.size() == n && std::memcmp(s.data(), buf, n) == 0 && s.c_str()[n] == '\0';
  }
};

// A string whose size equals its capacity, so growing it must reallocate.
std::string full(std::size_t at_least) {
  std::string s;
  s.reserve(at_least);
  for (std::size_t i = 0; s.size() < s.capacity(); ++i) s.push_back(static_cast<char>('a' + i % 26));
  return s;
}

template <class Op>
void check(std::size_t len, Op op) {
  std::string s = full(len);
  CHECK(s.size() == s.capacity());
  Ref r;
  r.set(s);
  op(s, r);
  CHECK(r.equals(s));
}

void run(std::size_t len) {
  // append
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s.append(s); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s += s; });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf + 1, 3); s.append(s, 1, 3); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s.append(s.data(), s.size()); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf + 2, r.n - 2); s.append(s.c_str() + 2); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf + 2, r.n - 2); s += s.c_str() + 2; });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s.append(std::string_view(s)); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s.append(s.begin(), s.end()); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf + 1, r.n - 1); s.append_range(std::string_view(s).substr(1)); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n, 0, r.buf, r.n); s.append_range(s); });
  // insert
  check(len, [](std::string& s, Ref& r) { r.replace(0, 0, r.buf, r.n); s.insert(0, s); });
  check(len, [](std::string& s, Ref& r) { r.replace(3, 0, r.buf, r.n); s.insert(3, s); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, 0, r.buf + 1, r.n - 1); s.insert(0, s.c_str() + 1); });
  check(len, [](std::string& s, Ref& r) { r.replace(2, 0, r.buf + 1, 3); s.insert(2, s.data() + 1, 3); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 0, r.buf + 2, 4); s.insert(1, s, 2, 4); });
  check(len, [](std::string& s, Ref& r) { r.replace(r.n - 1, 0, r.buf, r.n); s.insert(s.size() - 1, std::string_view(s)); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 0, r.buf, r.n); s.insert(s.begin() + 1, s.begin(), s.end()); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 0, r.buf, r.n); s.insert_range(s.begin() + 1, s); });
  // replace: growing, shrinking, same length; overlapping in both directions
  check(len, [](std::string& s, Ref& r) { r.replace(0, 2, r.buf + 1, 5); s.replace(0, 2, s.data() + 1, 5); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 5, r.buf + 3, 2); s.replace(1, 5, s.data() + 3, 2); });
  check(len, [](std::string& s, Ref& r) { r.replace(4, 3, r.buf + 2, 3); s.replace(4, 3, s.data() + 2, 3); });
  check(len, [](std::string& s, Ref& r) { r.replace(2, 3, r.buf + 4, 3); s.replace(2, 3, s.data() + 4, 3); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 3, r.buf, r.n); s.replace(1, 3, s); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 1, 2); s.replace(0, s.size(), s, 1, 2); });
  check(len, [](std::string& s, Ref& r) { r.replace(3, 1, r.buf + 1, r.n - 1); s.replace(3, 1, s.c_str() + 1); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 2, r.buf, r.n); s.replace(s.begin() + 1, s.begin() + 3, s.begin(), s.end()); });
  check(len, [](std::string& s, Ref& r) { r.replace(1, 2, r.buf, r.n); s.replace_with_range(s.begin() + 1, s.begin() + 3, s); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, 1, r.buf + 2, 7); s.replace(s.begin(), s.begin() + 1, std::string_view(s).substr(2, 7)); });
  // assign
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 2, 3); s.assign(s.data() + 2, 3); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 1, r.n - 1); s.assign(s.c_str() + 1); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 1, r.n - 1); s = s.c_str() + 1; });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf, r.n); s.assign(s); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 1, 2); s.assign(s, 1, 2); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 3, r.n - 3); s.assign(s.begin() + 3, s.end()); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 3, r.n - 3); s.assign_range(std::string_view(s).substr(3)); });
  check(len, [](std::string& s, Ref& r) { r.replace(0, r.n, r.buf + 1, 4); s.assign(std::string_view(s).substr(1, 4)); });
  // self copy and move assignment leave a valid string; self copy assignment keeps the value
  // ([string.cons]/28: operator=(const basic_string&) "If *this and str are the same object,
  // has no effect")
  check(len, [](std::string& s, Ref&) {
    std::string& alias = s;
    s = alias;
  });
}

int main() {
  run(8);     // fits in any small buffer
  run(15);
  run(40);
  run(1000);  // heap storage
  return 0;
}
