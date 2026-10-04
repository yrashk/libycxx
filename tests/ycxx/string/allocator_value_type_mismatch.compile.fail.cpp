// [string.require]/3 note 1: "The program is ill-formed if Allocator::value_type is not the
// same type as charT." [container.alloc.reqmts]/5: "Mandates: allocator_type::value_type is
// the same as X::value_type."
#include <string>
#include <memory>

std::basic_string<char, std::char_traits<char>, std::allocator<int>> s;
