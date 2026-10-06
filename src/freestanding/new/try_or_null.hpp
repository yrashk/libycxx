// The nothrow allocation forms ([new.delete.single]/7, [new.delete.array]): "calls the plain
// form; a null pointer if that fails".
//
// Without a heap, the default plain forms fail (bad_alloc, or the error handler without
// exceptions), so calling them from a nothrow form would never return null without exceptions.
// The default base forms therefore define a marker symbol, which the nothrow forms reference
// weakly: a weak reference does not pull an archive member, so a marker exists only if the
// heap-less default is the function actually linked. Then the nothrow form returns null at once;
// otherwise the program replaced the plain form, and the nothrow form forwards to it as
// specified (a replacement returns or does not return; it does not need the null path).
#pragma once
#include <ycxx/config.hpp>

extern "C" [[__gnu__::__weak__]] const char __ycxx_fs_default_new;
extern "C" [[__gnu__::__weak__]] const char __ycxx_fs_default_new_align;
extern "C" [[__gnu__::__weak__]] const char __ycxx_default_new_array;
extern "C" [[__gnu__::__weak__]] const char __ycxx_default_new_array_align;

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// A template, so that under -fno-exceptions the discarded try/catch is never instantiated.
template <class _Fp>
void* __try_or_null(bool __heapless_default, _Fp __f) noexcept {
  if (__heapless_default)
    return nullptr;
  if constexpr (__cfg::exceptions) {
    try {
      return __f();
    } catch (...) {
      return nullptr;
    }
  } else {
    return __f();
  }
}
}} // namespace __ycxx::__detail
