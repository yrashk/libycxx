// [util.smartptr.shared.const]/3: template<class Y> explicit shared_ptr(Y* p) is constrained on
// "the expression delete p is well-formed and Y* is convertible to T*" (T not an array). Deleting
// a void* is ill-formed ([expr.delete]/2: the operand's type is a pointer to object type...
// "cv void" is not an object type), so shared_ptr<void> and shared_ptr<const void> are not
// constructible from a void* (or const void*) alone, while they are from an int* (the
// shared_ptr then deletes the int), and from a void* with a deleter (/9: d(p) well-formed).
// reset(Y*) is "Equivalent to shared_ptr(p).swap(*this)" ([util.smartptr.shared.mod]/3).
// XFAIL-COMPILER: gcc  GCC 16.2 treats "delete p" for a void* as well-formed (a warning) in a
// requires-expression, so the constraint is satisfied (STATUS.md, known compiler gaps)
// COUNTERPART: libstdcxx:20_util/shared_ptr/modifiers/reset_sfinae.cc
#include <memory>
#include <type_traits>

struct void_deleter {
  void operator()(void*) const {}
};

static_assert(!std::is_constructible_v<std::shared_ptr<void>, void*>);
static_assert(!std::is_constructible_v<std::shared_ptr<const void>, void*>);
static_assert(!std::is_constructible_v<std::shared_ptr<const void>, const void*>);
static_assert(std::is_constructible_v<std::shared_ptr<void>, int*>);
static_assert(std::is_constructible_v<std::shared_ptr<const void>, int*>);
static_assert(std::is_constructible_v<std::shared_ptr<const void>, const int*>);
static_assert(std::is_constructible_v<std::shared_ptr<void>, void*, void_deleter>);
static_assert(std::is_constructible_v<std::shared_ptr<const void>, void*, void_deleter>);
static_assert(!std::is_convertible_v<int*, std::shared_ptr<void>>);  // explicit

int main() {
  std::shared_ptr<const void> p(new int(4));
  p.reset(new long(5));
  int x = 0;
  std::shared_ptr<void> q(static_cast<void*>(&x), void_deleter{});
  q.reset(&x, void_deleter{});
  return p && q ? 0 : 1;
}
