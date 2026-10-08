// `auto x = {1, 2}` and range-for over a braced list deduce std::initializer_list<E>; GCC finds the
// template only in plain std.
#ifdef PLAIN
namespace std {
#else
namespace std { inline namespace __y1 {
#endif
template <class E> class initializer_list {
  const E* b_; decltype(sizeof 0) n_;
  constexpr initializer_list(const E* b, decltype(sizeof 0) n) : b_(b), n_(n) {}
public:
  constexpr initializer_list() : b_(nullptr), n_(0) {}
  constexpr const E* begin() const { return b_; }
  constexpr const E* end() const { return b_ + n_; }
  constexpr decltype(sizeof 0) size() const { return n_; }
};
#ifdef PLAIN
}
#else
}}
#endif
int main() {
  int s = 0;
  for (int i : {1, 2, 3}) s += i;
  auto l = {4, 5};
  return s == 6 && l.size() == 2 ? 0 : 1;
}
