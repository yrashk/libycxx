// libycxx core: <debugging> ([debugging]). The functions are defined out of line in the runtime
// archives (src/runtime/debugging); is_debugger_present is replaceable.
#pragma once

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
void breakpoint() noexcept;
void breakpoint_if_debugging() noexcept;
bool is_debugger_present() noexcept;
}} // namespace std
