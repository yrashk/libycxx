// Both compilers predeclare std::align_val_t with the implicit global allocation functions
// ([basic.stc.dynamic.general]/2): a std::__y1::align_val_t makes the name ambiguous.
#ifdef PLAIN
namespace std { enum class align_val_t : decltype(sizeof 0) {}; }
#else
namespace std { inline namespace __y1 { enum class align_val_t : decltype(sizeof 0) {}; } }
#endif
void* operator new(decltype(sizeof 0), std::align_val_t);
void* operator new(decltype(sizeof 0) n, std::align_val_t) { alignas(64) static char buf[256]; return n <= 256 ? buf : nullptr; }
void operator delete(void*, std::align_val_t) noexcept {}
void operator delete(void*, decltype(sizeof 0), std::align_val_t) noexcept {}
struct alignas(64) Over { char c[64]; };
int main() { Over* o = new Over; delete o; return o ? 0 : 1; }
