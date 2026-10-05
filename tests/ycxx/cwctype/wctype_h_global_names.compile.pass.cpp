// [support.c.headers.other]/1: <wctype.h> places in the global namespace each name <cwctype>
// places in std ([cwctype.syn]: wint_t, wctrans_t, wctype_t, the isw* functions, iswctype,
// wctype, towlower, towupper, towctrans, wctrans; the macro WEOF). Only <wctype.h> is included.
#include <wctype.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::iswalnum;
using ::iswalpha;
using ::iswblank;
using ::iswcntrl;
using ::iswctype;
using ::iswdigit;
using ::iswgraph;
using ::iswlower;
using ::iswprint;
using ::iswpunct;
using ::iswspace;
using ::iswupper;
using ::iswxdigit;
using ::towctrans;
using ::towlower;
using ::towupper;
using ::wctrans;
using ::wctrans_t;
using ::wctype;
using ::wctype_t;
using ::wint_t;

#ifndef WEOF
#error WEOF
#endif
static_assert(same<decltype(::towupper(L'a')), ::wint_t>);
static_assert(same<decltype(::wctype("alpha")), ::wctype_t>);
static_assert(same<decltype(::wctrans("toupper")), ::wctrans_t>);

int main() { return ::towctrans(L'a', ::wctrans("toupper")) == L'A' ? 0 : 1; }
