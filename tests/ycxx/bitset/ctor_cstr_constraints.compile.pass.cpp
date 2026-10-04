// [bitset.cons]/8: the const charT* constructor "Constraints: is_array_v<charT> is false,
// is_trivially_copyable_v<charT> is true, is_standard_layout_v<charT> is true, and
// is_trivially_default_constructible_v<charT> is true."
#include <bitset>
#include <type_traits>

struct NotTrivial {
  NotTrivial();
  friend bool operator==(NotTrivial, NotTrivial);
};
struct NotStdLayout {
  int a;
 private:
  int b;
};

static_assert(!std::is_constructible_v<std::bitset<4>, const NotTrivial*>);
static_assert(!std::is_constructible_v<std::bitset<4>, NotTrivial*>);
static_assert(!std::is_constructible_v<std::bitset<4>, const NotStdLayout*>);
static_assert(!std::is_constructible_v<std::bitset<4>, int (*)[2]>);
static_assert(std::is_constructible_v<std::bitset<4>, const char*>);
static_assert(std::is_constructible_v<std::bitset<4>, char*>);
static_assert(std::is_constructible_v<std::bitset<4>, const char (&)[3]>);
