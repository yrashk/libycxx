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
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _It>
consteval auto __reverse_category() {
  using _Cp = typename std::iterator_traits<_It>::iterator_category;
  if constexpr (std::derived_from<_Cp, std::random_access_iterator_tag>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else
    return std::type_identity<_Cp>{};
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Iterator>
class reverse_iterator {
public:
  using iterator_type = _Iterator;
  using iterator_concept =
      conditional_t<random_access_iterator<_Iterator>, random_access_iterator_tag, bidirectional_iterator_tag>;
  using iterator_category = typename decltype(__ycxx::__detail::__reverse_category<_Iterator>())::type;
  using value_type = iter_value_t<_Iterator>;
  using difference_type = iter_difference_t<_Iterator>;
  using pointer = typename iterator_traits<_Iterator>::pointer;
  using reference = iter_reference_t<_Iterator>;

  constexpr reverse_iterator() = default;
  constexpr explicit reverse_iterator(_Iterator __x) : current(static_cast<_Iterator&&>(__x)) {}
  template <class _Up>
    requires(!is_same_v<_Up, _Iterator> && convertible_to<const _Up&, _Iterator>)
  constexpr reverse_iterator(const reverse_iterator<_Up>& __u) : current(__u.base()) {}
  template <class _Up>
    requires(!is_same_v<_Up, _Iterator> && convertible_to<const _Up&, _Iterator> && assignable_from<_Iterator&, const _Up&>)
  constexpr reverse_iterator& operator=(const reverse_iterator<_Up>& __u) {
    current = __u.base();
    return *this;
  }

  constexpr _Iterator base() const { return current; }
  constexpr reference operator*() const {
    _Iterator __tmp = current;
    return *--__tmp;
  }
  constexpr pointer operator->() const
    requires(is_pointer_v<_Iterator> || requires(const _Iterator i) { i.operator->(); })
  {
    _Iterator __tmp = current;
    --__tmp;
    if constexpr (is_pointer_v<_Iterator>)
      return __tmp;
    else
      return __tmp.operator->();
  }

  constexpr reverse_iterator& operator++() {
    --current;
    return *this;
  }
  constexpr reverse_iterator operator++(int) {
    reverse_iterator __tmp = *this;
    --current;
    return __tmp;
  }
  constexpr reverse_iterator& operator--() {
    ++current;
    return *this;
  }
  constexpr reverse_iterator operator--(int) {
    reverse_iterator __tmp = *this;
    ++current;
    return __tmp;
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

  friend constexpr iter_rvalue_reference_t<_Iterator> iter_move(const reverse_iterator& i) noexcept(
      is_nothrow_copy_constructible_v<_Iterator> && noexcept(ranges::iter_move(--declval<_Iterator&>()))) {
    auto __tmp = i.base();
    return ranges::iter_move(--__tmp);
  }
  template <indirectly_swappable<_Iterator> _Iterator2>
  friend constexpr void iter_swap(const reverse_iterator& __x, const reverse_iterator<_Iterator2>& y) noexcept(
      is_nothrow_copy_constructible_v<_Iterator> && is_nothrow_copy_constructible_v<_Iterator2> &&
      noexcept(ranges::iter_swap(--declval<_Iterator&>(), --declval<_Iterator2&>()))) {
    auto __xtmp = __x.base();
    auto __ytmp = y.base();
    ranges::iter_swap(--__xtmp, --__ytmp);
  }

protected:
  _Iterator current = _Iterator();
};

template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x == y } -> convertible_to<bool>;
  }
constexpr bool operator==(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() == y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x != y } -> convertible_to<bool>;
  }
constexpr bool operator!=(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() != y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x > y } -> convertible_to<bool>;
  }
constexpr bool operator<(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() > y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x < y } -> convertible_to<bool>;
  }
constexpr bool operator>(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() < y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x >= y } -> convertible_to<bool>;
  }
constexpr bool operator<=(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() >= y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x <= y } -> convertible_to<bool>;
  }
constexpr bool operator>=(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y) {
  return __x.base() <= y.base();
}
template <class _I1, three_way_comparable_with<_I1> _I2>
constexpr compare_three_way_result_t<_I1, _I2> operator<=>(const reverse_iterator<_I1>& __x,
                                                         const reverse_iterator<_I2>& y) {
  return y.base() <=> __x.base();
}
template <class _I1, class _I2>
constexpr auto operator-(const reverse_iterator<_I1>& __x, const reverse_iterator<_I2>& y)
    -> decltype(y.base() - __x.base()) {
  return y.base() - __x.base();
}
template <class _Ip>
constexpr reverse_iterator<_Ip> operator+(iter_difference_t<_Ip> n, const reverse_iterator<_Ip>& __x) {
  return reverse_iterator<_Ip>(__x.base() - n);
}
template <class _Ip>
constexpr reverse_iterator<_Ip> make_reverse_iterator(_Ip i) {
  return reverse_iterator<_Ip>(static_cast<_Ip&&>(i));
}
template <class _I1, class _I2>
  requires(!sized_sentinel_for<_I1, _I2>)
constexpr bool disable_sized_sentinel_for<reverse_iterator<_I1>, reverse_iterator<_I2>> = true;

// =============================================================================================
// insert iterators
// =============================================================================================
template <class _Container>
class back_insert_iterator {
protected:
  _Container* container;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = _Container;

  constexpr explicit back_insert_iterator(_Container& __x) : container(__builtin_addressof(__x)) {}
  constexpr back_insert_iterator& operator=(const typename _Container::value_type& __v) {
    container->push_back(__v);
    return *this;
  }
  constexpr back_insert_iterator& operator=(typename _Container::value_type&& __v) {
    container->push_back(static_cast<typename _Container::value_type&&>(__v));
    return *this;
  }
  constexpr back_insert_iterator& operator*() { return *this; }
  constexpr back_insert_iterator& operator++() { return *this; }
  constexpr back_insert_iterator operator++(int) { return *this; }
};
template <class _Container>
constexpr back_insert_iterator<_Container> back_inserter(_Container& __x) {
  return back_insert_iterator<_Container>(__x);
}

template <class _Container>
class front_insert_iterator {
protected:
  _Container* container;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = _Container;

  constexpr explicit front_insert_iterator(_Container& __x) : container(__builtin_addressof(__x)) {}
  constexpr front_insert_iterator& operator=(const typename _Container::value_type& __v) {
    container->push_front(__v);
    return *this;
  }
  constexpr front_insert_iterator& operator=(typename _Container::value_type&& __v) {
    container->push_front(static_cast<typename _Container::value_type&&>(__v));
    return *this;
  }
  constexpr front_insert_iterator& operator*() { return *this; }
  constexpr front_insert_iterator& operator++() { return *this; }
  constexpr front_insert_iterator operator++(int) { return *this; }
};
template <class _Container>
constexpr front_insert_iterator<_Container> front_inserter(_Container& __x) {
  return front_insert_iterator<_Container>(__x);
}

