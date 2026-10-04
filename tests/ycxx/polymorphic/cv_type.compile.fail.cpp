// [polymorphic.general]/5: "A program that instantiates the definition of polymorphic for ... a
// cv-qualified type is ill-formed."
#include <memory>

struct B {
  virtual ~B() = default;
};
std::polymorphic<const B> x;
