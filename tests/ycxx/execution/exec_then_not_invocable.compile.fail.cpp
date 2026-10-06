// [exec.snd.expos]/24.4: make-sender mandates dependent_sender<Sndr> || sender_in<Sndr>; then's
// check-types ([exec.then]/5) rejects an f that is not invocable with the child's values, so
// then(just(std::string()), f) with f taking an int is ill-formed.
// EXPECT-ERROR: static assertion failed.*the sender cannot complete in any environment
#include <execution>
#include <string>

void f() { (void)std::execution::then(std::execution::just(std::string()), [](int) { return 0; }); }
