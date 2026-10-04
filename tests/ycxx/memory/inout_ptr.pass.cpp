// [inout.ptr.t]/6: inout_ptr_t's constructor initializes p with smart.get() (or smart for
// a raw pointer); /11: the destructor releases the smart pointer (without deleting) and,
// if p is non-null, calls s.reset(static_cast<SP>(p), args...); for a raw pointer Smart
// it assigns s = Smart(static_cast<SP>(p), args...). So a function that frees *pp and
// stores a new value works; a function that frees it and stores null leaves s empty
// without a double delete. [inout.ptr]: inout_ptr<Pointer = void>(s, args...).
#include <memory>
#include <type_traits>
#include "check.hpp"

static int c_frees = 0;
static int del_calls = 0;
struct CDel {
  void operator()(int* p) const {
    ++del_calls;
    delete p;
  }
};

int c_replace(int** pp, int value) {  // frees the old object, stores a new one
  if (*pp) {
    ++c_frees;
    delete *pp;
  }
  *pp = new int(value);
  return 0;
}
int c_destroy(int** pp) {
  ++c_frees;
  delete *pp;
  *pp = nullptr;
  return 0;
}
int c_keep(int** pp) {  // reads the old value, leaves it in place
  return **pp;
}
int c_replace_void(void** pp) {
  delete static_cast<int*>(*pp);
  ++c_frees;
  *pp = new int(55);
  return 0;
}

int main() {
  {
    std::unique_ptr<int, CDel> u(new int(1));
    c_replace(std::inout_ptr(u), 2);
    CHECK(c_frees == 1 && del_calls == 0);  // the smart pointer did not delete the old object
    CHECK(*u == 2);
  }
  CHECK(del_calls == 1);
  {
    std::unique_ptr<int, CDel> u(new int(3));
    c_destroy(std::inout_ptr(u));
    CHECK(!u && c_frees == 2 && del_calls == 1);
  }
  {
    std::unique_ptr<int, CDel> u(new int(4));
    int* raw = u.get();
    CHECK(c_keep(std::inout_ptr(u)) == 4);
    CHECK(u.get() == raw && del_calls == 1);
  }
  CHECK(del_calls == 2);
  {
    std::unique_ptr<int> u(new int(5));
    c_replace_void(std::inout_ptr(u));
    CHECK(*u == 55 && c_frees == 3);
  }
  {
    int* raw = new int(6);
    c_replace(std::inout_ptr(raw), 7);
    CHECK(*raw == 7 && c_frees == 4);
    delete raw;
  }
  {
    std::unique_ptr<int> u;
    c_replace(std::inout_ptr(u), 8);  // null old value is passed in
    CHECK(*u == 8 && c_frees == 4);
  }
  static_assert(std::is_same_v<decltype(std::inout_ptr(std::declval<std::unique_ptr<int>&>())),
                               std::inout_ptr_t<std::unique_ptr<int>, int*>>);
  static_assert(!std::is_copy_constructible_v<std::inout_ptr_t<std::unique_ptr<int>, int*>>);
  return 0;
}
