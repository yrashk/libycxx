// [stringbuf.members]/12: view() returns "sv(pbase(), high_mark - pbase())" if out is set in
// mode, "Otherwise, if ios_base::in is set in mode, then sv(eback(), egptr() - eback())",
// "(12.3) Otherwise, sv() is returned." /6: str() is basic_string(view(), get_allocator()).
// A stringbuf opened with neither in nor out therefore shows an empty sequence.
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  std::stringbuf none("abc", std::ios_base::openmode());
  CHECK(none.view().empty());
  CHECK(none.str().empty());
  std::stringbuf ate_only("abc", std::ios_base::ate);
  CHECK(ate_only.view().empty());
  return 0;
}
