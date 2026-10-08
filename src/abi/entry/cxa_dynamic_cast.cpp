// libycxx ABI runtime: __dynamic_cast under the Itanium C++ ABI's name (§2.9.7), forwarding to the
// runtime's own (entry.hpp, DECISIONS §20.6).
#include <cstddef>

#include "../entry.hpp"

extern "C" [[__gnu__::__visibility__("hidden")]] void* __dynamic_cast(const void* __sub, const void* __src,
                                                                      const void* __dst,
                                                                      std::ptrdiff_t __src2dst_offset) {
  return __ycxx_abi_dynamic_cast(__sub, __src, __dst, __src2dst_offset);
}
