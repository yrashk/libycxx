// [exec.par.scheduler]/13: get_parallel_scheduler() calls terminate when
// query_parallel_scheduler_backend() returns a null pointer (here, the program's replacement,
// [exec.parschedrepl.query]/3).
// EXPECT-TERMINATE
#include <execution>
#include <cstdio>
#include <memory>

namespace std::execution::parallel_scheduler_replacement {
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() { return nullptr; }
} // namespace std::execution::parallel_scheduler_replacement

int main() {
  auto sch = std::execution::get_parallel_scheduler();
  (void)sch;
  std::puts("get_parallel_scheduler returned");
  return 0;
}
