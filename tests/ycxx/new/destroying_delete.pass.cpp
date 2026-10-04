// [new.syn]: struct destroying_delete_t { explicit destroying_delete_t() = default; };
// inline constexpr destroying_delete_t destroying_delete{};
// [expr.delete]/8 and [basic.stc.dynamic.deallocation]/2: a class-specific operator delete
// whose second parameter is std::destroying_delete_t is a destroying operator delete; a
// delete-expression calls it "without first calling the destructor", and it receives the
// most-derived ... pointer of the class type.
#include <new>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

static int dtor_runs = 0;
static int destroying_runs = 0;

struct S {
  int v = 7;
  ~S() { ++dtor_runs; }
  void operator delete(S* p, std::destroying_delete_t) {
    ++destroying_runs;
    if (p->v != 7) return;  // the object is still alive here
    p->~S();
    ::operator delete(p);
  }
};

struct Sized {
  void operator delete(Sized* p, std::destroying_delete_t, std::size_t sz) {
    ++destroying_runs;
    dtor_runs += static_cast<int>(sz == sizeof(Sized));
    ::operator delete(p);
  }
  long payload[4];
};

static_assert(std::is_same_v<std::remove_cv_t<decltype(std::destroying_delete)>, std::destroying_delete_t>);
static_assert(std::is_trivially_copyable_v<std::destroying_delete_t>);
constexpr std::destroying_delete_t tag = std::destroying_delete;

int main() {
  S* s = new S;
  delete s;
  CHECK(destroying_runs == 1 && dtor_runs == 1);  // only the explicit p->~S() ran it
  Sized* z = new Sized;
  delete z;
  CHECK(destroying_runs == 2 && dtor_runs == 2);
  (void)tag;
  return 0;
}