template <class _Container>
class insert_iterator {
protected:
  _Container* container;
  ranges::iterator_t<_Container> iter;

public:
  using iterator_category = output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = _Container;

  constexpr insert_iterator(_Container& __x, ranges::iterator_t<_Container> i)
      : container(__builtin_addressof(__x)), iter(static_cast<ranges::iterator_t<_Container>&&>(i)) {}
  constexpr insert_iterator& operator=(const typename _Container::value_type& __v) {
    iter = container->insert(iter, __v);
    ++iter;
    return *this;
  }
  constexpr insert_iterator& operator=(typename _Container::value_type&& __v) {
    iter = container->insert(iter, static_cast<typename _Container::value_type&&>(__v));
    ++iter;
    return *this;
  }
  constexpr insert_iterator& operator*() { return *this; }
  constexpr insert_iterator& operator++() { return *this; }
  constexpr insert_iterator& operator++(int) { return *this; }
};
template <class _Container>
constexpr insert_iterator<_Container> inserter(_Container& __x, ranges::iterator_t<_Container> i) {
  return insert_iterator<_Container>(__x, i);
}

// =============================================================================================
// [const.iterators]
// =============================================================================================
template <indirectly_readable _It>
using iter_const_reference_t = common_reference_t<const iter_value_t<_It>&&, iter_reference_t<_It>>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _It>
concept __constant_iterator = std::input_iterator<_It> && std::same_as<std::iter_const_reference_t<_It>, std::iter_reference_t<_It>>;
template <std::indirectly_readable _It>
using __iter_const_rvalue_reference_t = std::common_reference_t<const std::iter_value_t<_It>&&, std::iter_rvalue_reference_t<_It>>;

