// [func.wrap.ref.class]: "template<class T> function_ref& operator=(T) = delete;"
// [func.wrap.ref.ctor]/21: constrained on T not being a function_ref convertible from a
// specialization, not a pointer and not a constant_wrapper -- so assigning a callable object
// selects the deleted overload (it would dangle).
#include <functional>

struct Fn {
  int operator()() const { return 0; }
};

void test(std::function_ref<int()>& r) {
  Fn fn;
  r = fn;
}
