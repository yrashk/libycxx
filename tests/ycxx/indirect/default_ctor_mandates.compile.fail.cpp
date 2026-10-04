// [indirect.ctor]/4: indirect(): "Mandates: is_default_constructible_v<T> is true."
#include <memory>

struct NoDefault {
  explicit NoDefault(int) {}
};
std::indirect<NoDefault> x;
