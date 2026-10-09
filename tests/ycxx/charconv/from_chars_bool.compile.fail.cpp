// EXPECT-ERROR-GCC: error: no matching function for call to 'from_chars\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'from_chars'
// [charconv.syn]: from_chars is provided for integer-type, i.e. "char and all signed and
// unsigned integer types", and for floating-point types; bool is neither, so there is no
// from_chars(const char*, const char*, bool&).
#include <charconv>

int main() {
  const char s[] = "1";
  bool b = false;
  auto r = std::from_chars(s, s + 1, b);
  return r.ptr == s;
}
