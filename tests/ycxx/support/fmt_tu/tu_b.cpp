// format/formatter_headers_across_tus: <vector>, then <format>, then <queue>, then <stack>.
#include <vector>
#include <format>
#include <queue>
#include <stack>
#include "flag.hpp"
#include "body.hpp"
#include "facts.hpp"

std::string tu_b_text() { return format_all(); }
std::wstring tu_b_wtext() { return wformat_all(); }
Facts tu_b_facts() { return facts_here<std::vector<bool>::reference>(); }
