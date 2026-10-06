// [exec.run.loop.ctor]/2: ~run_loop() "If the run_loop instance's count is not 0 or if its
// state is running, invokes terminate". [exec.run.loop.types]/10.3: starting an operation of a
// run_loop's schedule sender enqueues it (count 1); destroying the loop without running it
// terminates.
// EXPECT-TERMINATE: terminate handler called at the destruction
#include <exception>
#include <execution>
#include <new>
#include <optional>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  void set_stopped() && noexcept {}
};

using op_t = decltype(ex::connect(ex::schedule(std::declval<ex::run_loop&>().get_scheduler()), rcvr{}));

int main() {
  // The operation state outlives the loop, so only the loop's destructor runs here.
  alignas(op_t) static unsigned char storage[sizeof(op_t)];
  {
    std::optional<ex::run_loop> loop;
    loop.emplace();
    auto* op = ::new (storage) op_t(ex::connect(ex::schedule(loop->get_scheduler()), rcvr{}));
    ex::start(*op);
    std::set_terminate([] {
      dprintf(2, "terminate handler called at the destruction\n");
      abort();
    });
    loop.reset(); // count is 1: terminate
  }
  return 0; // not reached
}
