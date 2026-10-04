// [futures.task.members]/4: "Mandates: is_invocable_r_v<R, decay_t<F>&, ArgTypes...> is true."
#include <future>

struct NotInvocable {};
void g() {
  std::packaged_task<int(int)> t{NotInvocable{}};
}
