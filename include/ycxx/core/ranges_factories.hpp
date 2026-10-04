// libycxx core: the range factories ([range.factories]): empty_view, single_view, iota_view
// (views::iota, views::indices), repeat_view and basic_istream_view.
#pragma once

#include <ycxx/core/ranges_adaptor.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/char_traits.hpp>

namespace std::ranges {

// [range.empty]
template <class T>
  requires is_object_v<T>
class empty_view : public view_interface<empty_view<T>> {
public:
  static constexpr T* begin() noexcept { return nullptr; }
  static constexpr T* end() noexcept { return nullptr; }
  static constexpr T* data() noexcept { return nullptr; }
  static constexpr size_t size() noexcept { return 0; }
  static constexpr bool empty() noexcept { return true; }
};
template <class T>
constexpr bool enable_borrowed_range<empty_view<T>> = true;

namespace views {
template <class T>
constexpr empty_view<T> empty{};
} // namespace views

// [range.single]
template <move_constructible T>
  requires is_object_v<T>
class single_view : public view_interface<single_view<T>> {
  ycxx::detail::movable_box<T> value_;

public:
  // The constructors are noexcept when constructing T is (a permitted strengthening).
  single_view()
    requires default_initializable<T>
  = default;
  constexpr explicit single_view(const T& t) noexcept(is_nothrow_copy_constructible_v<T>)
    requires copy_constructible<T>
      : value_(in_place, t) {}
  constexpr explicit single_view(T&& t) noexcept(is_nothrow_move_constructible_v<T>) : value_(in_place, std::move(t)) {}
  template <class... Args>
    requires constructible_from<T, Args...>
  constexpr explicit single_view(in_place_t, Args&&... args) noexcept(is_nothrow_constructible_v<T, Args...>)
      : value_(in_place, static_cast<Args&&>(args)...) {}

  constexpr T* begin() noexcept { return data(); }
  constexpr const T* begin() const noexcept { return data(); }
  constexpr T* end() noexcept { return data() + 1; }
  constexpr const T* end() const noexcept { return data() + 1; }
  static constexpr bool empty() noexcept { return false; }
  static constexpr size_t size() noexcept { return 1; }
  constexpr T* data() noexcept { return value_.operator->(); }
  constexpr const T* data() const noexcept { return value_.operator->(); }
};
template <class T>
single_view(T) -> single_view<T>;

} // namespace std::ranges

