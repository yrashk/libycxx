// libycxx hosted runtime: the default query_parallel_scheduler_backend
// ([exec.parschedrepl.query]). Replaceable ([dcl.fct.def.replace]): alone in its archive member,
// so a program's definition is linked instead of this one.
#include <execution>

namespace [[gnu::visibility("hidden")]] std { namespace execution { namespace parallel_scheduler_replacement {
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() {
  return ycxx::detail::exec::default_parallel_scheduler_backend();
}
}}} // namespace std::execution::parallel_scheduler_replacement
