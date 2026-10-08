// Built twice, as liba.dylib and libb.dylib (-DNAME=a / b): each returns the address of the same
// inline variable of std::__y1. With default visibility (VARIANT 1, the design's shared mode) dyld
// coalesces the two weak definitions: one object per process. Hidden (VARIANT 2, static mode),
// each image keeps its own.
namespace std { inline namespace __y1 {
template <class T> struct
#if VARIANT == 1
  [[gnu::visibility("default")]]
#else
  [[gnu::visibility("hidden")]]
#endif
  counter { static inline int value = 0; };
}}
#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
extern "C" [[gnu::visibility("default")]] int* CAT(addr_, NAME)() { return &std::counter<int>::value; }
