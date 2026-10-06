// [exec.sync.wait]/5.1: sync_wait(sndr) mandates sender_in<Sndr, sync-wait-env>. sync-wait-env
// answers only get_scheduler, get_start_scheduler and get_delegation_scheduler (/2), so
// read_env(get_allocator) has no completion signatures there ([exec.read.env]/5): ill-formed.
// EXPECT-ERROR: static assert.*sync_wait.*no completion signatures
#include <execution>
#include <memory>

namespace ex = std::execution;

void f() { (void)std::this_thread::sync_wait(ex::read_env(std::get_allocator)); }
