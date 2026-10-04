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

extern "C" [[gnu::weak]] const char ycxx_fs_default_new;
extern "C" [[gnu::weak]] const char ycxx_fs_default_new_align;
extern "C" [[gnu::weak]] const char ycxx_default_new_array;
extern "C" [[gnu::weak]] const char ycxx_default_new_array_align;

namespace ycxx::detail {
// A template, so that under -fno-exceptions the discarded try/catch is never instantiated.
template <class F>
void* try_or_null(bool heapless_default, F f) noexcept {
  if (heapless_default)
    return nullptr;
  if constexpr (cfg::exceptions) {
    try {
      return f();
    } catch (...) {
      return nullptr;
    }
  } else {
    return f();
  }
}
} // namespace ycxx::detail
