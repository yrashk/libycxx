// [file.native]/2: native_handle_type "is trivially copyable and models semiregular" (Note 1:
// int on POSIX systems). [filebuf.members]/11-12: native_handle() const noexcept returns the
// native handle associated with an open filebuf. [ifstream.members]/2, [ofstream.members],
// [fstream.members]: the streams' native_handle() is rdbuf()->native_handle(), and the streams
// have the same native_handle_type. [fstream.syn]: __cpp_lib_fstream_native_handle.
#include <fstream>
#include <concepts>
#include <type_traits>
#include <version>
#include <unistd.h>
#include "fs_tmpdir.hpp"
#include "check.hpp"

static_assert(__cpp_lib_fstream_native_handle >= 202306L);
using H = std::filebuf::native_handle_type;
static_assert(std::semiregular<H> && std::is_trivially_copyable_v<H>);
static_assert(std::is_same_v<std::ifstream::native_handle_type, H>);
static_assert(std::is_same_v<std::ofstream::native_handle_type, H>);
static_assert(std::is_same_v<std::fstream::native_handle_type, H>);
static_assert(std::is_same_v<std::wfilebuf::native_handle_type, std::wifstream::native_handle_type>);
static_assert(noexcept(std::declval<const std::filebuf&>().native_handle()));
static_assert(noexcept(std::declval<const std::ifstream&>().native_handle()));
static_assert(std::is_same_v<decltype(std::declval<const std::fstream&>().native_handle()), H>);

int main() {
  TmpDir dir;
  write_file(dir / "a", "abc");
  std::ifstream a(dir / "a");
  std::ofstream b(dir / "b");
  CHECK(a.is_open() && b.is_open());
  H ha = a.native_handle(), hb = b.native_handle();
  CHECK(ha == a.rdbuf()->native_handle());
  CHECK(hb == b.rdbuf()->native_handle());
  CHECK(!(ha == hb));
  if constexpr (std::is_same_v<H, int>) {
    // POSIX: a file descriptor that refers to the open file
    char c = 0;
    CHECK(::pread(ha, &c, 1, 1) == 1 && c == 'b');
  }
  return 0;
}
