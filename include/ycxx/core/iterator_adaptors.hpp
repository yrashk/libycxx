// libycxx core: iterator adaptors ([predef.iterators]): reverse_iterator, insert iterators,
// basic_const_iterator, move_iterator / move_sentinel, common_iterator, counted_iterator; and the
// range access CPOs that depend on them (rbegin/rend, cbegin/cend/crbegin/crend/cdata).
#pragma once

#include <ycxx/core/iterator_ops.hpp>
#include <ycxx/core/compare.hpp>
#include <initializer_list>

// =============================================================================================
// reverse_iterator
// =============================================================================================
namespace ycxx::detail {
template <class It>
consteval auto reverse_category() {
  using C = typename std::iterator_traits<It>::iterator_category;
  if constexpr (std::derived_from<C, std::random_access_iterator_tag>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else
    return std::type_identity<C>{};
}
} // namespace ycxx::detail

namespace std {

template <class Iterator>
class reverse_iterator {
public:
  using iterator_type = Iterator;
  using iterator_concept =
      conditional_t<random_access_iterator<Iterator>, random_access_iterator_tag, bidirectional_iterator_tag>;
  using iterator_category = typename decltype(ycxx::detail::reverse_category<Iterator>())::type;
  using value_type = iter_value_t<Iterator>;
  using difference_type = iter_difference_t<Iterator>;
  using pointer = typename iterator_traits<Iterator>::pointer;
  using reference = iter_reference_t<Iterator>;

  constexpr reverse_iterator() = default;
  constexpr explicit reverse_iterator(Iterator x) : current(static_cast<Iterator&&>(x)) {}
  template <class U>
    requires(!is_same_v<U, Iterator> && convertible_to<const U&, Iterator>)
  constexpr reverse_iterator(const reverse_iterator<U>& u) : current(u.base()) {}
  template <class U>
    requires(!is_same_v<U, Iterator> && convertible_to<const U&, Iterator> && assignable_from<Iterator&, const U&>)
  constexpr reverse_iterator& operator=(const reverse_iterator<U>& u) {
    current = u.base();
    return *this;
  }

  constexpr Iterator base() const { return current; }
  constexpr reference operator*() const {
    Iterator tmp = current;
    return *--tmp;
  }
  constexpr pointer operator->() const
    requires(is_pointer_v<Iterator> || requires(const Iterator i) { i.operator->(); })
  {
    Iterator tmp = current;
    --tmp;
    if constexpr (is_pointer_v<Iterator>)
      return tmp;
    else
      return tmp.operator->();
  }

  constexpr reverse_iterator& operator++() {
    --current;
    return *this;
  }
  constexpr reverse_iterator operator++(int) {
    reverse_iterator tmp = *this;
    --current;
    return tmp;
  }
  constexpr reverse_iterator& operator--() {
    ++current;
    return *this;
  }
  constexpr reverse_iterator operator--(int) {
    reverse_iterator tmp = *this;
    ++current;
    return tmp;
  }
  constexpr reverse_iterator operator+(difference_type n) const { return reverse_iterator(current - n); }
  constexpr reverse_iterator& operator+=(difference_type n) {
    current -= n;
    return *this;
  }
  constexpr reverse_iterator operator-(difference_type n) const { return reverse_iterator(current + n); }
  constexpr reverse_iterator& operator-=(difference_type n) {
    current += n;
    return *this;
  }
  constexpr reference operator[](difference_type n) const { return current[-n - 1]; }

  friend constexpr iter_rvalue_reference_t<Iterator> iter_move(const reverse_iterator& i) noexcept(
      is_nothrow_copy_constructible_v<Iterator> && noexcept(ranges::iter_move(--declval<Iterator&>()))) {
    auto tmp = i.base();
    return ranges::iter_move(--tmp);
  }
  template <indirectly_swappable<Iterator> Iterator2>
  friend constexpr void iter_swap(const reverse_iterator& x, const reverse_iterator<Iterator2>& y) noexcept(
      is_nothrow_copy_constructible_v<Iterator> && is_nothrow_copy_constructible_v<Iterator2> &&
      noexcept(ranges::iter_swap(--declval<Iterator&>(), --declval<Iterator2&>()))) {
    auto xtmp = x.base();
    auto ytmp = y.base();
    ranges::iter_swap(--xtmp, --ytmp);
  }

protected:
  Iterator current = Iterator();
};

template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x == y } -> convertible_to<bool>;
  }
constexpr bool operator==(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() == y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x != y } -> convertible_to<bool>;
  }
constexpr bool operator!=(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() != y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x > y } -> convertible_to<bool>;
  }
constexpr bool operator<(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() > y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x < y } -> convertible_to<bool>;
  }
constexpr bool operator>(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() < y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x >= y } -> convertible_to<bool>;
  }
constexpr bool operator<=(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() >= y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x <= y } -> convertible_to<bool>;
  }
constexpr bool operator>=(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y) {
  return x.base() <= y.base();
}
template <class I1, three_way_comparable_with<I1> I2>
constexpr compare_three_way_result_t<I1, I2> operator<=>(const reverse_iterator<I1>& x,
                                                         const reverse_iterator<I2>& y) {
  return y.base() <=> x.base();
}
template <class I1, class I2>
constexpr auto operator-(const reverse_iterator<I1>& x, const reverse_iterator<I2>& y)
    -> decltype(y.base() - x.base()) {
  return y.base() - x.base();
}
template <class I>
constexpr reverse_iterator<I> operator+(iter_difference_t<I> n, const reverse_iterator<I>& x) {
  return reverse_iterator<I>(x.base() - n);
}
template <class I>
constexpr reverse_iterator<I> make_reverse_iterator(I i) {
  return reverse_iterator<I>(static_cast<I&&>(i));
}
template <class I1, class I2>
  requires(!sized_sentinel_for<I1, I2>)
constexpr bool disable_sized_sentinel_for<reverse_iterator<I1>, reverse_iterator<I2>> = true;