template <class _Ip>
consteval auto __const_iter_concept() {
  if constexpr (std::contiguous_iterator<_Ip>)
    return std::type_identity<std::contiguous_iterator_tag>{};
  else if constexpr (std::random_access_iterator<_Ip>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (std::bidirectional_iterator<_Ip>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (std::forward_iterator<_Ip>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}

}} // namespace __ycxx::__detail

// Base classes of std types live in __ycxx::__adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// __ycxx::__detail base would expose every internal function to lookup on the std type.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Ip>
struct __const_iter_category {};
template <std::forward_iterator _Ip>
struct __const_iter_category<_Ip> {
  using iterator_category = typename std::iterator_traits<_Ip>::iterator_category;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <input_iterator _Iter>
class basic_const_iterator;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_basic_const_iterator = false;
template <class _Ip>
inline constexpr bool __is_basic_const_iterator<std::basic_const_iterator<_Ip>> = true;
template <class _Tp>
concept __not_a_const_iterator = !__is_basic_const_iterator<_Tp>;
template <class _Tp, class _Up>
concept __different_from = !std::same_as<std::remove_cvref_t<_Tp>, std::remove_cvref_t<_Up>>;
// `__it < i` alone, a part of totally_ordered_with<Iter, I> tested first by basic_const_iterator's
// comparisons with another type I. Found by argument-dependent lookup for an adaptor over a
// basic_const_iterator (reverse_iterator<basic_const_iterator<It>>), those operators would
// otherwise ask whether I is totally ordered while I's own comparison is being resolved
// (libstdc++ PR 112490); this test fails first, without asking about I < I.
template <class _Iter, class _Ip>
concept __const_iter_less_with = requires(const _Iter& __it, const _Ip& i) { __it < i; };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <input_iterator _Iter>
class basic_const_iterator : public __ycxx::__adl_free::__const_iter_category<_Iter> {
  template <input_iterator>
  friend class basic_const_iterator;
  _Iter __current_ = _Iter();
  using reference = iter_const_reference_t<_Iter>;
  using __rvalue_reference = __ycxx::__detail::__iter_const_rvalue_reference_t<_Iter>;

public:
  using iterator_concept = typename decltype(__ycxx::__detail::__const_iter_concept<_Iter>())::type;
  using value_type = iter_value_t<_Iter>;
  using difference_type = iter_difference_t<_Iter>;
  using iterator_type = _Iter;

  basic_const_iterator()
    requires default_initializable<_Iter>
  = default;
  constexpr basic_const_iterator(_Iter __x) : __current_(static_cast<_Iter&&>(__x)) {}
  template <convertible_to<_Iter> _Up>
  constexpr basic_const_iterator(basic_const_iterator<_Up> other) : __current_(static_cast<_Up&&>(other.__current_)) {}
  template <__ycxx::__detail::__different_from<basic_const_iterator> _Tp>
    requires convertible_to<_Tp, _Iter>
  constexpr basic_const_iterator(_Tp&& __x) : __current_(static_cast<_Tp&&>(__x)) {}

  constexpr const _Iter& base() const& noexcept { return __current_; }
  constexpr _Iter base() && { return static_cast<_Iter&&>(__current_); }

  constexpr reference operator*() const { return static_cast<reference>(*__current_); }
  constexpr const auto* operator->() const
    requires is_lvalue_reference_v<iter_reference_t<_Iter>> &&
             same_as<remove_cvref_t<iter_reference_t<_Iter>>, value_type>
  {
    if constexpr (contiguous_iterator<_Iter>)
      return std::to_address(__current_);
    else
      return __builtin_addressof(*__current_);
  }

  constexpr basic_const_iterator& operator++() {
    ++__current_;
    return *this;
  }
  constexpr void operator++(int) { ++__current_; }
  constexpr basic_const_iterator operator++(int)
    requires forward_iterator<_Iter>
  {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }
  constexpr basic_const_iterator& operator--()
    requires bidirectional_iterator<_Iter>
  {
    --__current_;
    return *this;
  }
  constexpr basic_const_iterator operator--(int)
    requires bidirectional_iterator<_Iter>
  {
    auto __tmp = *this;
    --*this;
    return __tmp;
  }
  constexpr basic_const_iterator& operator+=(difference_type n)
    requires random_access_iterator<_Iter>
  {
    __current_ += n;
    return *this;
  }
  constexpr basic_const_iterator& operator-=(difference_type n)
    requires random_access_iterator<_Iter>
  {
    __current_ -= n;
    return *this;
  }
  constexpr reference operator[](difference_type n) const
    requires random_access_iterator<_Iter>
  {
    return static_cast<reference>(__current_[n]);
  }

  template <sentinel_for<_Iter> _Sp>
  constexpr bool operator==(const _Sp& s) const {
    return __current_ == s;
  }

  template <__ycxx::__detail::__not_a_const_iterator _CI>
    requires __ycxx::__detail::__constant_iterator<_CI> && convertible_to<const _Iter&, _CI>
  constexpr operator _CI() const& {
    return __current_;
  }
  template <__ycxx::__detail::__not_a_const_iterator _CI>
    requires __ycxx::__detail::__constant_iterator<_CI> && convertible_to<_Iter, _CI>
  constexpr operator _CI() && {
    return static_cast<_Iter&&>(__current_);
  }

  constexpr bool operator<(const basic_const_iterator& y) const
    requires random_access_iterator<_Iter>
  {
    return __current_ < y.__current_;
  }
  constexpr bool operator>(const basic_const_iterator& y) const
    requires random_access_iterator<_Iter>
  {
    return __current_ > y.__current_;
  }
  constexpr bool operator<=(const basic_const_iterator& y) const
    requires random_access_iterator<_Iter>
  {
    return __current_ <= y.__current_;
  }
  constexpr bool operator>=(const basic_const_iterator& y) const
    requires random_access_iterator<_Iter>
  {
    return __current_ >= y.__current_;
  }
  constexpr auto operator<=>(const basic_const_iterator& y) const
    requires random_access_iterator<_Iter> && three_way_comparable<_Iter>
  {
    return __current_ <=> y.__current_;
  }

  template <__ycxx::__detail::__different_from<basic_const_iterator> _Ip>
  constexpr bool operator<(const _Ip& y) const
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __current_ < y;
  }
  template <__ycxx::__detail::__different_from<basic_const_iterator> _Ip>
  constexpr bool operator>(const _Ip& y) const
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __current_ > y;
  }
  template <__ycxx::__detail::__different_from<basic_const_iterator> _Ip>
  constexpr bool operator<=(const _Ip& y) const
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __current_ <= y;
  }
  template <__ycxx::__detail::__different_from<basic_const_iterator> _Ip>
  constexpr bool operator>=(const _Ip& y) const
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __current_ >= y;
  }
  template <__ycxx::__detail::__different_from<basic_const_iterator> _Ip>
  constexpr auto operator<=>(const _Ip& y) const
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip> && three_way_comparable_with<_Iter, _Ip>
  {
    return __current_ <=> y;
  }
  template <__ycxx::__detail::__not_a_const_iterator _Ip>
  friend constexpr bool operator<(const _Ip& __x, const basic_const_iterator& y)
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __x < y.__current_;
  }
  template <__ycxx::__detail::__not_a_const_iterator _Ip>
  friend constexpr bool operator>(const _Ip& __x, const basic_const_iterator& y)
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __x > y.__current_;
  }
  template <__ycxx::__detail::__not_a_const_iterator _Ip>
  friend constexpr bool operator<=(const _Ip& __x, const basic_const_iterator& y)
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __x <= y.__current_;
  }
  template <__ycxx::__detail::__not_a_const_iterator _Ip>
  friend constexpr bool operator>=(const _Ip& __x, const basic_const_iterator& y)
    requires random_access_iterator<_Iter> && __ycxx::__detail::__const_iter_less_with<_Iter, _Ip> &&
             totally_ordered_with<_Iter, _Ip>
  {
    return __x >= y.__current_;
  }

  friend constexpr basic_const_iterator operator+(const basic_const_iterator& i, difference_type n)
    requires random_access_iterator<_Iter>
  {
    return basic_const_iterator(i.__current_ + n);
  }
  friend constexpr basic_const_iterator operator+(difference_type n, const basic_const_iterator& i)
    requires random_access_iterator<_Iter>
  {
    return basic_const_iterator(i.__current_ + n);
  }
  friend constexpr basic_const_iterator operator-(const basic_const_iterator& i, difference_type n)
    requires random_access_iterator<_Iter>
  {
    return basic_const_iterator(i.__current_ - n);
  }
  template <sized_sentinel_for<_Iter> _Sp>
  constexpr difference_type operator-(const _Sp& y) const {
    return __current_ - y;
  }
  // The right operand is deduced (a basic_const_iterator or a class derived from it) instead of
  // converted to basic_const_iterator: an S whose associated classes include this one (such as
  // optional<basic_const_iterator<Iter>>, which join_view stores) would otherwise make
  // sized_sentinel_for<S, Iter> ask for s - i, which considers this friend with the same S again
  // (Iter converts to basic_const_iterator): a constraint that depends on itself (libstdc++
  // PR 115046). Only through ADL with an operand that converts but is not derived does it differ.
  template <__ycxx::__detail::__not_a_const_iterator _Sp, class _Self>
    requires derived_from<_Self, basic_const_iterator> && sized_sentinel_for<_Sp, _Iter>
  friend constexpr difference_type operator-(const _Sp& __x, const _Self& y) {
    return __x - static_cast<const basic_const_iterator&>(y).__current_;
  }

  friend constexpr __rvalue_reference iter_move(const basic_const_iterator& i) noexcept(
      noexcept(static_cast<__rvalue_reference>(ranges::iter_move(i.__current_)))) {
    return static_cast<__rvalue_reference>(ranges::iter_move(i.__current_));
  }
};

template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<basic_const_iterator<_Tp>, _Up> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};
template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<_Up, basic_const_iterator<_Tp>> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};
template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<basic_const_iterator<_Tp>, basic_const_iterator<_Up>> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};

template <input_iterator _Ip>
using const_iterator = conditional_t<__ycxx::__detail::__constant_iterator<_Ip>, _Ip, basic_const_iterator<_Ip>>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Sp>
struct __const_sentinel_impl {
  using type = _Sp;
};
template <std::input_iterator _Sp>
struct __const_sentinel_impl<_Sp> {
  using type = std::const_iterator<_Sp>;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <semiregular _Sp>
using const_sentinel = typename __ycxx::__detail::__const_sentinel_impl<_Sp>::type;

template <input_iterator _Ip>
constexpr const_iterator<_Ip> make_const_iterator(_Ip __it) {
  return __it;
}
template <semiregular _Sp>
constexpr const_sentinel<_Sp> make_const_sentinel(_Sp s) {
  return s;
}

// =============================================================================================
// move_iterator / move_sentinel
// =============================================================================================
template <semiregular _Sp>
class move_sentinel {
  _Sp __last_ = _Sp();

public:
  constexpr move_sentinel() = default;
  constexpr explicit move_sentinel(_Sp s) : __last_(static_cast<_Sp&&>(s)) {}
  template <class _S2>
    requires convertible_to<const _S2&, _Sp>
  constexpr move_sentinel(const move_sentinel<_S2>& s) : __last_(s.base()) {}
  template <class _S2>
    requires assignable_from<_Sp&, const _S2&>
  constexpr move_sentinel& operator=(const move_sentinel<_S2>& s) {
    __last_ = s.base();
    return *this;
  }
  constexpr _Sp base() const { return __last_; }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Ip>
consteval auto __move_iter_concept() {
  if constexpr (std::random_access_iterator<_Ip>)
    return std::type_identity<std::random_access_iterator_tag>{};
  else if constexpr (std::bidirectional_iterator<_Ip>)
    return std::type_identity<std::bidirectional_iterator_tag>{};
  else if constexpr (std::forward_iterator<_Ip>)
    return std::type_identity<std::forward_iterator_tag>{};
  else
    return std::type_identity<std::input_iterator_tag>{};
}
}} // namespace __ycxx::__detail

