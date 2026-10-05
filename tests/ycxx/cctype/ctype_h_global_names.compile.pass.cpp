// [support.c.headers.other]/1: <ctype.h> places in the global namespace each name <cctype>
// places in std ([cctype.syn]: isalnum, isalpha, isblank, iscntrl, isdigit, isgraph, islower,
// isprint, ispunct, isspace, isupper, isxdigit, tolower, toupper, each int(int)).
// Only <ctype.h> is included. (<wctype.h>: cwctype/wctype_h_global_names.)
#include <ctype.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::isalnum;
using ::isalpha;
using ::isblank;
using ::iscntrl;
using ::isdigit;
using ::isgraph;
using ::islower;
using ::isprint;
using ::ispunct;
using ::isspace;
using ::isupper;
using ::isxdigit;
using ::tolower;
using ::toupper;

static_assert(same<decltype(::isblank(' ')), int> && same<decltype(::toupper('a')), int>);

int main() { return (::isdigit('7') && ::toupper('a') == 'A') ? 0 : 1; }
