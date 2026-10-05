// [polymorphic.ctor]/4: polymorphic(): "Mandates: is_default_constructible_v<T> is true, and
// is_copy_constructible_v<T> is true." T is default constructible but not copyable.
// EXPECT-ERROR: static assertion failed.*std::polymorphic\(\): T must be default and copy constructible
#include <memory>

struct NoCopy {
  NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
};
std::polymorphic<NoCopy> x;
