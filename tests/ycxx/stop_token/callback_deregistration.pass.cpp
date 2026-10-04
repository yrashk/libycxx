// [stoptoken.concepts]/3.3 (stoppable callback deregistration) and /12, for stop_callback
// ([stopcallback.general]: stop_token models stoppable_token, stop_callback is its callback
// type): (3.3.4) destroying a stop_callback from within its own callback does not block;
// (3.3.2) a callback destroyed by another callback before it ran is never invoked; (3.3.5)
// deregistration does not block on another callback's invocation, also across threads;
// (3.2.1.3.2) a stop_callback constructed during the stop request (inside a callback) runs at
// once on the constructing thread; /12 inside a callback stop_requested() is true and another
// request_stop() returns false. See support/stop_callback_scenarios.hpp.
// FLAGS: -pthread
#include <stop_token>
#include "stop_callback_scenarios.hpp"
#include "watchdog.hpp"

template<class F> using Callback = std::stop_callback<F>;

int main() {
  watchdog(20);
  scb::run_all<std::stop_source, Callback>();
  return 0;
}
