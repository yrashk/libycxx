// [format.range.formatter] Table 115: the m range-type requires T to be "either a
// specialization of pair or a specialization of tuple such that tuple_size_v<T> is 2".
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:m}", std::vector<int>{1, 2}); }
