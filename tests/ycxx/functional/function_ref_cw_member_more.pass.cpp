// [func.wrap.ref.ctor]/13-16: function_ref(constant_wrapper<c, F> f, U&& obj): "Let T be
// remove_reference_t<U>" ... "Constraints: is_rvalue_reference_v<U&&> is false, and
// is-invocable-using<const F&, cv T&> is true." "Effects: Initializes bound-entity with
// addressof(obj), and thunk-ptr with the address of a function thunk such that
// thunk(bound-entity, call-args...) is expression-equivalent to invoke_r<R>(f.value,
// static_cast<cv T&>(obj), call-args...)." /17-20: the cv T* form calls invoke_r<R>(f.value,
// obj, call-args...). [func.require]/1: INVOKE of a pointer to member function with an object
// of a derived class, or a pointer to one, uses that object; an explicit object member
// function is an ordinary function pointer whose first parameter receives the bound object.
#include <functional>
#include <type_traits>
#include "check.hpp"

struct Base {
  int b = 1;
  int get_b() const { return b; }
  virtual int vf() const { return 10; }
  virtual ~Base() = default;
};
struct Derived : Base {
  int vf() const override { return 20; }
};
struct Self {
  int v;
  int bump(int k) { return v += k; }
  int nx() const noexcept { return v; }
};

int main() {
  Derived d;
  d.b = 7;
  // a base-class member function bound to a derived object, by reference and by pointer
  std::function_ref<int()> r1(std::cw<&Base::get_b>, d);
  std::function_ref<int()> r2(std::cw<&Base::get_b>, &d);
  CHECK(r1() == 7 && r2() == 7);
  // virtual dispatch through the bound object
  std::function_ref<int() const> r3(std::cw<&Base::vf>, d);
  const Base* pb = &d;
  std::function_ref<int()> r4(std::cw<&Base::vf>, pb);
  CHECK(r3() == 20 && r4() == 20);
  // a data member of a base class
  std::function_ref<int&()> r5(std::cw<&Base::b>, d);
  r5() = 9;
  CHECK(d.b == 9);

  Self s{7};
  // noexcept signature with a noexcept member function
  std::function_ref<int() noexcept> n1(std::cw<&Self::nx>, s);
  static_assert(noexcept(n1()));
  CHECK(n1() == 7);
  // the call operator is const: a const function_ref is callable
  std::function_ref<int(int)> bump(std::cw<&Self::bump>, s);
  const std::function_ref<int(int)>& cb = bump;
  cb(1);
  CHECK(s.v == 8);
  // copying a bound function_ref keeps the binding
  std::function_ref<int()> copy = n1;
  s.v = 1;
  CHECK(copy() == 1);
  return 0;
}
