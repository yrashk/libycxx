// EXPECT-ERROR: error: static assertion failed[^\n]*std::polymorphic: T must be a cv\-unqualified object type that is not an array, in_place_t or a specialization of in_place_type_t
// [polymorphic.general]/5: "A program that instantiates the definition of polymorphic for ... a
// cv-qualified type is ill-formed."
#include <memory>

struct B {
  virtual ~B() = default;
};
std::polymorphic<const B> x;
