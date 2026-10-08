// libycxx ABI runtime: the personality routine under the Itanium C++ ABI's name ([ABI-EH] 2.5.2),
// forwarding to the runtime's own (entry.hpp, DECISIONS §20.6). Every frame with a C++ handler or
// cleanup names it in its unwind information; in shared mode each image has this forwarder,
// hidden.
#include <cstdint>

#include "../entry.hpp"

extern "C" [[__gnu__::__visibility__("hidden")]] _Unwind_Reason_Code
__gxx_personality_v0(int __version, _Unwind_Action __actions, std::uint64_t __cls, _Unwind_Exception* __ue,
                     _Unwind_Context* __ctx) {
  return __ycxx_abi_personality(__version, __actions, __cls, __ue, __ctx);
}
