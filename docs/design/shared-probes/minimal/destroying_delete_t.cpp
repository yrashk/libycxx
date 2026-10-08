// A destroying operator delete names std::destroying_delete_t; GCC recognises it only in plain std.
#ifdef PLAIN
namespace std { struct destroying_delete_t { explicit destroying_delete_t() = default; }; }
#else
namespace std { inline namespace __y1 { struct destroying_delete_t { explicit destroying_delete_t() = default; }; } }
#endif
static int called;
struct D {
  static D storage;
  void operator delete(D* p, std::destroying_delete_t) { p->~D(); ++called; }
};
D D::storage;
int main() { delete &D::storage; return called == 1 ? 0 : 1; }
