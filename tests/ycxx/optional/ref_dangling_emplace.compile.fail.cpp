// [optional.ref.assign]/4.2: emplace is constrained on
// reference_constructs_from_temporary_v<T&, U> being false.
#include <optional>

void f() {
  std::optional<const int&> o;
  o.emplace(5);
}
