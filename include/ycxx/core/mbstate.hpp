// libycxx core: std::mbstate_t ([cwchar.syn], freestanding) for a freestanding implementation,
// which has no C library to take it from (DECISIONS §3). Hosted, std::mbstate_t is the C
// library's ::mbstate_t (ycxx/hosted/c_wchar.hpp), whose layout this type repeats, so both
// configurations agree on the size of every object holding one (fpos, conversion states).
#pragma once

#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
// An opaque, zero-initialisable object. glibc and musl: 8 bytes, 4-byte alignment; Darwin: 128
// bytes, 8-byte alignment (cfg::mbstate_size/_align, checked against the C library's when hosted).
struct mbstate_t {
  alignas(__ycxx::__detail::__cfg::__mbstate_align) unsigned char __state[__ycxx::__detail::__cfg::__mbstate_size];
};
}} // namespace std
