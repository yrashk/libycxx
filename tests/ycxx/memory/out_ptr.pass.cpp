// [out.ptr.t]/6: out_ptr_t's constructor resets the smart pointer (s.reset() if
// well-formed, else s = Smart()) and value-initializes p; /9: its destructor, if p is
// non-null, calls s.reset(static_cast<SP>(p), args...) (or s = Smart(static_cast<SP>(p),
// args...)); /12: operator Pointer*() returns the address of p; /14-15: operator void**()
// lets a void** API store into p. [out.ptr]: out_ptr<Pointer = void>(s, args...) returns
// out_ptr_t<Smart, P, Args&&...> where P is POINTER_OF(Smart) when Pointer is void.
#include <memory>
#include <type_traits>
#include "check.hpp"

static int deleted = 0;
struct CDel {
  void operator()(int* p) const {
    ++deleted;
    delete p;
  }
};

int c_create(int** out, int value) {
  *out = new int(value);
  return 0;
}
int c_fail(int** out) {
  *out = nullptr;
  return 1;
}
int c_create_void(void** out) {
  *out = new int(77);
  return 0;
}

int main() {
  {
    std::unique_ptr<int, CDel> u(new int(1));
    int err = c_create(std::out_ptr(u), 42);
    CHECK(err == 0);
    CHECK(deleted == 1);  // the old value was reset away by the constructor
    CHECK(u && *u == 42);
  }
  CHECK(deleted == 2);
  {
    std::unique_ptr<int, CDel> u(new int(1));
    int err = c_fail(std::out_ptr(u));
    CHECK(err == 1);
    CHECK(!u);  // reset in the constructor; null p leaves it empty
    CHECK(deleted == 3);
  }
  {
    std::unique_ptr<int> u;
    c_create_void(std::out_ptr(u));  // operator void**
    CHECK(u && *u == 77);
  }
  {
    std::unique_ptr<int> u;
    c_create(std::out_ptr<int*>(u), 5);  // explicit Pointer
    CHECK(*u == 5);
  }
  {
    // shared_ptr with a deleter argument forwarded to reset.
    std::shared_ptr<int> s;
    c_create(std::out_ptr(s, CDel{}), 9);
    CHECK(s && *s == 9);
    int before = deleted;
    s.reset();
    CHECK(deleted == before + 1);
  }
  {
    // out_ptr returns out_ptr_t<Smart, P, Args&&...>.
    std::unique_ptr<int> u;
    static_assert(std::is_same_v<decltype(std::out_ptr(u)), std::out_ptr_t<std::unique_ptr<int>, int*>>);
    static_assert(std::is_same_v<decltype(std::out_ptr<int*>(u)), std::out_ptr_t<std::unique_ptr<int>, int*>>);
    CDel d;
    std::shared_ptr<int> s;
    static_assert(std::is_same_v<decltype(std::out_ptr(s, d)), std::out_ptr_t<std::shared_ptr<int>, int*, CDel&>>);
    static_assert(!std::is_copy_constructible_v<std::out_ptr_t<std::unique_ptr<int>, int*>>);
  }
  return 0;
}