// Base classes of std types live in __ycxx::__adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// __ycxx::__detail base would expose every internal function to lookup on the std type.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Ip>
struct __move_iter_category {};
template <class _Ip>
  requires requires { typename std::iterator_traits<_Ip>::iterator_category; }
struct __move_iter_category<_Ip> {
  using iterator_category =
      std::conditional_t<std::derived_from<typename std::iterator_traits<_Ip>::iterator_category,
                                           std::random_access_iterator_tag>,
                         std::random_access_iterator_tag, typename std::iterator_traits<_Ip>::iterator_category>;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Iterator>
class move_iterator : public __ycxx::__adl_free::__move_iter_category<_Iterator> {
  _Iterator __current_ = _Iterator();

public:
  using iterator_type = _Iterator;
  using iterator_concept = typename decltype(__ycxx::__detail::__move_iter_concept<_Iterator>())::type;
  using value_type = iter_value_t<_Iterator>;
  using difference_type = iter_difference_t<_Iterator>;
  using pointer = _Iterator;
  using reference = iter_rvalue_reference_t<_Iterator>;

  constexpr move_iterator()
    requires default_initializable<_Iterator>
  = default;
  constexpr explicit move_iterator(_Iterator i) : __current_(static_cast<_Iterator&&>(i)) {}
  template <class _Up>
    requires(!is_same_v<_Up, _Iterator> && convertible_to<const _Up&, _Iterator>)
  constexpr move_iterator(const move_iterator<_Up>& __u) : __current_(__u.base()) {}
  template <class _Up>
    requires(!is_same_v<_Up, _Iterator> && convertible_to<const _Up&, _Iterator> && assignable_from<_Iterator&, const _Up&>)
  constexpr move_iterator& operator=(const move_iterator<_Up>& __u) {
    __current_ = __u.base();
    return *this;
  }

  constexpr const _Iterator& base() const& noexcept { return __current_; }
  constexpr _Iterator base() && { return static_cast<_Iterator&&>(__current_); }

  constexpr reference operator*() const { return ranges::iter_move(__current_); }
  [[deprecated("move_iterator::operator-> is deprecated ([depr.move.iter.elem])")]]
  constexpr pointer operator->() const {
    return __current_;
  }

  constexpr move_iterator& operator++() {
    ++__current_;
    return *this;
  }
  constexpr auto operator++(int) {
    if constexpr (forward_iterator<_Iterator>) {
      move_iterator __tmp = *this;
      ++__current_;
      return __tmp;
    } else {
      ++__current_;
    }
  }
  constexpr move_iterator& operator--() {
    --__current_;
    return *this;
  }
  constexpr move_iterator operator--(int) {
    move_iterator __tmp = *this;
    --__current_;
    return __tmp;
  }
  constexpr move_iterator operator+(difference_type n) const { return move_iterator(__current_ + n); }
  constexpr move_iterator& operator+=(difference_type n) {
    __current_ += n;
    return *this;
  }
  constexpr move_iterator operator-(difference_type n) const { return move_iterator(__current_ - n); }
  constexpr move_iterator& operator-=(difference_type n) {
    __current_ -= n;
    return *this;
  }
  constexpr reference operator[](difference_type n) const { return ranges::iter_move(__current_ + n); }

  template <sentinel_for<_Iterator> _Sp>
  friend constexpr bool operator==(const move_iterator& __x, const move_sentinel<_Sp>& y) {
    return __x.base() == y.base();
  }
  template <sized_sentinel_for<_Iterator> _Sp>
  friend constexpr iter_difference_t<_Iterator> operator-(const move_sentinel<_Sp>& __x, const move_iterator& y) {
    return __x.base() - y.base();
  }
  template <sized_sentinel_for<_Iterator> _Sp>
  friend constexpr iter_difference_t<_Iterator> operator-(const move_iterator& __x, const move_sentinel<_Sp>& y) {
    return __x.base() - y.base();
  }
  friend constexpr iter_rvalue_reference_t<_Iterator> iter_move(const move_iterator& i) noexcept(
      noexcept(ranges::iter_move(i.__current_))) {
    return ranges::iter_move(i.__current_);
  }
  template <indirectly_swappable<_Iterator> _Iterator2>
  friend constexpr void iter_swap(const move_iterator& __x, const move_iterator<_Iterator2>& y) noexcept(
      noexcept(ranges::iter_swap(__x.__current_, y.base()))) {
    ranges::iter_swap(__x.__current_, y.base());
  }
};

template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x == y } -> convertible_to<bool>;
  }
constexpr bool operator==(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return __x.base() == y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x < y } -> convertible_to<bool>;
  }
constexpr bool operator<(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return __x.base() < y.base();
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { y < __x } -> convertible_to<bool>;
  }
constexpr bool operator>(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return y < __x;
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { y < __x } -> convertible_to<bool>;
  }
constexpr bool operator<=(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return !(y < __x);
}
template <class _I1, class _I2>
  requires requires(const _I1& __x, const _I2& y) {
    { __x < y } -> convertible_to<bool>;
  }
constexpr bool operator>=(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return !(__x < y);
}
template <class _I1, three_way_comparable_with<_I1> _I2>
constexpr compare_three_way_result_t<_I1, _I2> operator<=>(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) {
  return __x.base() <=> y.base();
}
template <class _I1, class _I2>
constexpr auto operator-(const move_iterator<_I1>& __x, const move_iterator<_I2>& y) -> decltype(__x.base() - y.base()) {
  return __x.base() - y.base();
}
template <class _Ip>
  requires requires(const _Ip& i, iter_difference_t<_Ip> n) {
    { i + n } -> same_as<_Ip>;
  }
