// The nothrow forms' "call, or null on failure", for builds with or without exceptions. A
// template, so that under -fno-exceptions the discarded try/catch is never instantiated (a try
// in a discarded branch of a non-template function is still rejected).
#pragma once
#include <ycxx/config.hpp>

namespace ycxx::detail {
template <class F>
void* try_or_null(F f) noexcept {
  if constexpr (cfg::exceptions) {
    try {
      return f();
    } catch (...) {
      return nullptr;
    }
  } else {
    return nullptr; // a failure cannot be detected without exceptions; see the file comments
  }
}
} // namespace ycxx::detail
