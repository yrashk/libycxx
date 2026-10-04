// [class.cdtor]/5: when typeid is used in a constructor or destructor (or a function called
// from one) and "the operand of typeid refers to the object under construction or
// destruction, typeid yields the std::type_info object representing the constructor or
// destructor's class." /6: for dynamic_cast, "this object is considered to be a most derived
// object that has the type of the constructor or destructor's class." So casts to the more
// derived class fail during base construction/destruction.
#include <typeinfo>
#include "check.hpp"

struct Derived;
struct Base;
static bool type_was_base_in_ctor = false, type_was_base_in_dtor = false;
static bool cast_null_in_ctor = false, cast_null_in_dtor = false, void_is_base = false;
static bool helper_saw_base = false;

static void helper(Base* b);
static bool cast_to_derived_is_null(Base* b);

struct Base {
  Base() {
    type_was_base_in_ctor = typeid(*this) == typeid(Base);
    cast_null_in_ctor = cast_to_derived_is_null(this);
    void_is_base = dynamic_cast<void*>(this) == static_cast<void*>(this);
    helper(this);
  }
  virtual ~Base() {
    type_was_base_in_dtor = typeid(*this) == typeid(Base);
    cast_null_in_dtor = cast_to_derived_is_null(this);
  }
};

struct Pad {
  long pad[4] = {};
  virtual ~Pad() = default;
};

struct Derived : Pad, Base {
  bool in_derived_ctor = false;
  Derived() { in_derived_ctor = typeid(*static_cast<Base*>(this)) == typeid(Derived); }
};

// called (indirectly) from Base's constructor/destructor: the operand refers to the object
// under construction or destruction
static bool cast_to_derived_is_null(Base* b) { return dynamic_cast<Derived*>(b) == nullptr; }
static void helper(Base* b) { helper_saw_base = typeid(*b) == typeid(Base); }

int main() {
  {
    Derived d;
    CHECK(d.in_derived_ctor);
    Base* b = &d;
    CHECK(typeid(*b) == typeid(Derived));
    CHECK(dynamic_cast<Derived*>(b) == &d);
  }
  CHECK(type_was_base_in_ctor);
  CHECK(type_was_base_in_dtor);
  CHECK(cast_null_in_ctor);
  CHECK(cast_null_in_dtor);
  CHECK(void_is_base);  // the Base subobject is "most derived" while Base() runs
  CHECK(helper_saw_base);
  return 0;
}
