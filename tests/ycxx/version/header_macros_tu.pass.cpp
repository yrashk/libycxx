// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion of
// any member of the set of library headers indicated in the corresponding comment in this
// synopsis." For <algorithm>, <numeric>, <format>, <text_encoding>, <inplace_vector> and
// <ranges>, each in a translation unit of its own that includes only that header
// (support/version_tu/*.cpp): every macro whose comment names the header is defined there
// exactly when <version> defines it, with the same value. (Which macros <version> defines is
// checked by version/values.compile.pass.cpp; the support files list, per header, the macros
// that name it in the synopsis.)
// FILES: ../support/version_tu/algorithm.cpp ../support/version_tu/numeric.cpp
// FILES: ../support/version_tu/format.cpp ../support/version_tu/text_encoding.cpp
// FILES: ../support/version_tu/inplace_vector.cpp ../support/version_tu/ranges.cpp
// FILES: ../support/version_tu/version.cpp
// COUNTERPART: libstdcxx:23_containers/inplace_vector/version.cc
// COUNTERPART: libstdcxx:25_algorithms/(fill_n|swap_ranges)/requirements/version.cc
// COUNTERPART: libstdcxx:26_numerics/saturation/version.cc libstdcxx:std/format/functions/format.cc
// COUNTERPART: libstdcxx:std/ranges/conv/version.cc libstdcxx:std/text_encoding/requirements.cc
#include <cstdio>
#include <cstring>
#include "check.hpp"
#include "version_tu.hpp"

extern const macro_value macros_algorithm[], macros_numeric[], macros_format[], macros_text_encoding[],
    macros_inplace_vector[], macros_ranges[], macros_version[];

long in_version(const char* name) {
  for (const macro_value* v = macros_version; v->name; ++v)
    if (std::strcmp(v->name, name) == 0) return v->value;
  CHECK(!"macro missing from the <version> table");
  return -1;
}

int check_header(const char* header, const macro_value* table) {
  int bad = 0;
  for (const macro_value* m = table; m->name; ++m) {
    const long expected = in_version(m->name);
    if (m->value != expected) {
      std::printf("<%s>: %s is %ld, <version> has %ld\n", header, m->name, m->value, expected);
      ++bad;
    }
  }
  return bad;
}

int main() {
  int bad = check_header("algorithm", macros_algorithm) + check_header("numeric", macros_numeric) +
            check_header("format", macros_format) + check_header("text_encoding", macros_text_encoding) +
            check_header("inplace_vector", macros_inplace_vector) + check_header("ranges", macros_ranges);
  CHECK(bad == 0);
}