namespace ycxx::detail {

// ---- [range.iota.view] -------------------------------------------------------------------------
// IOTA-DIFF-T(W): a signed type wider than an integral W; int128 serves the 64-bit types (and
// itself, as the signed-integer-like type of width not less than W).
template <class W>
consteval auto iota_diff() {
  if constexpr (!std::is_integral_v<W> || sizeof(std::iter_difference_t<W>) > sizeof(W))
    return std::type_identity<std::iter_difference_t<W>>{};
  else if constexpr (sizeof(signed char) > sizeof(W))
    return std::type_identity<signed char>{};
  else if constexpr (sizeof(short) > sizeof(W))
    return std::type_identity<short>{};
  else if constexpr (sizeof(int) > sizeof(W))
    return std::type_identity<int>{};
  else if constexpr (sizeof(long long) > sizeof(W))
    return std::type_identity<long long>{};
  else if constexpr (cfg::has_int128)
    return std::type_identity<int128>{};
  else
    return std::type_identity<long long>{};
}
template <class W>
using iota_diff_t = typename decltype(::ycxx::detail::iota_diff<W>())::type;

template <class I>
concept decrementable = std::incrementable<I> && requires(I i) {
  { --i } -> std::same_as<I&>;
  { i-- } -> std::same_as<I>;
};

template <class I>
concept advanceable = decrementable<I> && std::totally_ordered<I> && requires(I i, const I j, const iota_diff_t<I> n) {
  { i += n } -> std::same_as<I&>;
  { i -= n } -> std::same_as<I&>;
  I(j + n);
  I(n + j);
  I(j - n);
  { j - j } -> std::convertible_to<iota_diff_t<I>>;
};

template <class W>
consteval auto iota_concept() {
  if constexpr (advanceable<W>)
    return std::random_access_iterator_tag{};
  else if constexpr (decrementable<W>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (std::incrementable<W>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}

template <class W>
struct iota_category {};
template <class W>
  requires std::incrementable<W> && std::is_integral_v<iota_diff_t<W>>
struct iota_category<W> {
  using iterator_category = std::input_iterator_tag;
};

} // namespace ycxx::detail

namespace std::ranges {

template <weakly_incrementable W, semiregular Bound = unreachable_sentinel_t>
  requires ycxx::detail::weakly_equality_comparable_with<W, Bound> && copyable<W>
class iota_view : public view_interface<iota_view<W, Bound>> {
  struct sentinel;

  struct iterator : ycxx::detail::iota_category<W> {
  private:
    friend iota_view;
    W value_ = W();
    constexpr explicit iterator(W value) : value_(value) {}

  public:
    using iterator_concept = decltype(ycxx::detail::iota_concept<W>());
    using value_type = W;
    using difference_type = ycxx::detail::iota_diff_t<W>;

    iterator()
      requires default_initializable<W>
    = default;

    constexpr W operator*() const noexcept(is_nothrow_copy_constructible_v<W>) { return value_; }
    constexpr iterator& operator++() {
      ++value_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires incrementable<W>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires ycxx::detail::decrementable<W>
    {
      --value_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires ycxx::detail::decrementable<W>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires ycxx::detail::advanceable<W>
    {
      if constexpr (ycxx::detail::integer_like<W> && !ycxx::detail::signed_integer_like<W>) {
        if (n >= difference_type(0))
          value_ += static_cast<W>(n);
        else
          value_ -= static_cast<W>(-n);
      } else {
        value_ += n;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires ycxx::detail::advanceable<W>
    {
      if constexpr (ycxx::detail::integer_like<W> && !ycxx::detail::signed_integer_like<W>) {
        if (n >= difference_type(0))
          value_ -= static_cast<W>(n);
        else
          value_ += static_cast<W>(-n);
      } else {
        value_ -= n;
      }
      return *this;
    }
    constexpr W operator[](difference_type n) const
      requires ycxx::detail::advanceable<W>
    {
      return W(value_ + n);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires equality_comparable<W>
    {
      return x.value_ == y.value_;
    }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires totally_ordered<W>
    {
      return x.value_ < y.value_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires totally_ordered<W>
    {
      return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires totally_ordered<W>
    {
      return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires totally_ordered<W>
    {
      return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires totally_ordered<W> && three_way_comparable<W>
    {
      return x.value_ <=> y.value_;
    }
    friend constexpr iterator operator+(iterator i, difference_type n)
      requires ycxx::detail::advanceable<W>
    {
      i += n;
      return i;
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
      requires ycxx::detail::advanceable<W>
    {
      return i + n;
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
      requires ycxx::detail::advanceable<W>
    {
      i -= n;
      return i;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires ycxx::detail::advanceable<W>
    {
      using D = difference_type;
      if constexpr (ycxx::detail::integer_like<W>) {
        if constexpr (ycxx::detail::signed_integer_like<W>)
          return D(D(x.value_) - D(y.value_));
        else
          return (y.value_ > x.value_) ? D(-D(y.value_ - x.value_)) : D(x.value_ - y.value_);
      } else {
        return x.value_ - y.value_;
      }
    }
  };

private:
  struct sentinel {
  private:
    friend iota_view;
    Bound bound_ = Bound();
    constexpr explicit sentinel(Bound bound) : bound_(bound) {}

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator& x, const sentinel& y) { return x.value_ == y.bound_; }
    friend constexpr iter_difference_t<W> operator-(const iterator& x, const sentinel& y)
      requires sized_sentinel_for<Bound, W>
    {
      return x.value_ - y.bound_;
    }
    friend constexpr iter_difference_t<W> operator-(const sentinel& x, const iterator& y)
      requires sized_sentinel_for<Bound, W>
    {
      return -(y - x);
    }
  };

  using last_type =
      conditional_t<same_as<W, Bound>, iterator, conditional_t<same_as<Bound, unreachable_sentinel_t>, Bound, sentinel>>;

  [[no_unique_address]] W value_ = W();
  [[no_unique_address]] Bound bound_ = Bound();

public:
  iota_view()
    requires default_initializable<W>
  = default;
  // The constructors are noexcept when copying W and Bound is (a permitted strengthening).
  constexpr explicit iota_view(W value) noexcept(is_nothrow_copy_constructible_v<W>) : value_(value) {
    if constexpr (totally_ordered_with<W, Bound>)
      ::ycxx::detail::precondition(bool(value_ <= bound_), "iota_view: the bound is not reachable from the value");
  }
  constexpr explicit iota_view(type_identity_t<W> value, type_identity_t<Bound> bound) noexcept(
      is_nothrow_copy_constructible_v<W> && is_nothrow_copy_constructible_v<Bound>)
      : value_(value), bound_(bound) {
    if constexpr (totally_ordered_with<W, Bound>)
      ::ycxx::detail::precondition(bool(value_ <= bound_), "iota_view: the bound is not reachable from the value");
  }
  constexpr explicit iota_view(iterator first, last_type last)
      : iota_view(first.value_, [&]() -> Bound {
          if constexpr (same_as<W, Bound>)
            return last.value_;
          else if constexpr (same_as<Bound, unreachable_sentinel_t>)
            return last;
          else
            return last.bound_;
        }()) {}

  constexpr iterator begin() const { return iterator{value_}; }
  constexpr auto end() const {
    if constexpr (same_as<Bound, unreachable_sentinel_t>)
      return unreachable_sentinel;
    else
      return sentinel{bound_};
  }
  constexpr iterator end() const
    requires same_as<W, Bound>
  {
    return iterator{bound_};
  }
  constexpr bool empty() const { return value_ == bound_; }
  constexpr auto size() const
    requires(same_as<W, Bound> && ycxx::detail::advanceable<W>) ||
            (ycxx::detail::integer_like<W> && ycxx::detail::integer_like<Bound>) || sized_sentinel_for<Bound, W>
  {
    using ycxx::detail::to_unsigned_like;
    if constexpr (ycxx::detail::integer_like<W> && ycxx::detail::integer_like<Bound>) {
      // The value of the specified expression, computed without negating a minimum value: both
      // operands converted (sign-extended) to a common unsigned type, whose modular difference
      // is the exact size. The result type is the specified one, made unsigned where integral
      // promotion turned it signed (narrow W).
      using R0 = decltype(to_unsigned_like(bound_) - to_unsigned_like(value_));
      using R = conditional_t<signed_integral<R0>, make_unsigned_t<R0>, R0>;
      using UC = make_unsigned_t<common_type_t<W, Bound>>;
      return static_cast<R>(static_cast<UC>(static_cast<UC>(bound_) - static_cast<UC>(value_)));
    } else {
      return to_unsigned_like(bound_ - value_);
    }
  }
};

template <class W, class Bound>
  requires(!ycxx::detail::integer_like<W> || !ycxx::detail::integer_like<Bound> ||
           (ycxx::detail::signed_integer_like<W> == ycxx::detail::signed_integer_like<Bound>))
iota_view(W, Bound) -> iota_view<W, Bound>;

template <class W, class Bound>
constexpr bool enable_borrowed_range<iota_view<W, Bound>> = true;

} // namespace std::ranges

namespace ycxx::detail {

// ---- [range.repeat.view] -----------------------------------------------------------------------
template <class T>
concept integer_like_with_usable_difference_type =
    signed_integer_like<T> || (integer_like<T> && std::weakly_incrementable<T>);

template <class T>
inline constexpr bool is_iota_view = false;
struct repeat_access;
template <class W, class B>
inline constexpr bool is_iota_view<std::ranges::iota_view<W, B>> = true;

} // namespace ycxx::detail

namespace std::ranges {

template <move_constructible T, semiregular Bound = unreachable_sentinel_t>
  requires(is_object_v<T> && same_as<T, remove_cv_t<T>> &&
           (ycxx::detail::integer_like_with_usable_difference_type<Bound> || same_as<Bound, unreachable_sentinel_t>))
class repeat_view : public view_interface<repeat_view<T, Bound>> {
  using index_type = conditional_t<same_as<Bound, unreachable_sentinel_t>, ptrdiff_t, Bound>;
  static constexpr bool bounded = !same_as<Bound, unreachable_sentinel_t>;

  // views::take / views::drop read the value ([range.take.overview]/2.5).
  friend struct ycxx::detail::repeat_access;

  [[no_unique_address]] ycxx::detail::movable_box<T> value_;
  [[no_unique_address]] Bound bound_ = Bound();

  class iterator {
    friend repeat_view;
    const T* value_ = nullptr;
    index_type current_ = index_type();

    constexpr explicit iterator(const T* value, index_type b = index_type()) : value_(value), current_(b) {
      if constexpr (bounded)
        ::ycxx::detail::precondition(b >= 0, "repeat_view: negative bound");
    }

  public:
    using iterator_concept = random_access_iterator_tag;
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type =
        conditional_t<ycxx::detail::signed_integer_like<index_type>, index_type, ycxx::detail::iota_diff_t<index_type>>;

    iterator() = default;
    constexpr const T& operator*() const noexcept { return *value_; }
    constexpr iterator& operator++() {
      ++current_;
      return *this;
    }
    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--() {
      if constexpr (bounded)
        ::ycxx::detail::precondition(current_ > 0, "repeat_view::iterator: decrement before the start");
      --current_;
      return *this;
    }
    constexpr iterator operator--(int) {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type n) {
      if constexpr (bounded)
        ::ycxx::detail::precondition(current_ + n >= 0, "repeat_view::iterator: advance before the start");
      current_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n) {
      if constexpr (bounded)
        ::ycxx::detail::precondition(current_ - n >= 0, "repeat_view::iterator: advance before the start");
      current_ -= n;
      return *this;
    }
    constexpr const T& operator[](difference_type n) const noexcept { return *(*this + n); }

    friend constexpr bool operator==(const iterator& x, const iterator& y) { return x.current_ == y.current_; }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y) { return x.current_ <=> y.current_; }
    friend constexpr iterator operator+(iterator i, difference_type n) {
      i += n;
      return i;
    }
    friend constexpr iterator operator+(difference_type n, iterator i) {
      i += n;
      return i;
    }
    friend constexpr iterator operator-(iterator i, difference_type n) {
      i -= n;
      return i;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y) {
      return static_cast<difference_type>(x.current_) - static_cast<difference_type>(y.current_);
    }
  };

public:
  repeat_view()
    requires default_initializable<T>
  = default;
  constexpr explicit repeat_view(const T& value, Bound bound = Bound())
    requires copy_constructible<T>
      : value_(in_place, value), bound_(bound) {
    if constexpr (bounded)
      ::ycxx::detail::precondition(bound >= 0, "repeat_view: negative bound");
  }
  constexpr explicit repeat_view(T&& value, Bound bound = Bound()) : value_(in_place, std::move(value)), bound_(bound) {
    if constexpr (bounded)
      ::ycxx::detail::precondition(bound >= 0, "repeat_view: negative bound");
  }
  template <class... TArgs, class... BoundArgs>
    requires constructible_from<T, TArgs...> && constructible_from<Bound, BoundArgs...>
  constexpr explicit repeat_view(piecewise_construct_t, tuple<TArgs...> value_args,
                                 tuple<BoundArgs...> bound_args = tuple<>{})
      : value_(in_place, std::make_from_tuple<T>(std::move(value_args))),
        bound_(std::make_from_tuple<Bound>(std::move(bound_args))) {
    if constexpr (bounded)
      ::ycxx::detail::precondition(bound_ >= 0, "repeat_view: negative bound");
  }

  constexpr iterator begin() const { return iterator(__builtin_addressof(*value_)); }
  constexpr iterator end() const
    requires(!same_as<Bound, unreachable_sentinel_t>)
  {
    return iterator(__builtin_addressof(*value_), bound_);
  }
  constexpr unreachable_sentinel_t end() const noexcept { return unreachable_sentinel; }
  constexpr auto size() const
    requires(!same_as<Bound, unreachable_sentinel_t>)
  {
    return ::ycxx::detail::to_unsigned_like(bound_);
  }
};

template <class T, class Bound = unreachable_sentinel_t>
repeat_view(T, Bound = Bound()) -> repeat_view<T, Bound>;

} // namespace std::ranges

namespace ycxx::detail {

template <class T>
inline constexpr bool is_repeat_view = false;
template <class T, class B>
inline constexpr bool is_repeat_view<std::ranges::repeat_view<T, B>> = true;

// The stored value of a repeat_view, for views::take and views::drop (*E.value_).
struct repeat_access {
  template <class R>
  static constexpr decltype(auto) value(R&& r) noexcept {
    return *static_cast<R&&>(r).value_;
  }
};

namespace view_fn {

struct single_fn {
  template <class T>
    requires requires { std::ranges::single_view<std::decay_t<T>>(std::declval<T>()); }
  [[nodiscard]] constexpr auto operator()(T&& t) const
      noexcept(noexcept(std::ranges::single_view<std::decay_t<T>>(static_cast<T&&>(t)))) {
    return std::ranges::single_view<std::decay_t<T>>(static_cast<T&&>(t));
  }
};

struct iota_fn {
  template <class W>
    requires requires { std::ranges::iota_view<std::decay_t<W>>(std::declval<W>()); }
  [[nodiscard]] constexpr auto operator()(W&& value) const
      noexcept(noexcept(std::ranges::iota_view<std::decay_t<W>>(static_cast<W&&>(value)))) {
    return std::ranges::iota_view<std::decay_t<W>>(static_cast<W&&>(value));
  }
  template <class W, class B>
    requires requires { std::ranges::iota_view(std::declval<W>(), std::declval<B>()); }
  [[nodiscard]] constexpr auto operator()(W&& value, B&& bound) const
      noexcept(noexcept(std::ranges::iota_view(static_cast<W&&>(value), static_cast<B&&>(bound)))) {
    return std::ranges::iota_view(static_cast<W&&>(value), static_cast<B&&>(bound));
  }
};

struct indices_fn {
  template <class E>
    requires integer_like<std::remove_cvref_t<E>> &&
             requires { iota_fn{}(std::remove_cvref_t<E>(0), std::declval<E>()); }
  [[nodiscard]] constexpr auto operator()(E&& e) const
      noexcept(noexcept(iota_fn{}(std::remove_cvref_t<E>(0), static_cast<E&&>(e)))) {
    return iota_fn{}(std::remove_cvref_t<E>(0), static_cast<E&&>(e));
  }
};

struct repeat_fn {
  template <class T>
    requires requires { std::ranges::repeat_view<std::decay_t<T>>(std::declval<T>()); }
  [[nodiscard]] constexpr auto operator()(T&& value) const
      noexcept(noexcept(std::ranges::repeat_view<std::decay_t<T>>(static_cast<T&&>(value)))) {
    return std::ranges::repeat_view<std::decay_t<T>>(static_cast<T&&>(value));
  }
  template <class T, class B>
    requires requires { std::ranges::repeat_view(std::declval<T>(), std::declval<B>()); }
  [[nodiscard]] constexpr auto operator()(T&& value, B&& bound) const
      noexcept(noexcept(std::ranges::repeat_view(static_cast<T&&>(value), static_cast<B&&>(bound)))) {
    return std::ranges::repeat_view(static_cast<T&&>(value), static_cast<B&&>(bound));
  }
};

} // namespace view_fn
} // namespace ycxx::detail

namespace std::ranges::views {
inline constexpr ycxx::detail::view_fn::single_fn single{};
inline constexpr ycxx::detail::view_fn::iota_fn iota{};
inline constexpr ycxx::detail::view_fn::indices_fn indices{};
inline constexpr ycxx::detail::view_fn::repeat_fn repeat{};
} // namespace std::ranges::views

// ---- [range.istream] ---------------------------------------------------------------------------
// The view needs only the stream's interface: basic_istream is declared here (without default
// arguments, which <istream>/<iosfwd> supply) and must be complete where the view is used.
namespace std {
template <class CharT, class Traits>
class basic_istream;
} // namespace std

namespace ycxx::detail {
template <class Val, class CharT, class Traits>
concept stream_extractable = requires(std::basic_istream<CharT, Traits>& is, Val& t) { is >> t; };
} // namespace ycxx::detail

namespace std::ranges {

template <movable Val, class CharT, class Traits = char_traits<CharT>>
  requires default_initializable<Val> && ycxx::detail::stream_extractable<Val, CharT, Traits>
class basic_istream_view : public view_interface<basic_istream_view<Val, CharT, Traits>> {
  class iterator {
    friend basic_istream_view;
    basic_istream_view* parent_;
    constexpr explicit iterator(basic_istream_view& parent) noexcept : parent_(__builtin_addressof(parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Val;

    iterator(const iterator&) = delete;
    iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = delete;
    iterator& operator=(iterator&&) = default;

    iterator& operator++() {
      *parent_->stream_ >> parent_->value_;
      return *this;
    }
    void operator++(int) { ++*this; }
    Val& operator*() const { return parent_->value_; }
    friend bool operator==(const iterator& x, default_sentinel_t) { return !*x.parent_->stream_; }
  };

  basic_istream<CharT, Traits>* stream_;
  Val value_ = Val();

public:
  constexpr explicit basic_istream_view(basic_istream<CharT, Traits>& stream) : stream_(__builtin_addressof(stream)) {}
  constexpr auto begin() {
    *stream_ >> value_;
    return iterator{*this};
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
};

template <class Val>
using istream_view = basic_istream_view<Val, char>;
template <class Val>
using wistream_view = basic_istream_view<Val, wchar_t>;

} // namespace std::ranges

namespace ycxx::detail::view_fn {
template <class T>
struct istream_fn {
  template <class E>
    requires requires {
      typename std::remove_cvref_t<E>::char_type;
      typename std::remove_cvref_t<E>::traits_type;
    } && std::derived_from<std::remove_cvref_t<E>, std::basic_istream<typename std::remove_cvref_t<E>::char_type,
                                                                     typename std::remove_cvref_t<E>::traits_type>> &&
             requires(E& e) {
               std::ranges::basic_istream_view<T, typename std::remove_cvref_t<E>::char_type,
                                               typename std::remove_cvref_t<E>::traits_type>(e);
             }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    using U = std::remove_cvref_t<E>;
    return std::ranges::basic_istream_view<T, typename U::char_type, typename U::traits_type>(e);
  }
};
} // namespace ycxx::detail::view_fn

namespace std::ranges::views {
template <class T>
constexpr ycxx::detail::view_fn::istream_fn<T> istream{};
} // namespace std::ranges::views
