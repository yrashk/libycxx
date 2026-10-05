// [execpol.seq]/2, [execpol.par]/2, [execpol.parunseq]/2, [execpol.unseq]/2: "During the
// execution of a parallel algorithm with the execution::X policy, if the invocation of an
// element access function exits via an exception, terminate is invoked ([except.terminate])."
// [algorithms.parallel.exceptions]/2: the behavior is determined by the policy.
// [algorithms.parallel.defns]/3.3: user-provided invocable objects are element access
// functions. Checked in a child process whose terminate handler ([terminate.handler]) exits
// with a recognizable status. Control: without a policy the exception propagates.
// REQUIRES: exceptions
#include <algorithm>
#include <exception>
#include <execution>
#include <numeric>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

int data[256];

template <class F>
int status_of(F f) {
  pid_t pid = fork();
  if (pid == 0) {
    std::set_terminate([] { _exit(42); });
    try {
      f();
    } catch (...) {
      _exit(7);  // the exception escaped the algorithm
    }
    _exit(0);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

template <class P>
void check_policy(const P& pol) {
  CHECK(status_of([&] { std::for_each(pol, data, data + 256, [](int) { throw 1; }); }) == 42);
  CHECK(status_of([&] { std::sort(pol, data, data + 256, [](int, int) -> bool { throw 1; }); }) == 42);
  CHECK(status_of([&] { (void)std::count_if(pol, data, data + 256, [](int x) -> bool { if (x == 100) throw 1; return false; }); }) == 42);
  CHECK(status_of([&] { (void)std::transform_reduce(pol, data, data + 256, 0, std::plus<>(), [](int x) -> int { if (x == 200) throw 1; return x; }); }) == 42);
}

int main() {
  for (int i = 0; i < 256; ++i) data[i] = i;
  // control: no policy, the exception propagates to the caller
  CHECK(status_of([] { std::for_each(data, data + 256, [](int) { throw 1; }); }) == 7);
  check_policy(std::execution::seq);
  check_policy(std::execution::par);
  check_policy(std::execution::par_unseq);
  check_policy(std::execution::unseq);
  return 0;
}
