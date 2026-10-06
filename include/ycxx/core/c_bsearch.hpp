// libycxx core: bsearch ([alg.c.library]), declared by <cstdlib> (hosted and freestanding) and,
// through it, by <stdlib.h>.
//
// The draft gives it a const-correct pair: a `void*` base returns `void*`, a `const void*` base
// `const void*`. The C library's single `void* bsearch(const void*, const void* base, ...)` cannot
// be one of them, so the pair is libycxx's own and the hosted <cstdlib> hides the C library's
// declaration (__ycxx_c_bsearch; see there). Templates, as the rest of <cstdlib>'s own functions,
// so that where the C library's declaration is visible after all (its header read before
// <cstdlib>), it wins ties instead of conflicting. The c-compare-pred and compare-pred overloads
// of the draft are one function each: neither compiler makes language linkage part of a
// function type.
#pragma once

#include <ycxx/core/cstddef.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The C semantics; compar's exceptions propagate ([alg.c.library]/4).
template <class = void>
const void* __c_bsearch(const void* key, const void* base, std::size_t __nmemb, std::size_t size,
                      int (*__compar)(const void*, const void*)) {
  const auto* b = static_cast<const unsigned char*>(base);
  while (__nmemb != 0) {
    const std::size_t __mid = __nmemb / 2;
    const unsigned char* p = b + __mid * size;
    const int r = __compar(key, p);
    if (r == 0)
      return p;
    if (r > 0) {
      b = p + size;
      __nmemb -= __mid + 1;
    } else {
      __nmemb = __mid;
    }
  }
  return nullptr;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class = void>
void* bsearch(const void* key, void* base, size_t __nmemb, size_t size, int (*__compar)(const void*, const void*)) {
  return const_cast<void*>(::__ycxx::__detail::__c_bsearch(key, base, __nmemb, size, __compar));
}
template <class = void>
const void* bsearch(const void* key, const void* base, size_t __nmemb, size_t size,
                    int (*__compar)(const void*, const void*)) {
  return ::__ycxx::__detail::__c_bsearch(key, base, __nmemb, size, __compar);
}
} // namespace std
