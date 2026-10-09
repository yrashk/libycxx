// EXPECT-ERROR: error: static assertion failed[^\n]*std::basic_string::resize_and_overwrite: the operation must return an integer\-like type
// [string.capacity]/8: resize_and_overwrite: "Mandates: OP has an integer-like type
// ([iterator.concept.winc])." An operation returning a pointer is ill-formed.
#include <string>
#include <cstddef>

void f(std::string& s) {
  s.resize_and_overwrite(4, [](char* p, std::size_t) { return p; });
}
