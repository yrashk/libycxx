// [tuple.cnstr]/4: "If is_trivially_destructible_v<Ti> is true for all Ti, then the destructor
// of tuple is trivial." /5: "The default constructor of tuple<> is trivial." (Nothing else about
// triviality is specified: whether tuple<>'s copy and move assignments are trivial is left to
// the implementation, so it is not checked here.)
// COUNTERPART: libstdcxx:20_util/tuple/requirements/empty_trivial.cc
// COUNTERPART: libstdcxx:20_util/tuple/requirements/dr801.cc
#include <string>
#include <tuple>
#include <type_traits>

struct TrivialDtor {
  int i;
};
struct NonTrivialDtor {
  ~NonTrivialDtor() {}
};

static_assert(std::is_trivially_default_constructible_v<std::tuple<>>);
static_assert(std::is_trivially_destructible_v<std::tuple<>>);
static_assert(std::is_trivially_destructible_v<std::tuple<int>>);
static_assert(std::is_trivially_destructible_v<std::tuple<int, double, TrivialDtor, int*>>);
static_assert(std::is_trivially_destructible_v<std::tuple<int&, const long&&>>);
static_assert(std::is_trivially_destructible_v<std::tuple<std::tuple<>, std::tuple<char>>>);
static_assert(!std::is_trivially_destructible_v<std::tuple<std::string>>);
static_assert(!std::is_trivially_destructible_v<std::tuple<int, NonTrivialDtor>>);

constexpr bool use() {
  std::tuple<> t;  // trivial default construction is usable in constant evaluation
  std::tuple<> u = t;
  (void)u;
  return true;
}
static_assert(use());

int main() {}
