// EXPECT-ERROR-GCC: error: no matching function.*std::optional<const int&>::emplace\(int\)
// EXPECT-ERROR-CLANG: error: no matching member function for call to 'emplace'
// [optional.ref.assign]/4.2: emplace is constrained on
// reference_constructs_from_temporary_v<T&, U> being false.
#include <optional>

void f() {
  std::optional<const int&> o;
  o.emplace(5);
}
