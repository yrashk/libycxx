// [res.on.data.races]/2-3, /7: error_category::message ([syserr.errcat.virtuals]) of the
// generic and system categories ([syserr.errcat.objects]) may be called from several threads
// at once, also for values the C library has no message for (a C library's strerror may use a
// static buffer for those; ISO C 7.24.6.3: strerror "is not required to avoid data races");
// error_code::message, error_condition::message ([syserr.errcode.observers],
// [syserr.errcondition.observers]), system_error::what ([syserr.syserr.members]),
// default_error_condition and equivalent ([syserr.errcat.virtuals]) likewise. Each thread asks
// for messages of its own set of values, interleaved with the others, and compares them with
// the messages computed before the threads started. Meant to be run under TSan.
// FLAGS: -pthread
#include <cerrno>
#include <cstddef>
#include <latch>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
#include "check.hpp"

constexpr int K = 4;

static std::vector<int> values() {
  std::vector<int> v = {0, EPERM, ENOENT, EINTR, EIO, ENOMEM, EACCES, EEXIST, EINVAL, ERANGE, EAGAIN, ETIMEDOUT};
  for (int i = 0; i < 24; ++i) v.push_back(4000 + 37 * i);  // unknown to the C library
  v.push_back(-1);
  v.push_back(-12345);
  return v;
}

int main() {
  const std::vector<int> vals = values();
  std::vector<std::string> gen, sys, what;
  for (int v : vals) {
    gen.push_back(std::generic_category().message(v));
    sys.push_back(std::system_category().message(v));
    what.push_back(std::system_error(v, std::generic_category(), "ctx").what());
  }
  CHECK(!gen[2].empty() && what[2].find("ctx") != std::string::npos);
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0; r < 200; ++r) {
        const std::size_t i = static_cast<std::size_t>(k * 7 + r * 3) % vals.size();
        const int v = vals[i];
        CHECK(std::generic_category().message(v) == gen[i]);
        CHECK(std::system_category().message(v) == sys[i]);
        CHECK(std::error_code(v, std::generic_category()).message() == gen[i]);
        CHECK(std::error_condition(v, std::generic_category()).message() == gen[i]);
        CHECK(std::make_error_code(static_cast<std::errc>(v)).message() == gen[i]);
        CHECK(std::string(std::system_error(v, std::generic_category(), "ctx").what()) == what[i]);
        std::error_condition c = std::system_category().default_error_condition(v);
        CHECK(c.value() == v);
        CHECK(std::system_category().equivalent(v, c));
        CHECK(std::string(std::generic_category().name()) == "generic");
        CHECK(std::string(std::system_category().name()) == "system");
      }
    });
  for (auto& t : ts) t.join();
}