constexpr move_iterator<_Ip> operator+(iter_difference_t<_Ip> n, const move_iterator<_Ip>& __x) {
  return __x + n;
}
template <class _Ip>
constexpr move_iterator<_Ip> make_move_iterator(_Ip i) {
  return move_iterator<_Ip>(static_cast<_Ip&&>(i));
}
template <class _I1, class _I2>
  requires(!sized_sentinel_for<_I1, _I2>)
constexpr bool disable_sized_sentinel_for<move_iterator<_I1>, move_iterator<_I2>> = true;

// =============================================================================================
// counted_iterator
// =============================================================================================
} // namespace std

// Base classes of std types live in __ycxx::__adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// __ycxx::__detail base would expose every internal function to lookup on the std type.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Ip>
struct __counted_value_type {};
template <std::indirectly_readable _Ip>
struct __counted_value_type<_Ip> {
  using value_type = std::iter_value_t<_Ip>;
};
template <class _Ip>
struct __counted_concept {};
template <class _Ip>
  requires requires { typename _Ip::iterator_concept; }
struct __counted_concept<_Ip> {
  using iterator_concept = typename _Ip::iterator_concept;
};
template <class _Ip>
struct __counted_category {};
template <class _Ip>
  requires requires { typename _Ip::iterator_category; }
struct __counted_category<_Ip> {
  using iterator_category = typename _Ip::iterator_category;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <input_or_output_iterator _Ip>
class counted_iterator : public __ycxx::__adl_free::__counted_value_type<_Ip>,
                         public __ycxx::__adl_free::__counted_concept<_Ip>,
                         public __ycxx::__adl_free::__counted_category<_Ip> {
  template <input_or_output_iterator _I2>
  friend class counted_iterator;

  _Ip __current_ = _Ip();
  iter_difference_t<_Ip> __length_ = 0;

public:
  using iterator_type = _Ip;
  using difference_type = iter_difference_t<_Ip>;

  constexpr counted_iterator()
    requires default_initializable<_Ip>
  = default;
  constexpr counted_iterator(_Ip __x, iter_difference_t<_Ip> n) : __current_(static_cast<_Ip&&>(__x)), __length_(n) {
    __ycxx::__detail::__precondition(n >= 0, "counted_iterator: negative count");
  }
  template <class _I2>
    requires convertible_to<const _I2&, _Ip>
  constexpr counted_iterator(const counted_iterator<_I2>& __x) : __current_(__x.__current_), __length_(__x.__length_) {}
  template <class _I2>
    requires assignable_from<_Ip&, const _I2&>
  constexpr counted_iterator& operator=(const counted_iterator<_I2>& __x) {
    __current_ = __x.__current_;
    __length_ = __x.__length_;
    return *this;
  }

  constexpr const _Ip& base() const& noexcept { return __current_; }
  constexpr _Ip base() && { return static_cast<_Ip&&>(__current_); }
  constexpr iter_difference_t<_Ip> count() const noexcept { return __length_; }

  constexpr decltype(auto) operator*() { return *__current_; }
  constexpr decltype(auto) operator*() const
    requires __ycxx::__detail::__dereferenceable<const _Ip>
  {
    return *__current_;
  }
  constexpr auto operator->() const noexcept
    requires contiguous_iterator<_Ip>
  {
    return std::to_address(__current_);
  }

  constexpr counted_iterator& operator++() {
    ++__current_;
    --__length_;
    return *this;
  }
  constexpr decltype(auto) operator++(int) {
    if constexpr (forward_iterator<_Ip>) {
      counted_iterator __tmp = *this;
      ++*this;
      return __tmp;
    } else {
      --__length_;
      if constexpr (__ycxx::__detail::__cfg::exceptions) {
        try {
          return __current_++;
        } catch (...) {
          ++__length_;
          throw;
        }
      } else {
        return __current_++;
      }
    }
  }
  constexpr counted_iterator& operator--()
    requires bidirectional_iterator<_Ip>
  {
    --__current_;
    ++__length_;
    return *this;
  }
  constexpr counted_iterator operator--(int)
    requires bidirectional_iterator<_Ip>
  {
    counted_iterator __tmp = *this;
    --*this;
    return __tmp;
  }
  constexpr counted_iterator operator+(iter_difference_t<_Ip> n) const
    requires random_access_iterator<_Ip>
  {
    return counted_iterator(__current_ + n, __length_ - n);
  }
  friend constexpr counted_iterator operator+(iter_difference_t<_Ip> n, const counted_iterator& __x)
    requires random_access_iterator<_Ip>
  {
    return __x + n;
  }
  constexpr counted_iterator& operator+=(iter_difference_t<_Ip> n)
    requires random_access_iterator<_Ip>
  {
    __current_ += n;
    __length_ -= n;
    return *this;
  }
  constexpr counted_iterator operator-(iter_difference_t<_Ip> n) const
    requires random_access_iterator<_Ip>
  {
    return counted_iterator(__current_ - n, __length_ + n);
  }
  template <common_with<_Ip> _I2>
  friend constexpr iter_difference_t<_I2> operator-(const counted_iterator& __x, const counted_iterator<_I2>& y) {
    return y.__length_ - __x.__length_;
  }
  friend constexpr iter_difference_t<_Ip> operator-(const counted_iterator& __x, default_sentinel_t) noexcept {
    return -__x.__length_;
  }
  friend constexpr iter_difference_t<_Ip> operator-(default_sentinel_t, const counted_iterator& y) noexcept {
    return y.__length_;
  }
  constexpr counted_iterator& operator-=(iter_difference_t<_Ip> n)
    requires random_access_iterator<_Ip>
  {
    __current_ -= n;
    __length_ += n;
    return *this;
  }
  constexpr decltype(auto) operator[](iter_difference_t<_Ip> n) const
    requires random_access_iterator<_Ip>
  {
    return __current_[n];
  }

  template <common_with<_Ip> _I2>
  friend constexpr bool operator==(const counted_iterator& __x, const counted_iterator<_I2>& y) {
    return __x.__length_ == y.__length_;
  }
  friend constexpr bool operator==(const counted_iterator& __x, default_sentinel_t) noexcept { return __x.__length_ == 0; }
  template <common_with<_Ip> _I2>
  friend constexpr strong_ordering operator<=>(const counted_iterator& __x, const counted_iterator<_I2>& y) {
    return y.__length_ <=> __x.__length_;
  }

  friend constexpr iter_rvalue_reference_t<_Ip> iter_move(const counted_iterator& i) noexcept(
      noexcept(ranges::iter_move(i.__current_)))
    requires input_iterator<_Ip>
  {
    return ranges::iter_move(i.__current_);
  }
  template <indirectly_swappable<_Ip> _I2>
  friend constexpr void iter_swap(const counted_iterator& __x, const counted_iterator<_I2>& y) noexcept(
      noexcept(ranges::iter_swap(__x.__current_, y.__current_))) {
    ranges::iter_swap(__x.__current_, y.__current_);
  }
};

template <input_iterator _Ip>
  requires same_as<__ycxx::__detail::__iter_traits<_Ip>, iterator_traits<_Ip>>
struct iterator_traits<counted_iterator<_Ip>> : iterator_traits<_Ip> {
  using pointer = conditional_t<contiguous_iterator<_Ip>, add_pointer_t<iter_reference_t<_Ip>>, void>;
};

// =============================================================================================
// common_iterator
// =============================================================================================
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [common.iter.nav]/5: the second condition of it++'s first form. A concept, so that
// iter_value_t<I> is formed only for a readable I (its conjuncts are checked in order).
template <class _Ip>
concept __common_iter_postfix_proxy = std::indirectly_readable<_Ip> &&
                                    std::constructible_from<std::iter_value_t<_Ip>, std::iter_reference_t<_Ip>> &&
                                    std::move_constructible<std::iter_value_t<_Ip>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <input_or_output_iterator _Ip, sentinel_for<_Ip> _Sp>
  requires(!same_as<_Ip, _Sp> && copyable<_Ip>)
class common_iterator {
  template <input_or_output_iterator _I2, sentinel_for<_I2> _S2>
    requires(!same_as<_I2, _S2> && copyable<_I2>)
  friend class common_iterator;

