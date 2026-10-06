// [ios.init]: ios_base::Init is copy constructible and copy assignable (defaulted, synopsis);
// /2: constructing an Init constructs and initializes the eight standard objects if they have not
// already been; /3: destroying an Init flushes them only when no other Init exists.
// [iostream.objects.overview]/3, /5: the objects are not destroyed during program execution, and
// including <iostream> behaves as if it defined an Init with static storage duration, so the
// program's own Init objects, their copies and their destruction leave the objects usable.
// [ios.base.general]: ios_base itself is neither copy constructible nor copy assignable.
// Checked in a child process whose output is captured: all of it arrives, in order (narrow
// output on stdout, wide on stderr: [iostream.objects.overview]/6, a C stream has one
// orientation).
#include <iostream>
#include <string>
#include <type_traits>
#include "child_process.hpp"
#include "check.hpp"

static_assert(std::is_copy_constructible_v<std::ios_base::Init>);
static_assert(std::is_copy_assignable_v<std::ios_base::Init>);
static_assert(std::is_default_constructible_v<std::ios_base::Init>);
static_assert(!std::is_copy_constructible_v<std::ios_base>);
static_assert(!std::is_copy_assignable_v<std::ios_base>);

static int child() {
  {
    std::ios_base::Init a;
    std::ios_base::Init b(a);
    std::ios_base::Init c;
    c = b;
    std::cout << "narrow ";
    std::wcerr << L"wide ";
  } // three Init objects destroyed; the static one remains
  std::cout << "after" << std::endl;
  std::wcerr << L"end" << std::flush;
  return std::cout.good() && std::wcerr.good() ? 0 : 2;
}

int main() {
  if (child_mode())
    return child();
  ChildResult r = run_self("init");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "narrow after\n", "stdout"));
  CHECK(same_text(r.err, "wide end", "stderr"));
}