// =============================================================================================
// insert iterators
// =============================================================================================
template <class Container>
class back_insert_iterator {
protected:
  Container* container;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr explicit back_insert_iterator(Container& x) : container(__builtin_addressof(x)) {}
  constexpr back_insert_iterator& operator=(const typename Container::value_type& v) {
    container->push_back(v);
    return *this;
  }
  constexpr back_insert_iterator& operator=(typename Container::value_type&& v) {
    container->push_back(static_cast<typename Container::value_type&&>(v));
    return *this;
  }
  constexpr back_insert_iterator& operator*() { return *this; }
  constexpr back_insert_iterator& operator++() { return *this; }
  constexpr back_insert_iterator operator++(int) { return *this; }
};
template <class Container>
constexpr back_insert_iterator<Container> back_inserter(Container& x) {
  return back_insert_iterator<Container>(x);
}

template <class Container>
class front_insert_iterator {
protected:
  Container* container;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr explicit front_insert_iterator(Container& x) : container(__builtin_addressof(x)) {}
  constexpr front_insert_iterator& operator=(const typename Container::value_type& v) {
    container->push_front(v);
    return *this;
  }
  constexpr front_insert_iterator& operator=(typename Container::value_type&& v) {
    container->push_front(static_cast<typename Container::value_type&&>(v));
    return *this;
  }
  constexpr front_insert_iterator& operator*() { return *this; }
  constexpr front_insert_iterator& operator++() { return *this; }
  constexpr front_insert_iterator operator++(int) { return *this; }
};
template <class Container>
constexpr front_insert_iterator<Container> front_inserter(Container& x) {
  return front_insert_iterator<Container>(x);
}

template <class Container>
class insert_iterator {
protected:
  Container* container;
  ranges::iterator_t<Container> iter;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr insert_iterator(Container& x, ranges::iterator_t<Container> i)
      : container(__builtin_addressof(x)), iter(static_cast<ranges::iterator_t<Container>&&>(i)) {}
  constexpr insert_iterator& operator=(const typename Container::value_type& v) {
    iter = container->insert(iter, v);
    ++iter;
    return *this;
  }
  constexpr insert_iterator& operator=(typename Container::value_type&& v) {
    iter = container->insert(iter, static_cast<typename Container::value_type&&>(v));
    ++iter;
    return *this;
  }
  constexpr insert_iterator& operator*() { return *this; }
  constexpr insert_iterator& operator++() { return *this; }
  constexpr insert_iterator& operator++(int) { return *this; }
};
template <class Container>
constexpr insert_iterator<Container> inserter(Container& x, ranges::iterator_t<Container> i) {
  return insert_iterator<Container>(x, i);
}

// =============================================================================================
// [const.iterators]
// =============================================================================================
template <indirectly_readable It>
using iter_const_reference_t = common_reference_t<const iter_value_t<It>&&, iter_reference_t<It>>;

} // namespace std

