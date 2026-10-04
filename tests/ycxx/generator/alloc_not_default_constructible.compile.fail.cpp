// [coro.generator.promise]: "void* operator new(size_t size) requires same_as<Allocator,
// void> || default_initializable<Allocator>;" -- with a non-default-constructible Allocator
// and no allocator_arg_t, alloc parameters, no promise operator new is viable for the
// coroutine, so the coroutine is ill-formed ([dcl.fct.def.coroutine]/9: "If no viable
// function is found ..., the program is ill-formed" once allocation functions are found in
// the promise's scope). Control (-DYCXX_CONTROL): the allocator is passed after
// allocator_arg, selecting the second overload.
#include <generator>
#include <cstddef>
#include <memory>

template <class T>
struct NoDefault {
  using value_type = T;
  explicit NoDefault(int) {}
  template <class U>
  NoDefault(const NoDefault<U>&) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const NoDefault&, const NoDefault<U>&) { return true; }
};

#ifdef YCXX_CONTROL
std::generator<int, void, NoDefault<int>> g(std::allocator_arg_t, NoDefault<int>, int n) {
  for (int i = 0; i < n; ++i) co_yield i;
}
int main() {
  int s = 0;
  for (int x : g(std::allocator_arg, NoDefault<int>(1), 3)) s += x;
  return s == 3 ? 0 : 1;
}
#else
std::generator<int, void, NoDefault<int>> g(int n) {
  for (int i = 0; i < n; ++i) co_yield i;
}
int main() {
  int s = 0;
  for (int x : g(3)) s += x;
  return s;
}
#endif
