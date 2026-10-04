// [unique.ptr.single.observers]/1-2: operator*: "Mandates:
// reference_converts_from_temporary_v<add_lvalue_reference_t<T>, decltype(*declval<pointer>())>
// is false." Here *pointer yields a prvalue int, which const int& would bind to a temporary.
#include <memory>
#include <cstddef>

struct ValuePtr {
  int* p = nullptr;
  ValuePtr() = default;
  ValuePtr(std::nullptr_t) {}
  int operator*() const { return *p; }  // by value
  explicit operator bool() const { return p != nullptr; }
  friend bool operator==(ValuePtr, ValuePtr) = default;
};
struct Del {
  using pointer = ValuePtr;
  void operator()(ValuePtr) const {}
};

int main() {
  std::unique_ptr<const int, Del> u;
  (void)*u;
}