namespace ycxx::detail {
template <class It>
concept constant_iterator = std::input_iterator<It> && std::same_as<std::iter_const_reference_t<It>, std::iter_reference_t<It>>;
template <std::indirectly_readable It>
using iter_const_rvalue_reference_t = std::common_reference_t<const std::iter_value_t<It>&&, std::iter_rvalue_reference_t<It>>;

template <class I>
consteval auto const_iter_concept() {
  if constexpr (std::contiguous_iterator<I>)
    return std::type_identity<std::contiguous_iterator_tag>{};
  else if constexpr (std::random_access_iterator<I>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (std::bidirectional_iterator<I>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (std::forward_iterator<I>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}

} // namespace ycxx::detail

// Base classes of std types live in ycxx::adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// ycxx::detail base would expose every internal function to lookup on the std type.
namespace ycxx::adl_free {
template <class I>
struct const_iter_category {};
template <std::forward_iterator I>
struct const_iter_category<I> {
  using iterator_category = typename std::iterator_traits<I>::iterator_category;
};
} // namespace ycxx::adl_free

namespace std {

template <input_iterator Iter>
class basic_const_iterator;

} // namespace std

namespace ycxx::detail {
template <class T>
inline constexpr bool is_basic_const_iterator = false;
template <class I>
inline constexpr bool is_basic_const_iterator<std::basic_const_iterator<I>> = true;
template <class T>
concept not_a_const_iterator = !is_basic_const_iterator<T>;
template <class T, class U>
concept different_from = !std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;
} // namespace ycxx::detail

namespace std {

template <input_iterator Iter>
class basic_const_iterator : public ycxx::adl_free::const_iter_category<Iter> {
  template <input_iterator>
  friend class basic_const_iterator;
  Iter current_ = Iter();
  using reference = iter_const_reference_t<Iter>;
  using rvalue_reference = ycxx::detail::iter_const_rvalue_reference_t<Iter>;

public:
  using iterator_concept = typename decltype(ycxx::detail::const_iter_concept<Iter>())::type;
  using value_type = iter_value_t<Iter>;
  using difference_type = iter_difference_t<Iter>;
  using iterator_type = Iter;

  basic_const_iterator()
    requires default_initializable<Iter>
  = default;
  constexpr basic_const_iterator(Iter x) : current_(static_cast<Iter&&>(x)) {}
  template <convertible_to<Iter> U>
  constexpr basic_const_iterator(basic_const_iterator<U> other) : current_(static_cast<U&&>(other.current_)) {}
  template <ycxx::detail::different_from<basic_const_iterator> T>
    requires convertible_to<T, Iter>
  constexpr basic_const_iterator(T&& x) : current_(static_cast<T&&>(x)) {}

  constexpr const Iter& base() const& noexcept { return current_; }
  constexpr Iter base() && { return static_cast<Iter&&>(current_); }

  constexpr reference operator*() const { return static_cast<reference>(*current_); }
  constexpr const auto* operator->() const
    requires is_lvalue_reference_v<iter_reference_t<Iter>> &&
             same_as<remove_cvref_t<iter_reference_t<Iter>>, value_type>
  {
    if constexpr (contiguous_iterator<Iter>)
      return std::to_address(current_);
    else
      return __builtin_addressof(*current_);
  }

  constexpr basic_const_iterator& operator++() {
    ++current_;
    return *this;
  }
  constexpr void operator++(int) { ++current_; }
  constexpr basic_const_iterator operator++(int)
    requires forward_iterator<Iter>
  {
    auto tmp = *this;
    ++*this;
    return tmp;
  }
  constexpr basic_const_iterator& operator--()
    requires bidirectional_iterator<Iter>
  {
    --current_;
    return *this;
  }
  constexpr basic_const_iterator operator--(int)
    requires bidirectional_iterator<Iter>
  {
    auto tmp = *this;
    --*this;
    return tmp;
  }
  constexpr basic_const_iterator& operator+=(difference_type n)
    requires random_access_iterator<Iter>
  {
    current_ += n;
    return *this;
  }
  constexpr basic_const_iterator& operator-=(difference_type n)
    requires random_access_iterator<Iter>
  {
    current_ -= n;
    return *this;
  }
  constexpr reference operator[](difference_type n) const
    requires random_access_iterator<Iter>
  {
    return static_cast<reference>(current_[n]);
  }

  template <sentinel_for<Iter> S>
  constexpr bool operator==(const S& s) const {
    return current_ == s;
  }

  template <ycxx::detail::not_a_const_iterator CI>
    requires ycxx::detail::constant_iterator<CI> && convertible_to<const Iter&, CI>
  constexpr operator CI() const& {
    return current_;
  }
  template <ycxx::detail::not_a_const_iterator CI>
    requires ycxx::detail::constant_iterator<CI> && convertible_to<Iter, CI>
  constexpr operator CI() && {
    return static_cast<Iter&&>(current_);
  }

  constexpr bool operator<(const basic_const_iterator& y) const
    requires random_access_iterator<Iter>
  {
    return current_ < y.current_;
  }
  constexpr bool operator>(const basic_const_iterator& y) const
    requires random_access_iterator<Iter>
  {
    return current_ > y.current_;
  }
  constexpr bool operator<=(const basic_const_iterator& y) const
    requires random_access_iterator<Iter>
  {
    return current_ <= y.current_;
  }
  constexpr bool operator>=(const basic_const_iterator& y) const
    requires random_access_iterator<Iter>
  {
    return current_ >= y.current_;
  }
  constexpr auto operator<=>(const basic_const_iterator& y) const
    requires random_access_iterator<Iter> && three_way_comparable<Iter>
  {
    return current_ <=> y.current_;
  }

  template <ycxx::detail::different_from<basic_const_iterator> I>
  constexpr bool operator<(const I& y) const
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return current_ < y;
  }
  template <ycxx::detail::different_from<basic_const_iterator> I>
  constexpr bool operator>(const I& y) const
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return current_ > y;
  }
  template <ycxx::detail::different_from<basic_const_iterator> I>
  constexpr bool operator<=(const I& y) const
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return current_ <= y;
  }
  template <ycxx::detail::different_from<basic_const_iterator> I>
  constexpr bool operator>=(const I& y) const
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return current_ >= y;
  }
  template <ycxx::detail::different_from<basic_const_iterator> I>
  constexpr auto operator<=>(const I& y) const
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I> && three_way_comparable_with<Iter, I>
  {
    return current_ <=> y;
  }
  template <ycxx::detail::not_a_const_iterator I>
  friend constexpr bool operator<(const I& x, const basic_const_iterator& y)
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return x < y.current_;
  }
  template <ycxx::detail::not_a_const_iterator I>
  friend constexpr bool operator>(const I& x, const basic_const_iterator& y)
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return x > y.current_;
  }
  template <ycxx::detail::not_a_const_iterator I>
  friend constexpr bool operator<=(const I& x, const basic_const_iterator& y)
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return x <= y.current_;
  }
  template <ycxx::detail::not_a_const_iterator I>
  friend constexpr bool operator>=(const I& x, const basic_const_iterator& y)
    requires random_access_iterator<Iter> && totally_ordered_with<Iter, I>
  {
    return x >= y.current_;
  }

  friend constexpr basic_const_iterator operator+(const basic_const_iterator& i, difference_type n)
    requires random_access_iterator<Iter>
  {
    return basic_const_iterator(i.current_ + n);
  }
  friend constexpr basic_const_iterator operator+(difference_type n, const basic_const_iterator& i)
    requires random_access_iterator<Iter>
  {
    return basic_const_iterator(i.current_ + n);
  }
  friend constexpr basic_const_iterator operator-(const basic_const_iterator& i, difference_type n)
    requires random_access_iterator<Iter>
  {
    return basic_const_iterator(i.current_ - n);
  }
  template <sized_sentinel_for<Iter> S>
  constexpr difference_type operator-(const S& y) const {
    return current_ - y;
  }
  template <ycxx::detail::not_a_const_iterator S>
    requires sized_sentinel_for<S, Iter>
  friend constexpr difference_type operator-(const S& x, const basic_const_iterator& y) {
    return x - y.current_;
  }

  friend constexpr rvalue_reference iter_move(const basic_const_iterator& i) noexcept(
      noexcept(static_cast<rvalue_reference>(ranges::iter_move(i.current_)))) {
    return static_cast<rvalue_reference>(ranges::iter_move(i.current_));
  }
};

template <class T, common_with<T> U>
  requires input_iterator<common_type_t<T, U>>
struct common_type<basic_const_iterator<T>, U> {
  using type = basic_const_iterator<common_type_t<T, U>>;
};
template <class T, common_with<T> U>
  requires input_iterator<common_type_t<T, U>>
struct common_type<U, basic_const_iterator<T>> {
  using type = basic_const_iterator<common_type_t<T, U>>;
};
template <class T, common_with<T> U>
  requires input_iterator<common_type_t<T, U>>
struct common_type<basic_const_iterator<T>, basic_const_iterator<U>> {
  using type = basic_const_iterator<common_type_t<T, U>>;
};

template <input_iterator I>
using const_iterator = conditional_t<ycxx::detail::constant_iterator<I>, I, basic_const_iterator<I>>;

} // namespace std

namespace ycxx::detail {
template <class S>
struct const_sentinel_impl {
  using type = S;
};
template <std::input_iterator S>
struct const_sentinel_impl<S> {
  using type = std::const_iterator<S>;
};
} // namespace ycxx::detail

namespace std {

template <semiregular S>
using const_sentinel = typename ycxx::detail::const_sentinel_impl<S>::type;

template <input_iterator I>
constexpr const_iterator<I> make_const_iterator(I it) {
  return it;
}
template <semiregular S>
constexpr const_sentinel<S> make_const_sentinel(S s) {
  return s;
}

// =============================================================================================
// move_iterator / move_sentinel
// =============================================================================================
template <semiregular S>
class move_sentinel {
  S last_ = S();

public:
  constexpr move_sentinel() = default;
  constexpr explicit move_sentinel(S s) : last_(static_cast<S&&>(s)) {}
  template <class S2>
    requires convertible_to<const S2&, S>
  constexpr move_sentinel(const move_sentinel<S2>& s) : last_(s.base()) {}
  template <class S2>
    requires assignable_from<S&, const S2&>
  constexpr move_sentinel& operator=(const move_sentinel<S2>& s) {
    last_ = s.base();
    return *this;
  }
  constexpr S base() const { return last_; }
};

} // namespace std