  // A two-alternative variant: index 0 = iterator, 1 = sentinel.
  union {
    _Ip __it_;
    _Sp __sent_;
  };
  unsigned char __index_;

  class proxy {
    iter_value_t<_Ip> __keep_;

  public:
    constexpr proxy(iter_reference_t<_Ip>&& __x) : __keep_(static_cast<iter_reference_t<_Ip>&&>(__x)) {}
    constexpr const iter_value_t<_Ip>* operator->() const noexcept { return __builtin_addressof(__keep_); }
  };
  class __postfix_proxy {
    iter_value_t<_Ip> __keep_;

  public:
    constexpr __postfix_proxy(iter_reference_t<_Ip>&& __x) : __keep_(static_cast<iter_reference_t<_Ip>&&>(__x)) {}
    constexpr const iter_value_t<_Ip>& operator*() const noexcept { return __keep_; }
  };

  constexpr void destroy() noexcept {
    if (__index_ == 0)
      std::destroy_at(__builtin_addressof(__it_));
    else if (__index_ == 1)
      std::destroy_at(__builtin_addressof(__sent_));
  }
  template <class _Other>
  constexpr void __copy_from(_Other&& __o) {
    if (__o.__index_ == 0)
      std::construct_at(__builtin_addressof(__it_), static_cast<_Other&&>(__o).__it_);
    else if (__o.__index_ == 1)
      std::construct_at(__builtin_addressof(__sent_), static_cast<_Other&&>(__o).__sent_);
    __index_ = __o.__index_; // 2 (valueless) copies as valueless
  }

public:
  constexpr common_iterator()
    requires default_initializable<_Ip>
      : __it_(), __index_(0) {}
  constexpr common_iterator(_Ip i) : __it_(static_cast<_Ip&&>(i)), __index_(0) {}
  constexpr common_iterator(_Sp s) : __sent_(static_cast<_Sp&&>(s)), __index_(1) {}
  template <class _I2, class _S2>
    requires convertible_to<const _I2&, _Ip> && convertible_to<const _S2&, _Sp>
  constexpr common_iterator(const common_iterator<_I2, _S2>& __x) : __index_(__x.__index_) {
    if (__x.__index_ == 0)
      std::construct_at(__builtin_addressof(__it_), __x.__it_);
    else
      std::construct_at(__builtin_addressof(__sent_), __x.__sent_);
  }
  constexpr common_iterator(const common_iterator& __x)
    requires(is_trivially_copy_constructible_v<_Ip> && is_trivially_copy_constructible_v<_Sp>)
  = default;
  constexpr common_iterator(const common_iterator& __x) : __index_(2) { __copy_from(__x); }
  constexpr common_iterator(common_iterator&& __x)
    requires(is_trivially_move_constructible_v<_Ip> && is_trivially_move_constructible_v<_Sp>)
  = default;
  constexpr common_iterator(common_iterator&& __x) : __index_(2) { __copy_from(static_cast<common_iterator&&>(__x)); }
  constexpr ~common_iterator()
    requires(is_trivially_destructible_v<_Ip> && is_trivially_destructible_v<_Sp>)
  = default;
  constexpr ~common_iterator() { destroy(); }

  template <class _I2, class _S2>
    requires convertible_to<const _I2&, _Ip> && convertible_to<const _S2&, _Sp> && assignable_from<_Ip&, const _I2&> &&
             assignable_from<_Sp&, const _S2&>
  constexpr common_iterator& operator=(const common_iterator<_I2, _S2>& __x) {
    if (__index_ == __x.__index_) {
      if (__index_ == 0)
        __it_ = __x.__it_;
      else
        __sent_ = __x.__sent_;
    } else {
      destroy();
      __index_ = 2;
      if (__x.__index_ == 0)
        std::construct_at(__builtin_addressof(__it_), __x.__it_);
      else
        std::construct_at(__builtin_addressof(__sent_), __x.__sent_);
      __index_ = __x.__index_;
    }
    return *this;
  }
  constexpr common_iterator& operator=(const common_iterator& __x)
    requires(is_trivially_copy_assignable_v<_Ip> && is_trivially_copy_assignable_v<_Sp> &&
             is_trivially_copy_constructible_v<_Ip> && is_trivially_copy_constructible_v<_Sp> &&
             is_trivially_destructible_v<_Ip> && is_trivially_destructible_v<_Sp>)
  = default;
  constexpr common_iterator& operator=(const common_iterator& __x) {
    if (this != &__x) {
      if (__index_ == __x.__index_ && __index_ == 0)
        __it_ = __x.__it_;
      else if (__index_ == __x.__index_ && __index_ == 1)
        __sent_ = __x.__sent_;
      else {
        destroy();
        __index_ = 2;
        __copy_from(__x);
      }
    }
    return *this;
  }
  constexpr common_iterator& operator=(common_iterator&& __x)
    requires(is_trivially_move_assignable_v<_Ip> && is_trivially_move_assignable_v<_Sp> &&
             is_trivially_move_constructible_v<_Ip> && is_trivially_move_constructible_v<_Sp> &&
             is_trivially_destructible_v<_Ip> && is_trivially_destructible_v<_Sp>)
  = default;
  constexpr common_iterator& operator=(common_iterator&& __x) {
    if (__index_ == __x.__index_ && __index_ == 0)
      __it_ = static_cast<_Ip&&>(__x.__it_);
    else if (__index_ == __x.__index_ && __index_ == 1)
      __sent_ = static_cast<_Sp&&>(__x.__sent_);
    else {
      destroy();
      __index_ = 2;
      __copy_from(static_cast<common_iterator&&>(__x));
    }
    return *this;
  }

