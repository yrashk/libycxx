// FLAGS: -O2 -Wno-builtin-macro-redefined -D__has_cpp_attribute(x)=0
// Exercise the scalar storage path for the power-of-two widths in this regression.
#include "narrow_signed_wrap.pass.cpp"