namespace ycxx::detail {
template <class I>
consteval auto move_iter_concept() {
  if constexpr (std::random_access_iterator<I>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (std::bidirectional_iterator<I>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (std::forward_iterator<I>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}
} // namespace ycxx::detail

// Base classes of std types live in ycxx::adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// ycxx::detail base would expose every internal function to lookup on the std type.
namespace ycxx::adl_free {
template <class I>
struct move_iter_category {};
template <class I>
  requires requires { typename std::iterator_traits<I>::iterator_category; }
struct move_iter_category<I> {
  using iterator_category =
      std::conditional_t<std::derived_from<typename std::iterator_traits<I>::iterator_category,
                                           std::random_access_iterator_tag>,
                         std::random_access_iterator_tag, typename std::iterator_traits<I>::iterator_category>;
};
} // namespace ycxx::adl_free

namespace std {

template <class Iterator>
class move_iterator : public ycxx::adl_free::move_iter_category<Iterator> {
  Iterator current_ = Iterator();

public:
  using iterator_type = Iterator;
  using iterator_concept = typename decltype(ycxx::detail::move_iter_concept<Iterator>())::type;
  using value_type = iter_value_t<Iterator>;
  using difference_type = iter_difference_t<Iterator>;
  using pointer = Iterator;
  using reference = iter_rvalue_reference_t<Iterator>;

  constexpr move_iterator()
    requires default_initializable<Iterator>
  = default;
  constexpr explicit move_iterator(Iterator i) : current_(static_cast<Iterator&&>(i)) {}
  template <class U>
    requires(!is_same_v<U, Iterator> && convertible_to<const U&, Iterator>)
  constexpr move_iterator(const move_iterator<U>& u) : current_(u.base()) {}
  template <class U>
    requires(!is_same_v<U, Iterator> && convertible_to<const U&, Iterator> && assignable_from<Iterator&, const U&>)
  constexpr move_iterator& operator=(const move_iterator<U>& u) {
    current_ = u.base();
    return *this;
  }

  constexpr const Iterator& base() const& noexcept { return current_; }
  constexpr Iterator base() && { return static_cast<Iterator&&>(current_); }

  constexpr reference operator*() const { return ranges::iter_move(current_); }

  constexpr move_iterator& operator++() {
    ++current_;
    return *this;
  }
  constexpr auto operator++(int) {
    if constexpr (forward_iterator<Iterator>) {
      move_iterator tmp = *this;
      ++current_;
      return tmp;
    } else {
      ++current_;
    }
  }
  constexpr move_iterator& operator--() {
    --current_;
    return *this;
  }
  constexpr move_iterator operator--(int) {
    move_iterator tmp = *this;
    --current_;
    return tmp;
  }
  constexpr move_iterator operator+(difference_type n) const { return move_iterator(current_ + n); }
  constexpr move_iterator& operator+=(difference_type n) {
    current_ += n;
    return *this;
  }
  constexpr move_iterator operator-(difference_type n) const { return move_iterator(current_ - n); }
  constexpr move_iterator& operator-=(difference_type n) {
    current_ -= n;
    return *this;
  }
  constexpr reference operator[](difference_type n) const { return ranges::iter_move(current_ + n); }

  template <sentinel_for<Iterator> S>
  friend constexpr bool operator==(const move_iterator& x, const move_sentinel<S>& y) {
    return x.base() == y.base();
  }
  template <sized_sentinel_for<Iterator> S>
  friend constexpr iter_difference_t<Iterator> operator-(const move_sentinel<S>& x, const move_iterator& y) {
    return x.base() - y.base();
  }
  template <sized_sentinel_for<Iterator> S>
  friend constexpr iter_difference_t<Iterator> operator-(const move_iterator& x, const move_sentinel<S>& y) {
    return x.base() - y.base();
  }
  friend constexpr iter_rvalue_reference_t<Iterator> iter_move(const move_iterator& i) noexcept(
      noexcept(ranges::iter_move(i.current_))) {
    return ranges::iter_move(i.current_);
  }
  template <indirectly_swappable<Iterator> Iterator2>
  friend constexpr void iter_swap(const move_iterator& x, const move_iterator<Iterator2>& y) noexcept(
      noexcept(ranges::iter_swap(x.current_, y.base()))) {
    ranges::iter_swap(x.current_, y.base());
  }
};

template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x == y } -> convertible_to<bool>;
  }
constexpr bool operator==(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return x.base() == y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x < y } -> convertible_to<bool>;
  }
constexpr bool operator<(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return x.base() < y.base();
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { y < x } -> convertible_to<bool>;
  }
constexpr bool operator>(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return y < x;
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { y < x } -> convertible_to<bool>;
  }
constexpr bool operator<=(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return !(y < x);
}
template <class I1, class I2>
  requires requires(const I1& x, const I2& y) {
    { x < y } -> convertible_to<bool>;
  }
constexpr bool operator>=(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return !(x < y);
}
template <class I1, three_way_comparable_with<I1> I2>
constexpr compare_three_way_result_t<I1, I2> operator<=>(const move_iterator<I1>& x, const move_iterator<I2>& y) {
  return x.base() <=> y.base();
}
template <class I1, class I2>
constexpr auto operator-(const move_iterator<I1>& x, const move_iterator<I2>& y) -> decltype(x.base() - y.base()) {
  return x.base() - y.base();
}
template <class I>
  requires requires(const I& i, iter_difference_t<I> n) {
    { i + n } -> same_as<I>;
  }
constexpr move_iterator<I> operator+(iter_difference_t<I> n, const move_iterator<I>& x) {
  return x + n;
}
template <class I>
constexpr move_iterator<I> make_move_iterator(I i) {
  return move_iterator<I>(static_cast<I&&>(i));
}
template <class I1, class I2>
  requires(!sized_sentinel_for<I1, I2>)
constexpr bool disable_sized_sentinel_for<move_iterator<I1>, move_iterator<I2>> = true;

// =============================================================================================
// counted_iterator
// =============================================================================================
} // namespace std

