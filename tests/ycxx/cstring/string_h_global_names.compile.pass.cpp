// [support.c.headers.other]/1: <string.h> places in the global namespace each name <cstring>
// places in std. [cstring.syn] and [library.c] make memchr, strchr, strpbrk, strrchr and strstr
// const-correct overload pairs ("const char* strchr(const char* s, int c); char* strchr(char*
// s, int c);"), so the global names are overloaded the same way. Only <string.h> is included.
// The C23 additions: cstring/string_h_c23_global_names.
#include <string.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::memcmp;
using ::memcpy;
using ::memmove;
using ::memset;
using ::size_t;
using ::strcat;
using ::strcmp;
using ::strcoll;
using ::strcpy;
using ::strcspn;
using ::strerror;
using ::strlen;
using ::strncat;
using ::strncmp;
using ::strncpy;
using ::strspn;
using ::strtok;
using ::strxfrm;

const char* cs = "";
char* s = nullptr;
const void* cv = nullptr;
void* v = nullptr;

static_assert(same<decltype(::memchr(cv, 0, 0)), const void*>);
static_assert(same<decltype(::memchr(v, 0, 0)), void*>);
static_assert(same<decltype(::strchr(cs, 0)), const char*>);
static_assert(same<decltype(::strchr(s, 0)), char*>);
static_assert(same<decltype(::strpbrk(cs, "")), const char*>);
static_assert(same<decltype(::strpbrk(s, "")), char*>);
static_assert(same<decltype(::strrchr(cs, 0)), const char*>);
static_assert(same<decltype(::strrchr(s, 0)), char*>);
static_assert(same<decltype(::strstr(cs, "")), const char*>);
static_assert(same<decltype(::strstr(s, "")), char*>);
static_assert(same<decltype(::strlen("")), ::size_t>);
static_assert(same<decltype(::strerror(0)), char*>);

int main() { return 0; }
