// [fs.rec.dir.itr.members]/3, /15: options() is the constructor's argument, else none.
// /7-14: copies and moves (construction and assignment) keep options(), depth() and
// recursion_pending(). /17 (Note 3): the initial directory's entries are at depth 0.
// /19, /27: recursion_pending() is true after construction and after each increment, false after
// disable_recursion_pending(). /21.2 (Note 2 of /6): a symlink to a directory is entered only
// with follow_directory_symlink. /24: pop() at depth 0 is the end iterator; deeper, it leaves the
// current directory and continues in the parent. /21: increment(ec) clears ec on success.
// Entries come in an unspecified order, so the checks do not depend on it.
#include <filesystem>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;
using RDI = fs::recursive_directory_iterator;
using opts = fs::directory_options;

std::set<std::string> walk(RDI it, const fs::path& base) {
  std::set<std::string> out;
  for (; it != RDI(); ++it) {
    CHECK(it.recursion_pending());
    out.insert(it->path().lexically_relative(base).generic_string() + "@" + std::to_string(it.depth()));
  }
  return out;
}

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  fs::create_directory(base / "a");
  write_file((base / "a" / "x").string(), "1");
  write_file((base / "a" / "y").string(), "2");
  write_file((base / "f").string(), "3");
  fs::create_directory_symlink(base / "a", base / "l");

  // /3, /15
  CHECK(RDI(base).options() == opts::none);
  std::error_code ec;
  CHECK(RDI(base, ec).options() == opts::none && !ec);
  CHECK(RDI(base, opts::follow_directory_symlink).options() == opts::follow_directory_symlink);
  const opts both = opts::follow_directory_symlink | opts::skip_permission_denied;
  CHECK(RDI(base, both, ec).options() == both);

  // /21.2: without the option the link is listed but not entered.
  using S = std::set<std::string>;
  CHECK((walk(RDI(base), base) == S{"a@0", "a/x@1", "a/y@1", "f@0", "l@0"}));
  CHECK((walk(RDI(base, opts::follow_directory_symlink), base) ==
         S{"a@0", "a/x@1", "a/y@1", "f@0", "l@0", "l/x@1", "l/y@1"}));

  // /7-14: copies and moves keep the state, at depth 1 with recursion disabled.
  RDI it(base, opts::follow_directory_symlink);
  while (it.depth() == 0) ++it;
  CHECK(it.depth() == 1 && it.recursion_pending());
  it.disable_recursion_pending();
  CHECK(!it.recursion_pending());
  RDI copy(it);
  CHECK(copy.depth() == 1 && !copy.recursion_pending() && copy.options() == opts::follow_directory_symlink);
  CHECK(copy == it && copy->path() == it->path());
  RDI assigned;
  assigned = it;
  CHECK(assigned.depth() == 1 && !assigned.recursion_pending() && assigned.options() == opts::follow_directory_symlink);
  RDI moved(std::move(copy));
  CHECK(moved.depth() == 1 && !moved.recursion_pending() && moved.options() == opts::follow_directory_symlink);
  RDI move_assigned;
  move_assigned = std::move(assigned);
  CHECK(move_assigned.depth() == 1 && !move_assigned.recursion_pending());
  CHECK(move_assigned.options() == opts::follow_directory_symlink);
  it = it; // /9: self-assignment has no effect
  CHECK(it.depth() == 1 && !it.recursion_pending());
  // /19: an increment makes recursion pending again.
  it.increment(ec);
  CHECK(!ec);
  CHECK(it == RDI() || it.recursion_pending());

  // /27 with a directory at depth 0: disabling recursion skips its contents.
  S seen;
  for (RDI r(base); r != RDI(); r.increment(ec)) {
    CHECK(!ec);
    if (r->path().filename() == "a") r.disable_recursion_pending();
    seen.insert(r->path().lexically_relative(base).generic_string());
  }
  CHECK((seen == S{"a", "f", "l"}));

  // /24: pop at depth 0 ends the iteration.
  RDI p0(base);
  p0.pop();
  CHECK(p0 == RDI());
  // pop at depth 1 leaves "a" after one of its two entries and continues with the parent.
  int deep = 0, shallow = 0;
  for (RDI r(base); r != RDI();) {
    if (r.depth() == 1) {
      ++deep;
      r.pop(ec);
      CHECK(!ec);
      CHECK(r == RDI() || r.depth() == 0);
    } else {
      ++shallow;
      ++r;
    }
  }
  CHECK(deep == 1 && shallow == 3);
  return 0;
}