// Base classes of std types live in ycxx::adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// ycxx::detail base would expose every internal function to lookup on the std type.
namespace ycxx::adl_free {
template <class I>
struct counted_value_type {};
template <std::indirectly_readable I>
struct counted_value_type<I> {
  using value_type = std::iter_value_t<I>;
};
template <class I>
struct counted_concept {};
template <class I>
  requires requires { typename I::iterator_concept; }
struct counted_concept<I> {
  using iterator_concept = typename I::iterator_concept;
};
template <class I>
struct counted_category {};
template <class I>
  requires requires { typename I::iterator_category; }
struct counted_category<I> {
  using iterator_category = typename I::iterator_category;
};
} // namespace ycxx::adl_free

namespace std {

template <input_or_output_iterator I>
class counted_iterator : public ycxx::adl_free::counted_value_type<I>,
                         public ycxx::adl_free::counted_concept<I>,
                         public ycxx::adl_free::counted_category<I> {
  template <input_or_output_iterator I2>
  friend class counted_iterator;

  I current_ = I();
  iter_difference_t<I> length_ = 0;

public:
  using iterator_type = I;
  using difference_type = iter_difference_t<I>;

  constexpr counted_iterator()
    requires default_initializable<I>
  = default;
  constexpr counted_iterator(I x, iter_difference_t<I> n) : current_(static_cast<I&&>(x)), length_(n) {
    ycxx::detail::precondition(n >= 0, "counted_iterator: negative count");
  }
  template <class I2>
    requires convertible_to<const I2&, I>
  constexpr counted_iterator(const counted_iterator<I2>& x) : current_(x.current_), length_(x.length_) {}
  template <class I2>
    requires assignable_from<I&, const I2&>
  constexpr counted_iterator& operator=(const counted_iterator<I2>& x) {
    current_ = x.current_;
    length_ = x.length_;
    return *this;
  }

  constexpr const I& base() const& noexcept { return current_; }
  constexpr I base() && { return static_cast<I&&>(current_); }
  constexpr iter_difference_t<I> count() const noexcept { return length_; }

  constexpr decltype(auto) operator*() { return *current_; }
  constexpr decltype(auto) operator*() const
    requires ycxx::detail::dereferenceable<const I>
  {
    return *current_;
  }
  constexpr auto operator->() const noexcept
    requires contiguous_iterator<I>
  {
    return std::to_address(current_);
  }

  constexpr counted_iterator& operator++() {
    ++current_;
    --length_;
    return *this;
  }
  constexpr decltype(auto) operator++(int) {
    if constexpr (forward_iterator<I>) {
      counted_iterator tmp = *this;
      ++*this;
      return tmp;
    } else {
      --length_;
      if constexpr (ycxx::detail::cfg::exceptions) {
        try {
          return current_++;
        } catch (...) {
          ++length_;
          throw;
        }
      } else {
        return current_++;
      }
    }
  }
  constexpr counted_iterator& operator--()
    requires bidirectional_iterator<I>
  {
    --current_;
    ++length_;
    return *this;
  }
  constexpr counted_iterator operator--(int)
    requires bidirectional_iterator<I>
  {
    counted_iterator tmp = *this;
    --*this;
    return tmp;
  }
  constexpr counted_iterator operator+(iter_difference_t<I> n) const
    requires random_access_iterator<I>
  {
    return counted_iterator(current_ + n, length_ - n);
  }
  friend constexpr counted_iterator operator+(iter_difference_t<I> n, const counted_iterator& x)
    requires random_access_iterator<I>
  {
    return x + n;
  }
  constexpr counted_iterator& operator+=(iter_difference_t<I> n)
    requires random_access_iterator<I>
  {
    current_ += n;
    length_ -= n;
    return *this;
  }
  constexpr counted_iterator operator-(iter_difference_t<I> n) const
    requires random_access_iterator<I>
  {
    return counted_iterator(current_ - n, length_ + n);
  }
  template <common_with<I> I2>
  friend constexpr iter_difference_t<I2> operator-(const counted_iterator& x, const counted_iterator<I2>& y) {
    return y.length_ - x.length_;
  }
  friend constexpr iter_difference_t<I> operator-(const counted_iterator& x, default_sentinel_t) noexcept {
    return -x.length_;
  }
  friend constexpr iter_difference_t<I> operator-(default_sentinel_t, const counted_iterator& y) noexcept {
    return y.length_;
  }
  constexpr counted_iterator& operator-=(iter_difference_t<I> n)
    requires random_access_iterator<I>
  {
    current_ -= n;
    length_ += n;
    return *this;
  }
  constexpr decltype(auto) operator[](iter_difference_t<I> n) const
    requires random_access_iterator<I>
  {
    return current_[n];
  }

  template <common_with<I> I2>
  friend constexpr bool operator==(const counted_iterator& x, const counted_iterator<I2>& y) {
    return x.length_ == y.length_;
  }
  friend constexpr bool operator==(const counted_iterator& x, default_sentinel_t) noexcept { return x.length_ == 0; }
  template <common_with<I> I2>
  friend constexpr strong_ordering operator<=>(const counted_iterator& x, const counted_iterator<I2>& y) {
    return y.length_ <=> x.length_;
  }

  friend constexpr iter_rvalue_reference_t<I> iter_move(const counted_iterator& i) noexcept(
      noexcept(ranges::iter_move(i.current_)))
    requires input_iterator<I>
  {
    return ranges::iter_move(i.current_);
  }
  template <indirectly_swappable<I> I2>
  friend constexpr void iter_swap(const counted_iterator& x, const counted_iterator<I2>& y) noexcept(
      noexcept(ranges::iter_swap(x.current_, y.current_))) {
    ranges::iter_swap(x.current_, y.current_);
  }
};

template <input_iterator I>
  requires same_as<ycxx::detail::iter_traits<I>, iterator_traits<I>>
struct iterator_traits<counted_iterator<I>> : iterator_traits<I> {
  using pointer = conditional_t<contiguous_iterator<I>, add_pointer_t<iter_reference_t<I>>, void>;
};

// =============================================================================================
// common_iterator
// =============================================================================================
template <input_or_output_iterator I, sentinel_for<I> S>
  requires(!same_as<I, S> && copyable<I>)
class common_iterator {
  template <input_or_output_iterator I2, sentinel_for<I2> S2>
    requires(!same_as<I2, S2> && copyable<I2>)
  friend class common_iterator;

