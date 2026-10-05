// format/formatter_headers_across_tus: <stack> and <queue> only, no <format> and no <vector>.
// [format.formatter.spec]/2: each header that declares formatter provides the listed
// specializations.
#include <stack>
#include <queue>
#include "facts.hpp"

Facts tu_f_facts() { return facts_here<>(); }
