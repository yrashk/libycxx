// Which of the macros that name <text_encoding> in [version.syn] are defined after including only
// <text_encoding> (for version/header_macros_tu.pass.cpp).
#include <text_encoding>
#include "../version_tu.hpp"

extern const macro_value macros_text_encoding[] = {
#ifdef __cpp_lib_text_encoding
    {"__cpp_lib_text_encoding", __cpp_lib_text_encoding},
#else
    {"__cpp_lib_text_encoding", 0},
#endif
    {nullptr, 0}};