  // A two-alternative variant: index 0 = iterator, 1 = sentinel.
  union {
    I it_;
    S sent_;
  };
  unsigned char index_;

  class proxy {
    iter_value_t<I> keep_;

  public:
    constexpr proxy(iter_reference_t<I>&& x) : keep_(static_cast<iter_reference_t<I>&&>(x)) {}
    constexpr const iter_value_t<I>* operator->() const noexcept { return __builtin_addressof(keep_); }
  };
  class postfix_proxy {
    iter_value_t<I> keep_;

  public:
    constexpr postfix_proxy(iter_reference_t<I>&& x) : keep_(static_cast<iter_reference_t<I>&&>(x)) {}
    constexpr const iter_value_t<I>& operator*() const noexcept { return keep_; }
  };

  constexpr void destroy() noexcept {
    if (index_ == 0)
      std::destroy_at(__builtin_addressof(it_));
    else if (index_ == 1)
      std::destroy_at(__builtin_addressof(sent_));
  }
  template <class Other>
  constexpr void copy_from(Other&& o) {
    if (o.index_ == 0)
      std::construct_at(__builtin_addressof(it_), static_cast<Other&&>(o).it_);
    else if (o.index_ == 1)
      std::construct_at(__builtin_addressof(sent_), static_cast<Other&&>(o).sent_);
    index_ = o.index_; // 2 (valueless) copies as valueless
  }

public:
  constexpr common_iterator()
    requires default_initializable<I>
      : it_(), index_(0) {}
  constexpr common_iterator(I i) : it_(static_cast<I&&>(i)), index_(0) {}
  constexpr common_iterator(S s) : sent_(static_cast<S&&>(s)), index_(1) {}
  template <class I2, class S2>
    requires convertible_to<const I2&, I> && convertible_to<const S2&, S>
  constexpr common_iterator(const common_iterator<I2, S2>& x) : index_(x.index_) {
    if (x.index_ == 0)
      std::construct_at(__builtin_addressof(it_), x.it_);
    else
      std::construct_at(__builtin_addressof(sent_), x.sent_);
  }
  constexpr common_iterator(const common_iterator& x)
    requires(is_trivially_copy_constructible_v<I> && is_trivially_copy_constructible_v<S>)
  = default;
  constexpr common_iterator(const common_iterator& x) : index_(2) { copy_from(x); }
  constexpr common_iterator(common_iterator&& x)
    requires(is_trivially_move_constructible_v<I> && is_trivially_move_constructible_v<S>)
  = default;
  constexpr common_iterator(common_iterator&& x) : index_(2) { copy_from(static_cast<common_iterator&&>(x)); }
  constexpr ~common_iterator()
    requires(is_trivially_destructible_v<I> && is_trivially_destructible_v<S>)
  = default;
  constexpr ~common_iterator() { destroy(); }

  template <class I2, class S2>
    requires convertible_to<const I2&, I> && convertible_to<const S2&, S> && assignable_from<I&, const I2&> &&
             assignable_from<S&, const S2&>
  constexpr common_iterator& operator=(const common_iterator<I2, S2>& x) {
    if (index_ == x.index_) {
      if (index_ == 0)
        it_ = x.it_;
      else
        sent_ = x.sent_;
    } else {
      destroy();
      index_ = 2;
      if (x.index_ == 0)
        std::construct_at(__builtin_addressof(it_), x.it_);
      else
        std::construct_at(__builtin_addressof(sent_), x.sent_);
      index_ = x.index_;
    }
    return *this;
  }
  constexpr common_iterator& operator=(const common_iterator& x)
    requires(is_trivially_copy_assignable_v<I> && is_trivially_copy_assignable_v<S> &&
             is_trivially_copy_constructible_v<I> && is_trivially_copy_constructible_v<S> &&
             is_trivially_destructible_v<I> && is_trivially_destructible_v<S>)
  = default;
  constexpr common_iterator& operator=(const common_iterator& x) {
    if (this != &x) {
      if (index_ == x.index_ && index_ == 0)
        it_ = x.it_;
      else if (index_ == x.index_ && index_ == 1)
        sent_ = x.sent_;
      else {
        destroy();
        index_ = 2;
        copy_from(x);
      }
    }
    return *this;
  }
  constexpr common_iterator& operator=(common_iterator&& x)
    requires(is_trivially_move_assignable_v<I> && is_trivially_move_assignable_v<S> &&
             is_trivially_move_constructible_v<I> && is_trivially_move_constructible_v<S> &&
             is_trivially_destructible_v<I> && is_trivially_destructible_v<S>)
  = default;
  constexpr common_iterator& operator=(common_iterator&& x) {
    if (index_ == x.index_ && index_ == 0)
      it_ = static_cast<I&&>(x.it_);
    else if (index_ == x.index_ && index_ == 1)
      sent_ = static_cast<S&&>(x.sent_);
    else {
      destroy();
      index_ = 2;
      copy_from(static_cast<common_iterator&&>(x));
    }
    return *this;
  }

  constexpr decltype(auto) operator*() {
    ycxx::detail::precondition(index_ == 0, "common_iterator: dereferencing a sentinel");
    return *it_;
  }
  constexpr decltype(auto) operator*() const
    requires ycxx::detail::dereferenceable<const I>
  {
    ycxx::detail::precondition(index_ == 0, "common_iterator: dereferencing a sentinel");
    return *it_;
  }
  constexpr auto operator->() const
    requires indirectly_readable<const I> &&
             (requires(const I& i) { i.operator->(); } || is_reference_v<iter_reference_t<I>> ||
              constructible_from<iter_value_t<I>, iter_reference_t<I>>)
  {
    ycxx::detail::precondition(index_ == 0, "common_iterator: operator-> on a sentinel");
    if constexpr (is_pointer_v<I> || requires(const I& i) { i.operator->(); }) {
      return it_;
    } else if constexpr (is_reference_v<iter_reference_t<I>>) {
      auto&& tmp = *it_;
      return __builtin_addressof(tmp);
    } else {
      return proxy(*it_);
    }
  }

