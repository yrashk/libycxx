// format/formatter_headers_across_tus: <vector> only, no <format>. [format.formatter.spec]/2:
// the header declares formatter, so it provides the listed specializations (and
// [vector.bool.fmt] the one for vector<bool>::reference); objects of them made here are used
// by a translation unit that includes <format>.
#include <vector>
#include "flag.hpp"
#include "facts.hpp"

Facts tu_e_facts() { return facts_here<std::vector<bool>::reference>(); }
std::formatter<int> tu_e_int_formatter() { return {}; }
std::formatter<std::vector<bool>::reference> tu_e_ref_formatter() { return {}; }
