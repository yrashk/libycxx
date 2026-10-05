// format/formatter_headers_across_tus: <vector> (with the program-defined formatter), <stack>,
// <queue>, and only then <format>.
#include "flag.hpp"
#include <stack>
#include <queue>
#include <format>
#include "body.hpp"
#include "facts.hpp"

std::string tu_c_text() { return format_all(); }
std::wstring tu_c_wtext() { return wformat_all(); }
Facts tu_c_facts() { return facts_here<std::vector<bool>::reference>(); }
