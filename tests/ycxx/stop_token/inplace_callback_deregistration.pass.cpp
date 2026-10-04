// [stoptoken.concepts]/3.3 and /12 for inplace_stop_callback ([stopcallback.inplace.general]:
// "inplace_stop_callback ... models stoppable-callback-for<CallbackFn, inplace_stop_token,
// Initializer>"; [stoptoken.inplace.general]: inplace_stop_token models stoppable_token):
// the same deregistration scenarios as stop_token/callback_deregistration (self-destruction
// inside the callback, destroying another callback, no blocking on another callback across
// threads, registration and request_stop from inside a callback). See
// support/stop_callback_scenarios.hpp.
// FLAGS: -pthread
#include <stop_token>
#include "stop_callback_scenarios.hpp"
#include "watchdog.hpp"

template<class F> using Callback = std::inplace_stop_callback<F>;

int main() {
  watchdog(20);
  scb::run_all<std::inplace_stop_source, Callback>();
  return 0;
}