  constexpr common_iterator& operator++() {
    ycxx::detail::precondition(index_ == 0, "common_iterator: incrementing a sentinel");
    ++it_;
    return *this;
  }
  constexpr decltype(auto) operator++(int) {
    ycxx::detail::precondition(index_ == 0, "common_iterator: incrementing a sentinel");
    if constexpr (forward_iterator<I>) {
      common_iterator tmp = *this;
      ++*this;
      return tmp;
    } else if constexpr (requires(I& i) {
                           { *i++ } -> ycxx::detail::can_reference;
                         } || !(indirectly_readable<I> && constructible_from<iter_value_t<I>, iter_reference_t<I>> &&
                                move_constructible<iter_value_t<I>>)) {
      return it_++;
    } else {
      postfix_proxy p(*it_);
      ++*this;
      return p;
    }
  }

  template <class I2, sentinel_for<I> S2>
    requires sentinel_for<S, I2>
  friend constexpr bool operator==(const common_iterator& x, const common_iterator<I2, S2>& y) {
    if (x.index_ == y.index_) {
      if constexpr (equality_comparable_with<I, I2>) {
        if (x.index_ == 0)
          return x.it_ == y.it_;
      }
      return true;
    }
    return x.index_ == 0 ? x.it_ == y.sent_ : x.sent_ == y.it_;
  }

  template <sized_sentinel_for<I> I2, sized_sentinel_for<I> S2>
    requires sized_sentinel_for<S, I2>
  friend constexpr iter_difference_t<I2> operator-(const common_iterator& x, const common_iterator<I2, S2>& y) {
    if (x.index_ == 1 && y.index_ == 1)
      return 0;
    if (x.index_ == 0 && y.index_ == 0)
      return x.it_ - y.it_;
    return x.index_ == 0 ? x.it_ - y.sent_ : x.sent_ - y.it_;
  }

  friend constexpr decltype(auto) iter_move(const common_iterator& i) noexcept(noexcept(ranges::iter_move(declval<const I&>())))
    requires input_iterator<I>
  {
    return ranges::iter_move(i.it_);
  }
  template <indirectly_swappable<I> I2, class S2>
  friend constexpr void iter_swap(const common_iterator& x, const common_iterator<I2, S2>& y) noexcept(
      noexcept(ranges::iter_swap(declval<const I&>(), declval<const I2&>()))) {
    ranges::iter_swap(x.it_, y.it_);
  }
};

template <class I, class S>
struct incrementable_traits<common_iterator<I, S>> {
  using difference_type = iter_difference_t<I>;
};

} // namespace std

namespace ycxx::detail {
template <class I, class S>
consteval auto common_iter_pointer() {
  if constexpr (requires(const std::common_iterator<I, S>& a) { a.operator->(); })
    return std::type_identity<decltype(std::declval<const std::common_iterator<I, S>&>().operator->())>{};
  else
    return std::type_identity<void>{};
}
} // namespace ycxx::detail

namespace std {
template <input_iterator I, class S>
struct iterator_traits<common_iterator<I, S>> {
  using iterator_concept = conditional_t<forward_iterator<I>, forward_iterator_tag, input_iterator_tag>;
  using iterator_category = decltype([] {
    if constexpr (requires { typename iterator_traits<I>::iterator_category; }) {
      if constexpr (derived_from<typename iterator_traits<I>::iterator_category, forward_iterator_tag>)
        return forward_iterator_tag{};
      else
        return input_iterator_tag{};
    } else {
      return input_iterator_tag{};
    }
  }());
  using value_type = iter_value_t<I>;
  using difference_type = iter_difference_t<I>;
  using pointer = typename decltype(ycxx::detail::common_iter_pointer<I, S>())::type;
  using reference = iter_reference_t<I>;
};

} // namespace std

// =============================================================================================
// range access CPOs that need adaptors
// =============================================================================================
namespace ycxx::detail::range_access {

namespace rbegin_ns {
void rbegin() = delete; // hides outer declarations: the call below uses argument-dependent lookup only
template <class T>
concept member = requires(T& t) {
  { ::ycxx::detail::decay_copy(t.rbegin()) } -> std::input_or_output_iterator;
};
template <class T>
concept adl = class_or_enum<T> && requires(T& t) {
  { ::ycxx::detail::decay_copy(rbegin(t)) } -> std::input_or_output_iterator;
};
template <class T>
concept reversible = requires(T& t) {
  { std::ranges::begin(t) } -> std::bidirectional_iterator;
  { std::ranges::end(t) } -> std::same_as<decltype(std::ranges::begin(t))>;
};
struct fn {
  template <class T>
  static consteval bool nothrow() {
    if constexpr (member<T>)
      return noexcept(::ycxx::detail::decay_copy(std::declval<T&>().rbegin()));
    else if constexpr (adl<T>)
      return noexcept(::ycxx::detail::decay_copy(rbegin(std::declval<T&>())));
    else
      return noexcept(std::make_reverse_iterator(std::ranges::end(std::declval<T&>())));
  }
  template <class T>
    requires maybe_borrowed<T> && (member<T> || adl<T> || reversible<T>)
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
    if constexpr (member<T>)
      return ::ycxx::detail::decay_copy(t.rbegin());
    else if constexpr (adl<T>)
      return ::ycxx::detail::decay_copy(rbegin(t));
    else
      return std::make_reverse_iterator(std::ranges::end(t));
  }
};
} // namespace rbegin_ns
} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::rbegin_ns::fn rbegin{};
}
} // namespace std::ranges

namespace ycxx::detail::range_access {
namespace rend_ns {
void rend() = delete; // hides outer declarations: the call below uses argument-dependent lookup only
template <class T>
concept member = requires(T& t) {
  { ::ycxx::detail::decay_copy(t.rend()) } -> std::sentinel_for<decltype(std::ranges::rbegin(t))>;
};
template <class T>
concept adl = class_or_enum<T> && requires(T& t) {
  { ::ycxx::detail::decay_copy(rend(t)) } -> std::sentinel_for<decltype(std::ranges::rbegin(t))>;
};
struct fn {
  template <class T>
  static consteval bool nothrow() {
    if constexpr (member<T>)
      return noexcept(::ycxx::detail::decay_copy(std::declval<T&>().rend()));
    else if constexpr (adl<T>)
      return noexcept(::ycxx::detail::decay_copy(rend(std::declval<T&>())));
    else
      return noexcept(std::make_reverse_iterator(std::ranges::begin(std::declval<T&>())));
  }
  template <class T>
    requires maybe_borrowed<T> && (member<T> || adl<T> || rbegin_ns::reversible<T>)
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
    if constexpr (member<T>)
      return ::ycxx::detail::decay_copy(t.rend());
    else if constexpr (adl<T>)
      return ::ycxx::detail::decay_copy(rend(t));
    else
      return std::make_reverse_iterator(std::ranges::begin(t));
  }
};
} // namespace rend_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::rend_ns::fn rend{};
} // namespace cpo

