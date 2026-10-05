// format/formatter_headers_across_tus: <queue>, then <format>, then <stack>, then <vector>.
#include <queue>
#include <format>
#include <stack>
#include <vector>
#include "flag.hpp"
#include "body.hpp"
#include "facts.hpp"

std::string tu_d_text() { return format_all(); }
std::wstring tu_d_wtext() { return wformat_all(); }
Facts tu_d_facts() { return facts_here<std::vector<bool>::reference>(); }