  constexpr decltype(auto) operator*() {
    __ycxx::__detail::__precondition(__index_ == 0, "common_iterator: dereferencing a sentinel");
    return *__it_;
  }
  constexpr decltype(auto) operator*() const
    requires __ycxx::__detail::__dereferenceable<const _Ip>
  {
    __ycxx::__detail::__precondition(__index_ == 0, "common_iterator: dereferencing a sentinel");
    return *__it_;
  }
  constexpr auto operator->() const
    requires indirectly_readable<const _Ip> &&
             (requires(const _Ip& i) { i.operator->(); } || is_reference_v<iter_reference_t<_Ip>> ||
              constructible_from<iter_value_t<_Ip>, iter_reference_t<_Ip>>)
  {
    __ycxx::__detail::__precondition(__index_ == 0, "common_iterator: operator-> on a sentinel");
    if constexpr (is_pointer_v<_Ip> || requires(const _Ip& i) { i.operator->(); }) {
      return __it_;
    } else if constexpr (is_reference_v<iter_reference_t<_Ip>>) {
      auto&& __tmp = *__it_;
      return __builtin_addressof(__tmp);
    } else {
      return proxy(*__it_);
    }
  }

  constexpr common_iterator& operator++() {
    __ycxx::__detail::__precondition(__index_ == 0, "common_iterator: incrementing a sentinel");
    ++__it_;
    return *this;
  }
  constexpr decltype(auto) operator++(int) {
    __ycxx::__detail::__precondition(__index_ == 0, "common_iterator: incrementing a sentinel");
    if constexpr (forward_iterator<_Ip>) {
      common_iterator __tmp = *this;
      ++*this;
      return __tmp;
    } else if constexpr (requires(_Ip& i) {
                           { *i++ } -> __ycxx::__detail::__can_reference;
                         } || !__ycxx::__detail::__common_iter_postfix_proxy<_Ip>) {
      return __it_++;
    } else {
      __postfix_proxy p(*__it_);
      ++*this;
      return p;
    }
  }

  template <class _I2, sentinel_for<_Ip> _S2>
    requires sentinel_for<_Sp, _I2>
  friend constexpr bool operator==(const common_iterator& __x, const common_iterator<_I2, _S2>& y) {
    if (__x.__index_ == y.__index_) {
      if constexpr (equality_comparable_with<_Ip, _I2>) {
        if (__x.__index_ == 0)
          return __x.__it_ == y.__it_;
      }
      return true;
    }
    return __x.__index_ == 0 ? __x.__it_ == y.__sent_ : __x.__sent_ == y.__it_;
  }

  template <sized_sentinel_for<_Ip> _I2, sized_sentinel_for<_Ip> _S2>
    requires sized_sentinel_for<_Sp, _I2>
  friend constexpr iter_difference_t<_I2> operator-(const common_iterator& __x, const common_iterator<_I2, _S2>& y) {
    if (__x.__index_ == 1 && y.__index_ == 1)
      return 0;
    if (__x.__index_ == 0 && y.__index_ == 0)
      return __x.__it_ - y.__it_;
    return __x.__index_ == 0 ? __x.__it_ - y.__sent_ : __x.__sent_ - y.__it_;
  }

  friend constexpr decltype(auto) iter_move(const common_iterator& i) noexcept(noexcept(ranges::iter_move(declval<const _Ip&>())))
    requires input_iterator<_Ip>
  {
    return ranges::iter_move(i.__it_);
  }
  template <indirectly_swappable<_Ip> _I2, class _S2>
  friend constexpr void iter_swap(const common_iterator& __x, const common_iterator<_I2, _S2>& y) noexcept(
      noexcept(ranges::iter_swap(declval<const _Ip&>(), declval<const _I2&>()))) {
    ranges::iter_swap(__x.__it_, y.__it_);
  }
};

template <class _Ip, class _Sp>
struct incrementable_traits<common_iterator<_Ip, _Sp>> {
  using difference_type = iter_difference_t<_Ip>;
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Ip, class _Sp>
consteval auto __common_iter_pointer() {
  if constexpr (requires(const std::common_iterator<_Ip, _Sp>& a) { a.operator->(); })
    return std::type_identity<decltype(std::declval<const std::common_iterator<_Ip, _Sp>&>().operator->())>{};
  else
    return std::type_identity<void>{};
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
template <input_iterator _Ip, class _Sp>
struct iterator_traits<common_iterator<_Ip, _Sp>> {
  using iterator_concept = conditional_t<forward_iterator<_Ip>, forward_iterator_tag, input_iterator_tag>;
  using iterator_category = decltype([] {
    if constexpr (requires { typename iterator_traits<_Ip>::iterator_category; }) {
      if constexpr (derived_from<typename iterator_traits<_Ip>::iterator_category, forward_iterator_tag>)
        return forward_iterator_tag{};
      else
        return input_iterator_tag{};
    } else {
      return input_iterator_tag{};
    }
  }());
  using value_type = iter_value_t<_Ip>;
  using difference_type = iter_difference_t<_Ip>;
  using pointer = typename decltype(__ycxx::__detail::__common_iter_pointer<_Ip, _Sp>())::type;
  using reference = iter_reference_t<_Ip>;
};

} // namespace std

// =============================================================================================
// range access CPOs that need adaptors
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

namespace __rbegin_ns {
void rbegin() = delete; // hides outer declarations: the call below uses argument-dependent lookup only
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.rbegin()) } -> std::input_or_output_iterator;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(rbegin(t)) } -> std::input_or_output_iterator;
};
template <class _Tp>
concept __reversible = requires(_Tp& t) {
  { std::ranges::begin(t) } -> std::bidirectional_iterator;
  { std::ranges::end(t) } -> std::same_as<decltype(std::ranges::begin(t))>;
};
struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().rbegin()));
    else if constexpr (__adl<_Tp>)
      return noexcept(auto(rbegin(std::declval<_Tp&>())));
    else
      return noexcept(std::make_reverse_iterator(std::ranges::end(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && (__member<_Tp> || __adl<_Tp> || __reversible<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (__member<_Tp>)
      return t.rbegin();
    else if constexpr (__adl<_Tp>)
      return rbegin(t);
    else
      return std::make_reverse_iterator(std::ranges::end(t));
  }
};
} // namespace rbegin_ns
}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__range_access::__rbegin_ns::__fn rbegin{};
}
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {
namespace __rend_ns {
void rend() = delete; // hides outer declarations: the call below uses argument-dependent lookup only
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.rend()) } -> std::sentinel_for<decltype(std::ranges::rbegin(t))>;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(rend(t)) } -> std::sentinel_for<decltype(std::ranges::rbegin(t))>;
};
struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().rend()));
    else if constexpr (__adl<_Tp>)
      return noexcept(auto(rend(std::declval<_Tp&>())));
    else
      return noexcept(std::make_reverse_iterator(std::ranges::begin(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && (__member<_Tp> || __adl<_Tp> || __rbegin_ns::__reversible<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (__member<_Tp>)
      return t.rend();
    else if constexpr (__adl<_Tp>)
      return rend(t);
    else
      return std::make_reverse_iterator(std::ranges::begin(t));
  }
};
} // namespace rend_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__range_access::__rend_ns::__fn rend{};
} // namespace cpo