template <class T>
concept constant_range = input_range<T> && ycxx::detail::constant_iterator<iterator_t<T>>;
} // namespace std::ranges

namespace ycxx::detail::range_access {

template <std::ranges::input_range R>
constexpr auto& possibly_const_range(R& r) noexcept {
  if constexpr (std::ranges::input_range<const R>)
    return const_cast<const R&>(r);
  else
    return r;
}

template <class T>
constexpr auto as_const_pointer(const T* p) noexcept {
  return p;
}

namespace cbegin_ns {
struct fn {
  template <class T>
    requires maybe_borrowed<T> && requires(T& t) { std::ranges::begin(::ycxx::detail::range_access::possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::const_iterator<decltype(std::ranges::begin(::ycxx::detail::range_access::possibly_const_range(t)))>(
          std::ranges::begin(::ycxx::detail::range_access::possibly_const_range(t))))) {
    return std::const_iterator<decltype(std::ranges::begin(::ycxx::detail::range_access::possibly_const_range(t)))>(
        std::ranges::begin(::ycxx::detail::range_access::possibly_const_range(t)));
  }
};
} // namespace cbegin_ns
namespace cend_ns {
struct fn {
  template <class T>
    requires maybe_borrowed<T> && requires(T& t) { std::ranges::end(::ycxx::detail::range_access::possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::const_sentinel<decltype(std::ranges::end(::ycxx::detail::range_access::possibly_const_range(t)))>(
          std::ranges::end(::ycxx::detail::range_access::possibly_const_range(t))))) {
    return std::const_sentinel<decltype(std::ranges::end(::ycxx::detail::range_access::possibly_const_range(t)))>(
        std::ranges::end(::ycxx::detail::range_access::possibly_const_range(t)));
  }
};
} // namespace cend_ns
namespace crbegin_ns {
struct fn {
  template <class T>
    requires maybe_borrowed<T> && requires(T& t) { std::ranges::rbegin(::ycxx::detail::range_access::possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::const_iterator<decltype(std::ranges::rbegin(::ycxx::detail::range_access::possibly_const_range(t)))>(
          std::ranges::rbegin(::ycxx::detail::range_access::possibly_const_range(t))))) {
    return std::const_iterator<decltype(std::ranges::rbegin(::ycxx::detail::range_access::possibly_const_range(t)))>(
        std::ranges::rbegin(::ycxx::detail::range_access::possibly_const_range(t)));
  }
};
} // namespace crbegin_ns
namespace crend_ns {
struct fn {
  template <class T>
    requires maybe_borrowed<T> && requires(T& t) { std::ranges::rend(::ycxx::detail::range_access::possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::const_sentinel<decltype(std::ranges::rend(::ycxx::detail::range_access::possibly_const_range(t)))>(
          std::ranges::rend(::ycxx::detail::range_access::possibly_const_range(t))))) {
    return std::const_sentinel<decltype(std::ranges::rend(::ycxx::detail::range_access::possibly_const_range(t)))>(
        std::ranges::rend(::ycxx::detail::range_access::possibly_const_range(t)));
  }
};
} // namespace crend_ns
namespace cdata_ns {
struct fn {
  template <class T>
    requires maybe_borrowed<T> && requires(T& t) { std::ranges::data(::ycxx::detail::range_access::possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::ranges::data(::ycxx::detail::range_access::possibly_const_range(t)))) {
    return ::ycxx::detail::range_access::as_const_pointer(std::ranges::data(::ycxx::detail::range_access::possibly_const_range(t)));
  }
};
} // namespace cdata_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::cbegin_ns::fn cbegin{};
inline constexpr ycxx::detail::range_access::cend_ns::fn cend{};
inline constexpr ycxx::detail::range_access::crbegin_ns::fn crbegin{};
inline constexpr ycxx::detail::range_access::crend_ns::fn crend{};
inline constexpr ycxx::detail::range_access::cdata_ns::fn cdata{};
} // namespace cpo

template <range R>
using const_iterator_t = decltype(ranges::cbegin(declval<R&>()));
template <range R>
using const_sentinel_t = decltype(ranges::cend(declval<R&>()));
template <range R>
using range_const_reference_t = iter_const_reference_t<iterator_t<R>>;
} // namespace std::ranges

namespace std {
template <class C>
constexpr auto rbegin(C& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
  return c.rbegin();
}
template <class C>
constexpr auto rbegin(const C& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
  return c.rbegin();
}
template <class C>
constexpr auto rend(C& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
  return c.rend();
}
template <class C>
constexpr auto rend(const C& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
  return c.rend();
}
template <class T, size_t N>
constexpr reverse_iterator<T*> rbegin(T (&a)[N]) noexcept {
  return reverse_iterator<T*>(a + N);
}
template <class T, size_t N>
constexpr reverse_iterator<T*> rend(T (&a)[N]) noexcept {
  return reverse_iterator<T*>(a);
}
template <class E>
constexpr reverse_iterator<const E*> rbegin(initializer_list<E> il) noexcept {
  return reverse_iterator<const E*>(il.end());
}
template <class E>
constexpr reverse_iterator<const E*> rend(initializer_list<E> il) noexcept {
  return reverse_iterator<const E*>(il.begin());
}
template <class C>
constexpr auto crbegin(const C& c) noexcept(noexcept(std::rbegin(c))) -> decltype(std::rbegin(c)) {
  return std::rbegin(c);
}
template <class C>
constexpr auto crend(const C& c) noexcept(noexcept(std::rend(c))) -> decltype(std::rend(c)) {
  return std::rend(c);
}
} // namespace std
