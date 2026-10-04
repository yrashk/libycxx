// [mdspan.accessor.aligned.overview]/1: "Mandates: byte_alignment is a power of two, and
// byte_alignment >= alignof(ElementType) is true." 2 < alignof(double).
#include <mdspan>

std::aligned_accessor<double, 2> a;
