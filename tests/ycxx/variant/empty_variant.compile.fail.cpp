// [variant.variant.general]/3: "A program that instantiates the definition of variant with
// no template arguments is ill-formed."
#include <variant>

std::variant<> v;
