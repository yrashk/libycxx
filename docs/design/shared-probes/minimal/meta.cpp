// FLAGS: -freflection
// COMPILERS: gcc
// GCC 16 predeclares namespace std::meta with -freflection and evaluates the metafunctions
// declared there (DECISIONS §13); a std::__y1::meta makes `meta` ambiguous inside std.
#ifdef PLAIN
namespace std { namespace meta {
#else
namespace std { inline namespace __y1 { namespace meta {
#endif
using info = decltype(^^int);
consteval bool is_class_type(info);
#ifdef PLAIN
}}
#else
}}}
#endif
namespace std { consteval bool probe() { return meta::is_class_type(^^int); } }
struct S {};
int main() { return std::meta::is_class_type(^^S) && !std::probe() ? 0 : 1; }
