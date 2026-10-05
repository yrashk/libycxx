// A feature-test macro's value as seen in one translation unit (0: not defined), for
// version/header_macros_tu.pass.cpp and its translation units in support/version_tu/.
#pragma once

struct macro_value {
  const char* name;
  long value;
};
