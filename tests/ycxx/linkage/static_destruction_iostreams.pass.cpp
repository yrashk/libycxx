// Whole-program behaviour of the standard iostream objects during dynamic initialisation and
// termination, across three translation units (the other two are
// support/linkage/static_destruction_tu2.cpp, which includes <iostream>, and
// support/linkage/static_destruction_tu3.cpp, which does not). The program re-runs itself in a
// child process and checks the child's exact standard output and standard error.
//
//   [iostream.objects.overview]/3: the objects are constructed before the body of main begins;
//     "The objects are not destroyed during program execution" (footnote: "Constructors and
//     destructors for objects with static storage duration can access these objects to read
//     input from stdin or write output to stdout or stderr").
//   [iostream.objects.overview]/5: "The results of including <iostream> in a translation unit
//     shall be as if <iostream> defined an instance of ios_base::Init with static storage
//     duration." [ios.init]/3: ~Init(): "If there are no other instances of the class still in
//     existence, calls cout.flush(), cerr.flush(), clog.flush(), ...". So even with
//     sync_with_stdio(false) (mode "unsync"), output that a static object's destructor writes
//     into cout's own buffer is flushed: that object was constructed after its TU's Init, so it
//     is destroyed before that Init ([basic.start.term]/4), hence before the last Init.
//   [narrow.stream.objects]/2,5: cin.tie() == &cout, cerr.tie() == &cout, cerr unitbuf: still so
//     in the destructors (not destroyed, nothing resets them).
//   [basic.start.term]/2-6, [support.start.term]/9: exit (and returning from main, which first
//     destroys main's automatic objects: [basic.start.main]/5) destroys the thread-storage
//     objects of the main thread first, then static objects in reverse order of construction,
//     interleaved with the atexit functions: a function registered after an object's
//     construction completes is called before that object's destruction, one registered before
//     the object's construction is called after its destruction, and atexit functions run in
//     reverse order of registration. Block-scope statics are included ([stmt.dcl]/5).
//     "Objects with automatic storage duration are not destroyed as a result of calling exit()"
//     (mode "exit"). Then all C streams are flushed (/9.2), which makes the output of TU3's
//     destructor appear in the synchronized modes.
// Orders between objects of different translation units are unspecified
// ([basic.start.dynamic]/7); only orders within this TU, and relative to main, are checked for
// them.
// FILES: ../support/linkage/static_destruction_tu2.cpp ../support/linkage/static_destruction_tu3.cpp
//   (relative to the per-test temporary directory build/lit-*/linkage/<name>.XXXX: the harness
//   has no directive for additional translation units)
#include <iostream>
#include <cstdlib>
#include <format>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

int tu2_touch();
int tu3_touch();

void tu1_say(const std::string& s) { std::cout << s << '\n'; }

static bool in_main = false;

namespace {
bool ctor_may_print() { return child_mode() && (in_main || !child_mode_is("unsync")); }

struct Loud {
  std::string name;
  explicit Loud(std::string n) : name(std::move(n)) {
    if (ctor_may_print()) std::cout << "ctor " << name << '\n';
  }
  ~Loud() {
    if (!child_mode()) return;
    bool ok = std::cout.good() && std::cout.rdbuf() != nullptr && std::cin.tie() == &std::cout &&
              std::cerr.tie() == &std::cout && (std::cerr.flags() & std::ios_base::unitbuf) != 0 &&
              std::clog.rdbuf() != nullptr;
    // library facilities in a destructor that runs after main returned
    std::map<std::string, int> m{{name, 1}};
    std::ostringstream os;
    os << m.begin()->first;
    std::cout << std::format("dtor {} {}", os.str(), ok ? "ok" : "BAD") << '\n';
    std::cerr << "err " << name << '\n';
  }
};

Loud a1("a1");
Loud b1("b1");

Loud& local_static() {
  static Loud l("L");
  return l;
}
Loud& local_thread() {
  thread_local Loud t("T");
  return t;
}
void h1() { std::cout << "h1\n"; }
void h2() { std::cout << "h2\n"; }

[[noreturn]] void exit_from_nested() { std::exit(0); }

int child_main() {
  if (child_mode_is("unsync")) std::ios_base::sync_with_stdio(false);
  in_main = true;
  Loud automatic("auto");
  std::cout << "main\n";
  std::atexit(h1);
  local_static();
  local_thread();
  std::atexit(h2);
  tu2_touch();
  tu3_touch();
  std::cout << "end";  // neither newline nor flush
  if (child_mode_is("exit")) exit_from_nested();
  return 0;
}

std::vector<std::string> lines(const std::string& s) {
  std::vector<std::string> v;
  std::string cur;
  for (char c : s) {
    if (c == '\n') {
      v.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  if (!cur.empty()) v.push_back(cur);
  return v;
}

bool is_tu1(const std::string& l) {
  return l.find("c2") == std::string::npos && l.find("c3") == std::string::npos && l != "touch2";
}

std::string only_tu1(const std::string& s) {
  std::string r;
  for (auto& l : lines(s))
    if (is_tu1(l)) r += l + "\n";
  return r;
}

int count(const std::vector<std::string>& v, const std::string& x) {
  int n = 0;
  for (auto& l : v) n += (l == x);
  return n;
}
int index_of(const std::vector<std::string>& v, const std::string& x) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i] == x) return static_cast<int>(i);
  return -1;
}

void check_mode(const char* mode) {
  ChildResult r = run_self(mode);
  CHECK(r.status == 0);
  bool sync = std::string(mode) != "unsync";
  bool ret = std::string(mode) != "exit";
  // TU1, in order. "end" has no newline, so the next line continues it.
  std::string want = sync ? "ctor a1\nctor b1\n" : "";
  want += "ctor auto\nmain\nctor L\nctor T\nend";
  want += ret ? "dtor auto ok\n" : "";  // exit does not destroy the automatic object
  want += "dtor T ok\nh2\ndtor L ok\nh1\ndtor b1 ok\ndtor a1 ok\n";
  CHECK(same_text(only_tu1(r.out), want, mode));

  auto v = lines(r.out);
  // TU2: constructed once (before main, or at the latest before tu2_touch's first statement:
  // [basic.start.dynamic]/5), destroyed once after main's "end".
  int end_line = -1;
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i].starts_with("end")) end_line = static_cast<int>(i);
  CHECK(end_line >= 0);
  if (sync) {
    CHECK(count(v, "ctor c2") == 1);
    CHECK(index_of(v, "ctor c2") < index_of(v, "touch2"));
  } else {
    CHECK(count(v, "ctor c2") == 0);
  }
  CHECK(count(v, "touch2") == 1);
  CHECK(count(v, "dtor c2 ok") == 1);
  CHECK(index_of(v, "dtor c2 ok") > end_line);
  // TU3 (no <iostream>): its destructor's output, synchronized modes only.
  if (sync) {
    CHECK(count(v, "dtor c3 ok") == 1);
    CHECK(index_of(v, "dtor c3 ok") > end_line);
  }

  std::string want_err = ret ? "err auto\n" : "";
  want_err += "err T\nerr L\nerr b1\nerr a1\n";
  CHECK(same_text(only_tu1(r.err), want_err, (std::string(mode) + " stderr").c_str()));
  CHECK(count(lines(r.err), "err c2") == 1);
}
}  // namespace

int main(int argc, char**) {
  if (child_mode()) return child_main();
  CHECK(argc >= 1);
  check_mode("sync");
  check_mode("unsync");
  check_mode("exit");
  return 0;
}
