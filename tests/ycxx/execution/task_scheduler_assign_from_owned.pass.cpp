// [exec.task.scheduler]: copy assignment is defaulted (synopsis), i.e. it assigns sch_, a
// shared_ptr ([util.smartptr.shared.assign]/1: equivalent to shared_ptr(r).swap(*this)), so it
// is correct when the source is reachable only through the target's own SCHED(x):
//   x wraps holder_sched{inner}, where inner is the only owner of a task_scheduler built from
//   inline_scheduler; after inner.reset(), ref = *inner lives only inside x's backend; x = ref
//   leaves x equal to SCHED(ref), an inline_scheduler (/6-7), and x still schedules (/13.3, /10).
// Done with a small holder (a shared_ptr, nothrow copy) and a large one (256 more bytes), and
// for plain self-assignment.
#include <execution>
#include <array>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using TS = ex::task_scheduler;

template <std::size_t Extra>
struct holder_sched {
  using scheduler_concept = ex::scheduler_tag;
  std::shared_ptr<TS> inner;
  std::array<unsigned char, Extra> state{};

  struct sender {
    using sender_concept = ex::sender_tag;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>();
    }
    struct attrs {
      std::shared_ptr<TS> inner;
      holder_sched query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {inner}; }
    };
    std::shared_ptr<TS> inner;
    attrs get_env() const noexcept { return {inner}; }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      void start() & noexcept { ex::set_value(std::move(r)); }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {std::move(r)};
    }
  };
  sender schedule() const noexcept { return {inner}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const holder_sched& o) const noexcept { return inner == o.inner; }
};

template <std::size_t Extra>
void run() {
  using H = holder_sched<Extra>;
  static_assert(ex::scheduler<H>);
  auto inner = std::make_shared<TS>(ex::inline_scheduler{});
  TS x(ex::inline_scheduler{});
  x = TS(H{inner});
  CHECK(x == H{inner} && !(x == ex::inline_scheduler{}));
  CHECK(tt::sync_wait(ex::schedule(x)).has_value());
  TS& ref = *inner;
  inner.reset();  // ref is now owned only through SCHED(x)
  x = ref;
  CHECK(x == ex::inline_scheduler{} && !(x == H{}));
  auto r = tt::sync_wait(ex::schedule(x) | ex::then([] { return 7; }));
  CHECK(r && std::get<0>(*r) == 7);

  // The same through a copy constructed from the owned source.
  auto inner2 = std::make_shared<TS>(ex::inline_scheduler{});
  TS y(H{inner2});
  TS& ref2 = *inner2;
  inner2.reset();
  TS z = ref2;
  y = z;
  CHECK(y == ex::inline_scheduler{} && z == ex::inline_scheduler{} && tt::sync_wait(ex::schedule(y)).has_value());
}

int main() {
  run<0>();
  run<256>();

  // Plain self-assignment.
  ex::run_loop loop;
  TS s(ex::inline_scheduler{});
  TS& sr = s;
  s = sr;
  CHECK(s == ex::inline_scheduler{} && tt::sync_wait(ex::schedule(s)).has_value());
  TS l(loop.get_scheduler());
  TS& lr = l;
  l = lr;
  CHECK(l == loop.get_scheduler());
  auto big = std::make_shared<TS>(ex::inline_scheduler{});
  TS b(holder_sched<256>{big});
  TS& br = b;
  b = br;
  CHECK(b == holder_sched<256>{big} && tt::sync_wait(ex::schedule(b)).has_value());
  return 0;
}