template <class _Tp>
concept constant_range = input_range<_Tp> && __ycxx::__detail::__constant_iterator<iterator_t<_Tp>>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

template <std::ranges::input_range _Rp>
constexpr auto& __possibly_const_range(_Rp& r) noexcept {
  if constexpr (std::ranges::input_range<const _Rp>)
    return const_cast<const _Rp&>(r);
  else
    return r;
}

template <class _Tp>
constexpr auto __as_const_pointer(const _Tp* p) noexcept {
  return p;
}

namespace __cbegin_ns {
struct __fn {
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && requires(_Tp& t) { std::ranges::begin(::__ycxx::__detail::__range_access::__possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::const_iterator<decltype(std::ranges::begin(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
          std::ranges::begin(::__ycxx::__detail::__range_access::__possibly_const_range(t))))) {
    return std::const_iterator<decltype(std::ranges::begin(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
        std::ranges::begin(::__ycxx::__detail::__range_access::__possibly_const_range(t)));
  }
};
} // namespace cbegin_ns
namespace __cend_ns {
struct __fn {
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && requires(_Tp& t) { std::ranges::end(::__ycxx::__detail::__range_access::__possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::const_sentinel<decltype(std::ranges::end(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
          std::ranges::end(::__ycxx::__detail::__range_access::__possibly_const_range(t))))) {
    return std::const_sentinel<decltype(std::ranges::end(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
        std::ranges::end(::__ycxx::__detail::__range_access::__possibly_const_range(t)));
  }
};
} // namespace cend_ns
namespace __crbegin_ns {
struct __fn {
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && requires(_Tp& t) { std::ranges::rbegin(::__ycxx::__detail::__range_access::__possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::const_iterator<decltype(std::ranges::rbegin(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
          std::ranges::rbegin(::__ycxx::__detail::__range_access::__possibly_const_range(t))))) {
    return std::const_iterator<decltype(std::ranges::rbegin(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
        std::ranges::rbegin(::__ycxx::__detail::__range_access::__possibly_const_range(t)));
  }
};
} // namespace crbegin_ns
namespace __crend_ns {
struct __fn {
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && requires(_Tp& t) { std::ranges::rend(::__ycxx::__detail::__range_access::__possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::const_sentinel<decltype(std::ranges::rend(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
          std::ranges::rend(::__ycxx::__detail::__range_access::__possibly_const_range(t))))) {
    return std::const_sentinel<decltype(std::ranges::rend(::__ycxx::__detail::__range_access::__possibly_const_range(t)))>(
        std::ranges::rend(::__ycxx::__detail::__range_access::__possibly_const_range(t)));
  }
};
} // namespace crend_ns
namespace __cdata_ns {
struct __fn {
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && requires(_Tp& t) { std::ranges::data(::__ycxx::__detail::__range_access::__possibly_const_range(t)); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::ranges::data(::__ycxx::__detail::__range_access::__possibly_const_range(t)))) {
    return ::__ycxx::__detail::__range_access::__as_const_pointer(std::ranges::data(::__ycxx::__detail::__range_access::__possibly_const_range(t)));
  }
};
} // namespace cdata_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace __cpo {
inline constexpr __ycxx::__detail::__range_access::__cbegin_ns::__fn cbegin{};
inline constexpr __ycxx::__detail::__range_access::__cend_ns::__fn cend{};
inline constexpr __ycxx::__detail::__range_access::__crbegin_ns::__fn crbegin{};
inline constexpr __ycxx::__detail::__range_access::__crend_ns::__fn crend{};
inline constexpr __ycxx::__detail::__range_access::__cdata_ns::__fn cdata{};
} // namespace cpo

template <range _Rp>
using const_iterator_t = decltype(ranges::cbegin(declval<_Rp&>()));
template <range _Rp>
using const_sentinel_t = decltype(ranges::cend(declval<_Rp&>()));
template <range _Rp>
using range_const_reference_t = iter_const_reference_t<iterator_t<_Rp>>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Cp>
constexpr auto rbegin(_Cp& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
  return c.rbegin();
}
template <class _Cp>
constexpr auto rbegin(const _Cp& c) noexcept(noexcept(c.rbegin())) -> decltype(c.rbegin()) {
  return c.rbegin();
}
template <class _Cp>
constexpr auto rend(_Cp& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
  return c.rend();
}
template <class _Cp>
constexpr auto rend(const _Cp& c) noexcept(noexcept(c.rend())) -> decltype(c.rend()) {
  return c.rend();
}
template <class _Tp, size_t _Np>
constexpr reverse_iterator<_Tp*> rbegin(_Tp (&a)[_Np]) noexcept {
  return reverse_iterator<_Tp*>(a + _Np);
}
template <class _Tp, size_t _Np>
constexpr reverse_iterator<_Tp*> rend(_Tp (&a)[_Np]) noexcept {
  return reverse_iterator<_Tp*>(a);
}
template <class _Ep>
constexpr reverse_iterator<const _Ep*> rbegin(initializer_list<_Ep> il) noexcept {
  return reverse_iterator<const _Ep*>(il.end());
}
template <class _Ep>
constexpr reverse_iterator<const _Ep*> rend(initializer_list<_Ep> il) noexcept {
  return reverse_iterator<const _Ep*>(il.begin());
}
template <class _Cp>
constexpr auto crbegin(const _Cp& c) noexcept(noexcept(std::rbegin(c))) -> decltype(std::rbegin(c)) {
  return std::rbegin(c);
}
template <class _Cp>
constexpr auto crend(const _Cp& c) noexcept(noexcept(std::rend(c))) -> decltype(std::rend(c)) {
  return std::rend(c);
}
} // namespace std
