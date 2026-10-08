// FLAGS: -O2
// std::byte may alias any object ([basic.lval]/11.3). GCC knows the type by its name in plain std:
// in std::__y1 the store through b is assumed not to modify *i, and f returns 1.
#ifdef PLAIN
namespace std { enum class byte : unsigned char {}; }
#else
namespace std { inline namespace __y1 { enum class byte : unsigned char {}; } }
#endif
[[gnu::noinline]] int f(int* i, std::byte* b) { *i = 1; *b = std::byte{2}; return *i; }
int main() { int x = 0; return f(&x, reinterpret_cast<std::byte*>(&x)) != 1 ? 0 : 1; }
