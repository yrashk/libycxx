// [iterator.range]/1: the function templates of [iterator.range] are available when <span> is
// included (without <iterator>).
#include <span>

int a[3] = {1, 2, 3};
auto b = std::begin(a);
auto e = std::end(a);
auto cb = std::cbegin(a);
auto ce = std::cend(a);
auto rb = std::rbegin(a);
auto re = std::rend(a);
auto crb = std::crbegin(a);
auto cre = std::crend(a);
auto n = std::size(a);
auto sn = std::ssize(a);
bool em = std::empty(a);
int* d = std::data(a);
