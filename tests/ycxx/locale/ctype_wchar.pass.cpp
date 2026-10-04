// [locale.ctype], [locale.ctype.members]: ctype<wchar_t> in the classic locale classifies
// the basic characters like "C"; toupper/tolower convert; widen maps char to wchar_t and narrow
// maps back (dfault for characters with no narrow form); scan_is/scan_not search ranges.
#include <locale>
#include <cwchar>
#include "check.hpp"

int main() {
  const std::ctype<wchar_t>& ct = std::use_facet<std::ctype<wchar_t>>(std::locale::classic());
  using B = std::ctype_base;
  CHECK(ct.is(B::alpha, L'a') && ct.is(B::digit, L'5') && ct.is(B::space, L' ') && !ct.is(B::digit, L'x'));
  CHECK(ct.toupper(L'a') == L'A' && ct.tolower(L'B') == L'b');
  CHECK(ct.widen('q') == L'q' && ct.narrow(L'q', '?') == 'q');
  wchar_t w[3];
  const char in[] = "ok";
  CHECK(ct.widen(in, in + 2, w) == in + 2 && w[0] == L'o' && w[1] == L'k');
  char n[2];
  const wchar_t win[] = L"hi";
  CHECK(ct.narrow(win, win + 2, '?', n) == win + 2 && n[0] == 'h' && n[1] == 'i');
  wchar_t up[] = L"mixed Case";
  ct.toupper(up, up + 10);
  CHECK(std::wcscmp(up, L"MIXED CASE") == 0);
  const wchar_t s[] = L"ab 12";
  CHECK(ct.scan_is(B::digit, s, s + 5) == s + 3 && ct.scan_not(B::alpha, s, s + 5) == s + 2);
  B::mask m[2];
  ct.is(L"A1", L"A1" + 2, m);
  CHECK((m[0] & B::upper) && (m[1] & B::digit));
  return 0;
}
