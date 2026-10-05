// [fs.class.directory.iterator]: iterating yields a directory_entry for each file in the
// directory, in unspecified order, never "." or ".."; a default iterator is the end iterator;
// begin(it) / end(it) make it a range; [fs.dir.itr.members] increment(ec).
// [fs.class.rec.dir.itr]: recursive_directory_iterator visits subdirectories (depth()
// reflects the nesting; disable_recursion_pending / pop). The comparisons are made order-
// independent.
// REQUIRES: exceptions
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

static_assert(std::input_iterator<fs::directory_iterator>);
static_assert(std::input_iterator<fs::recursive_directory_iterator>);
static_assert(std::ranges::input_range<fs::directory_iterator>);
static_assert(std::ranges::borrowed_range<fs::directory_iterator>);
static_assert(std::ranges::view<fs::recursive_directory_iterator>);

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  CHECK(fs::directory_iterator(base) == fs::directory_iterator());  // empty directory
  fs::create_directories(base / "sub" / "deeper");
  write_file(tmp / "f1", "1");
  write_file(tmp / "f2", "22");
  write_file(tmp / "sub/f3", "333");
  write_file(tmp / "sub/deeper/f4", "4444");

  std::set<std::string> names;
  for (const fs::directory_entry& e : fs::directory_iterator(base)) {
    CHECK(e.path().parent_path() == base);
    names.insert(e.path().filename().string());
    if (e.path().filename() == "f2") {
      CHECK(e.is_regular_file());
      CHECK(e.file_size() == 2);
    }
    if (e.path().filename() == "sub") CHECK(e.is_directory());
  }
  CHECK(names == (std::set<std::string>{"f1", "f2", "sub"}));

  // increment with error_code
  std::error_code ec;
  fs::directory_iterator it(base, ec);
  CHECK(!ec);
  int count = 0;
  for (; it != fs::directory_iterator(); it.increment(ec)) {
    CHECK(!ec);
    ++count;
  }
  CHECK(count == 3);

  // recursive
  std::set<std::string> all;
  int max_depth = -1;
  for (fs::recursive_directory_iterator r(base), end; r != end; ++r) {
    all.insert(fs::path(r->path()).lexically_relative(base).generic_string());
    max_depth = std::max(max_depth, r.depth());
  }
  CHECK(all == (std::set<std::string>{"f1", "f2", "sub", "sub/f3", "sub/deeper", "sub/deeper/f4"}));
  CHECK(max_depth == 2);

  // disable_recursion_pending: do not descend into "sub"
  std::set<std::string> top;
  for (fs::recursive_directory_iterator r(base); r != fs::recursive_directory_iterator(); ++r) {
    if (r->path().filename() == "sub") r.disable_recursion_pending();
    top.insert(r->path().filename().string());
  }
  CHECK(top == (std::set<std::string>{"f1", "f2", "sub"}));

  // a missing directory: error
  fs::directory_iterator bad(base / "missing", ec);
  CHECK(bool(ec));
  CHECK(bad == fs::directory_iterator());
  bool threw = false;
  try {
    fs::directory_iterator x(base / "missing");
  } catch (const fs::filesystem_error& e) {
    threw = e.path1() == base / "missing";
  }
  CHECK(threw);
  // range algorithms
  CHECK(std::ranges::distance(fs::directory_iterator(base)) == 3);
  return 0;
}
