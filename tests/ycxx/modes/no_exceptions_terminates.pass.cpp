// Built with -fno-exceptions. Where the draft says an operation throws, a program without
// exceptions cannot observe that exception; what happens instead is implementation-defined (for
// libycxx: DECISIONS §4, ycxx_error_handler, by default an abort). What every implementation
// must not do is carry on as if the operation had succeeded: the postconditions the draft gives
// for a normal return cannot hold. Each operation below runs in a child process, which reports a
// normal return with exit status 0; the parent requires any other outcome.
//   [sequence.reqmts]/121, [array.overview], [string.access]/4, [map.access]/9 (at: out_of_range)
//   [optional.observe]/18 (value: bad_optional_access), [expected.object.obs]/15
//   [variant.get]/8 (get: bad_variant_access), [any.nonmembers]/6 (any_cast: bad_any_cast)
//   [func.wrap.func.inv]/2 (empty function: bad_function_call)
//   [bitset.members]/50 (test: out_of_range), [string.conversions]/2 (stoi: invalid_argument)
//   [string.substr]/2 (substr: out_of_range), [format.functions]/10 (vformat: format_error)
//   [util.smartptr.shared.const]/32 (shared_ptr(weak_ptr) of an expired pointer: bad_weak_ptr)
//   [vector.capacity]/6 (reserve(n > max_size()): length_error)
//   [new.delete.single]/3 (operator new: bad_alloc; a normal return yields storage)
#include <any>
#include <array>
#include <bitset>
#include <deque>
#include <expected>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

// FLAGS: -fno-exceptions

template <class F>
bool returns_normally(F f) {
  pid_t pid = fork();
  if (pid == 0) {
    f();
    _exit(0);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}

volatile std::size_t huge = std::size_t(-1) / 2;
int sink;

int main() {
  // control: the harness sees a normal return
  CHECK(returns_normally([] { sink = std::vector<int>{1}.at(0); }));

  CHECK(!returns_normally([] { std::vector<int> v{1, 2}; sink = v.at(2); }));
  CHECK(!returns_normally([] { std::deque<int> d; sink = d.at(0); }));
  CHECK(!returns_normally([] { std::array<int, 3> a{}; sink = a.at(3); }));
  CHECK(!returns_normally([] { std::string s = "ab"; sink = s.at(5); }));
  CHECK(!returns_normally([] { std::map<int, int> m; sink = m.at(1); }));
  CHECK(!returns_normally([] { std::optional<int> o; sink = o.value(); }));
  CHECK(!returns_normally([] { std::expected<int, int> e = std::unexpected(1); sink = e.value(); }));
  CHECK(!returns_normally([] { std::variant<int, long> v = 1L; sink = std::get<int>(v); }));
  CHECK(!returns_normally([] { std::any a = 1L; sink = std::any_cast<int>(a); }));
  CHECK(!returns_normally([] { std::function<int()> f; sink = f(); }));
  CHECK(!returns_normally([] { std::bitset<8> b; sink = b.test(8); }));
  CHECK(!returns_normally([] { sink = std::stoi("x"); }));
  CHECK(!returns_normally([] { std::string s = "ab"; sink = int(s.substr(3).size()); }));
  CHECK(!returns_normally([] { sink = int(std::vformat("{", std::make_format_args(sink)).size()); }));
  CHECK(!returns_normally([] {
    std::weak_ptr<int> w;
    std::shared_ptr<int> p(w);
    sink = p ? 1 : 0;
  }));
  if (std::vector<int>().max_size() < std::size_t(-1))
    CHECK(!returns_normally([] { std::vector<int> v; v.reserve(v.max_size() + 1); }));
  CHECK(!returns_normally([] {
    void* p = ::operator new(huge * 2);
    static_cast<char*>(p)[0] = 1;
  }));
  return 0;
}
