// One thread, a chain of syncbufs: the stream buffer wrapped by the i-th syncbuf forwards what it
// receives through the (i+1)-th syncbuf, whose emit() it calls, down to a final string buffer;
// 200 links, every stream buffer distinct.
//   [syncstream.syncbuf.members]/1: emit() "Atomically transfers the associated output of *this
//     to the stream buffer *wrapped"; /5: "May call member functions of wrapped while holding a
//     lock uniquely associated with wrapped". The emits are nested, each holding the lock of its
//     own wrapped buffer, all distinct: nothing waits, and the text arrives whole at the end
//     ([syncstream.syncbuf.members]/4: emit() returns true when every character was
//     transferred).
// The chain runs in a child process with an alarm: an emit that waits forever ends it.
#include <memory>
#include <sstream>
#include <streambuf>
#include <string>
#include <syncstream>
#include <vector>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

constexpr int Links = 200;

// Forwards each write through a syncbuf wrapping `next`, emitting at once.
struct Forward : std::streambuf {
  std::streambuf* next;
  bool ok = true;
  explicit Forward(std::streambuf* n) : next(n) {}
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::syncbuf sb(next);
    const bool put = sb.sputn(s, n) == n;
    ok = sb.emit() && put && ok;
    return n;
  }
  int_type overflow(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
    const char ch = traits_type::to_char_type(c);
    return xsputn(&ch, 1) == 1 ? c : traits_type::eof();
  }
};

static void chain() {
  std::stringbuf final_buf;
  std::vector<std::unique_ptr<Forward>> links;
  std::streambuf* next = &final_buf;
  for (int i = 0; i < Links; ++i) {
    links.push_back(std::make_unique<Forward>(next));
    next = links.back().get();
  }
  std::osyncstream out(next);
  out << "through " << Links << " links";
  out.emit();
  CHECK(out.good());
  for (const auto& l : links) CHECK(l->ok);
  CHECK(final_buf.str() == "through 200 links");
}

int main() {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    alarm(20);
    chain();
    _exit(0);
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) dprintf(2, "a nested emit() waited forever\n");
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  return 0;
}
