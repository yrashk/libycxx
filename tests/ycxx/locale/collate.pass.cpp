// [locale.collate]: collate<char> in the classic locale compares lexicographically by char
// value (compare returns -1, 0, 1), transform returns a string whose lexicographic comparison
// matches compare, and hash gives equal values for strings that compare equal.
#include <locale>
#include <string>
#include "check.hpp"

int main() {
  const auto& co = std::use_facet<std::collate<char>>(std::locale::classic());
  const char a[] = "apple", b[] = "apricot", A[] = "Apple";
  CHECK(co.compare(a, a + 5, b, b + 7) == -1);
  CHECK(co.compare(b, b + 7, a, a + 5) == 1);
  CHECK(co.compare(a, a + 5, a, a + 5) == 0);
  CHECK(co.compare(A, A + 5, a, a + 5) == -1);  // 'A' < 'a' in "C"
  CHECK(co.compare(a, a + 2, a, a + 5) == -1);  // prefix
  std::string ta = co.transform(a, a + 5), tb = co.transform(b, b + 7);
  CHECK(ta < tb);
  CHECK(co.hash(a, a + 5) == co.hash(a, a + 5));
  const std::string a2 = "apple";
  CHECK(co.hash(a2.data(), a2.data() + 5) == co.hash(a, a + 5));
  const auto& wco = std::use_facet<std::collate<wchar_t>>(std::locale::classic());
  const wchar_t wx[] = L"x", wy[] = L"y";
  CHECK(wco.compare(wx, wx + 1, wy, wy + 1) == -1);
  return 0;
}
