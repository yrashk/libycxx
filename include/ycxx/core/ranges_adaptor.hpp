// libycxx core: the range adaptor machinery ([range.adaptor.object], [range.move.wrap],
// [range.nonprop.cache], [range.utility.helpers], [range.adaptor.helpers]):
// range_adaptor_closure and the pipe operators, the bound-argument closures adaptor(args...)
// returns, movable-box and non-propagating-cache, and the exposition-only helper concepts the
// views share.
#pragma once

#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/bind.hpp>
#include <ycxx/core/memory_base.hpp>

namespace std::ranges {
// [range.adaptor.object]/2: a class derived from range_adaptor_closure<D> (and not a range) is a
// range adaptor closure object type. The pipe operators below are found through this base.
template <class D>
  requires is_class_v<D> && same_as<D, remove_cv_t<D>>
class range_adaptor_closure {};
} // namespace std::ranges

namespace ycxx::detail {

template <bool Const, class T>
using maybe_const = std::conditional_t<Const, const T, T>;

template <class I>
concept has_arrow = std::input_iterator<I> && (std::is_pointer_v<I> || requires(const I i) { i.operator->(); });

template <class R>
concept range_with_movable_references = std::ranges::input_range<R> &&
                                        std::move_constructible<std::ranges::range_reference_t<R>> &&
                                        std::move_constructible<std::ranges::range_rvalue_reference_t<R>>;

template <class T>
constexpr T& as_lvalue(T&& t) noexcept {
  return static_cast<T&>(t);
}

// The type is derived from range_adaptor_closure of itself (an unambiguous base, so a cast works)
// and is not a range.
template <class T>
concept range_adaptor_closure_object =
    !std::ranges::range<std::remove_cvref_t<T>> &&
    requires(std::remove_cvref_t<T>& t) {
      static_cast<std::ranges::range_adaptor_closure<std::remove_cvref_t<T>>&>(t);
    } &&
    std::derived_from<std::remove_cvref_t<T>, std::ranges::range_adaptor_closure<std::remove_cvref_t<T>>>;

// iterator_category of the views' iterators: iterator_traits<I>::iterator_category.
template <class I>
using iter_category_t = typename std::iterator_traits<I>::iterator_category;

} // namespace ycxx::detail

namespace ycxx::adl_free {

// C | D ([range.adaptor.object]/1): a perfect forwarding call wrapper with call pattern d(c(arg)).
template <class C, class D>
struct pipe_closure : std::ranges::range_adaptor_closure<pipe_closure<C, D>> {
  [[no_unique_address]] C c;
  [[no_unique_address]] D d;

  template <class CC, class DD>
  constexpr pipe_closure(wrapper_init_t, CC&& cc, DD&& dd) : c(static_cast<CC&&>(cc)), d(static_cast<DD&&>(dd)) {}

  template <class Self, class R>
    requires std::invocable<ycxx::detail::forward_like_t<Self, C>, R> &&
             std::invocable<ycxx::detail::forward_like_t<Self, D>,
                            std::invoke_result_t<ycxx::detail::forward_like_t<Self, C>, R>>
  constexpr decltype(auto) operator()(this Self&& self, R&& r) noexcept(
      std::is_nothrow_invocable_v<ycxx::detail::forward_like_t<Self, C>, R> &&
      std::is_nothrow_invocable_v<ycxx::detail::forward_like_t<Self, D>,
                                  std::invoke_result_t<ycxx::detail::forward_like_t<Self, C>, R>>) {
    return ::ycxx::detail::invoke(std::forward_like<Self>(self.d),
                                  ::ycxx::detail::invoke(std::forward_like<Self>(self.c), static_cast<R&&>(r)));
  }
};

// adaptor(args...) ([range.adaptor.object]/8): the call pattern adaptor(r, bound_args...) is
// bind_back's, so the closure reuses its wrapper.
template <class Adaptor, class... Bound>
struct adaptor_closure : partial_wrapper<false, Adaptor, Bound...>,
                         std::ranges::range_adaptor_closure<adaptor_closure<Adaptor, Bound...>> {
  using partial_wrapper<false, Adaptor, Bound...>::partial_wrapper;
};

} // namespace ycxx::adl_free

namespace std::ranges {

// R | C is C(R); C | D composes. Declared in std::ranges, an associated namespace of every
// closure type through its range_adaptor_closure base.
template <class R, class C>
  requires(!ycxx::detail::range_adaptor_closure_object<R>) && ycxx::detail::range_adaptor_closure_object<C> &&
          invocable<C, R>
constexpr decltype(auto) operator|(R&& r, C&& c) noexcept(is_nothrow_invocable_v<C, R>) {
  return ::ycxx::detail::invoke(static_cast<C&&>(c), static_cast<R&&>(r));
}

template <class C, class D>
  requires ycxx::detail::range_adaptor_closure_object<C> && ycxx::detail::range_adaptor_closure_object<D> &&
           constructible_from<decay_t<C>, C> && constructible_from<decay_t<D>, D>
constexpr auto operator|(C&& c, D&& d) noexcept(is_nothrow_constructible_v<decay_t<C>, C> &&
                                                is_nothrow_constructible_v<decay_t<D>, D>) {
  return ycxx::adl_free::pipe_closure<decay_t<C>, decay_t<D>>(ycxx::adl_free::wrapper_init_t{}, static_cast<C&&>(c),
                                                              static_cast<D&&>(d));
}

} // namespace std::ranges

namespace ycxx::detail {

// adaptor(args...): the closure binding args (decayed copies) after the range argument.
template <class Adaptor, class... Args>
  requires(std::constructible_from<std::decay_t<Args>, Args> && ...)
constexpr auto bind_adaptor(const Adaptor& a, Args&&... args) noexcept(
    (std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...)) {
  return ycxx::adl_free::adaptor_closure<Adaptor, std::decay_t<Args>...>(ycxx::adl_free::wrapper_init_t{}, a,
                                                                         static_cast<Args&&>(args)...);
}

// ---- [range.move.wrap] movable-box -----------------------------------------------------------
// Following the recommended practice, only a T is stored when assignment can be done without an
// empty state: T is copyable (movable, for a move-only T), or construction cannot throw.
// Assignment by destroy-and-reconstruct writes the whole object, so that form keeps T in a plain
// member: a [[no_unique_address]] T could share its tail padding with a neighbour.
template <class T>
concept boxable = std::move_constructible<T> && std::is_object_v<T>;
template <class T>
concept box_assignable = (std::copy_constructible<T> && std::copyable<T>) || (!std::copy_constructible<T> && std::movable<T>);
template <class T>
concept box_nothrow =
    std::is_nothrow_move_constructible_v<T> && (!std::copy_constructible<T> || std::is_nothrow_copy_constructible_v<T>);

template <class T>
  requires boxable<T>
class movable_box {
  // The general form: an optional<T>.
  union {
    T value_;
  };
  bool engaged_ = false;

  template <class... Args>
  constexpr void construct(Args&&... args) {
    std::construct_at(__builtin_addressof(value_), static_cast<Args&&>(args)...);
    engaged_ = true;
  }

public:
  constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
    requires std::default_initializable<T>
      : value_(), engaged_(true) {}
  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr explicit movable_box(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : value_(static_cast<Args&&>(args)...), engaged_(true) {}

  constexpr movable_box(const movable_box& o) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::copy_constructible<T>
  {
    if (o.engaged_)
      construct(o.value_);
  }
  constexpr movable_box(movable_box&& o) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (o.engaged_)
      construct(std::move(o.value_));
  }
  constexpr movable_box& operator=(const movable_box& o) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::copy_constructible<T>
  {
    if (this != __builtin_addressof(o)) {
      reset();
      if (o.engaged_)
        construct(o.value_);
    }
    return *this;
  }
  constexpr movable_box& operator=(movable_box&& o) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (this != __builtin_addressof(o)) {
      reset();
      if (o.engaged_)
        construct(std::move(o.value_));
    }
    return *this;
  }
  constexpr ~movable_box() { reset(); }

  constexpr void reset() noexcept {
    if (engaged_) {
      std::destroy_at(__builtin_addressof(value_));
      engaged_ = false;
    }
  }
  constexpr bool has_value() const noexcept { return engaged_; }
  constexpr T& operator*() & noexcept { return value_; }
  constexpr const T& operator*() const& noexcept { return value_; }
  constexpr T&& operator*() && noexcept { return std::move(value_); }
  constexpr const T&& operator*() const&& noexcept { return std::move(value_); }
  constexpr T* operator->() noexcept { return __builtin_addressof(value_); }
  constexpr const T* operator->() const noexcept { return __builtin_addressof(value_); }
};

template <class T>
  requires boxable<T> && box_assignable<T>
class movable_box<T> {
  [[no_unique_address]] T value_;

public:
  constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
    requires std::default_initializable<T>
      : value_() {}
  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr explicit movable_box(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : value_(static_cast<Args&&>(args)...) {}

  constexpr bool has_value() const noexcept { return true; }
  constexpr T& operator*() & noexcept { return value_; }
  constexpr const T& operator*() const& noexcept { return value_; }
  constexpr T&& operator*() && noexcept { return std::move(value_); }
  constexpr const T&& operator*() const&& noexcept { return std::move(value_); }
  constexpr T* operator->() noexcept { return __builtin_addressof(value_); }
  constexpr const T* operator->() const noexcept { return __builtin_addressof(value_); }
};

template <class T>
  requires boxable<T> && (!box_assignable<T>) && box_nothrow<T>
class movable_box<T> {
  T value_;

public:
  constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
    requires std::default_initializable<T>
      : value_() {}
  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr explicit movable_box(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : value_(static_cast<Args&&>(args)...) {}

  movable_box(const movable_box&) = default;
  movable_box(movable_box&&) = default;
  constexpr movable_box& operator=(const movable_box& o) noexcept
    requires std::copy_constructible<T>
  {
    if (this != __builtin_addressof(o)) {
      std::destroy_at(__builtin_addressof(value_));
      std::construct_at(__builtin_addressof(value_), o.value_);
    }
    return *this;
  }
  constexpr movable_box& operator=(movable_box&& o) noexcept {
    if (this != __builtin_addressof(o)) {
      std::destroy_at(__builtin_addressof(value_));
      std::construct_at(__builtin_addressof(value_), std::move(o.value_));
    }
    return *this;
  }

  constexpr bool has_value() const noexcept { return true; }
  constexpr T& operator*() & noexcept { return value_; }
  constexpr const T& operator*() const& noexcept { return value_; }
  constexpr T&& operator*() && noexcept { return std::move(value_); }
  constexpr const T&& operator*() const&& noexcept { return std::move(value_); }
  constexpr T* operator->() noexcept { return __builtin_addressof(value_); }
  constexpr const T* operator->() const noexcept { return __builtin_addressof(value_); }
};

// ---- [range.nonprop.cache] non-propagating-cache ----------------------------------------------
// An optional<T> that is emptied rather than copied or moved.
template <class T>
  requires std::is_object_v<T>
class non_propagating_cache {
  union {
    T value_;
  };
  bool engaged_ = false;

public:
  constexpr non_propagating_cache() noexcept {}
  constexpr non_propagating_cache(const non_propagating_cache&) noexcept {}
  constexpr non_propagating_cache(non_propagating_cache&& other) noexcept { other.reset(); }
  constexpr non_propagating_cache& operator=(const non_propagating_cache& other) noexcept {
    if (__builtin_addressof(other) != this)
      reset();
    return *this;
  }
  constexpr non_propagating_cache& operator=(non_propagating_cache&& other) noexcept {
    reset();
    other.reset();
    return *this;
  }
  constexpr ~non_propagating_cache() { reset(); }

  constexpr void reset() noexcept {
    if (engaged_) {
      engaged_ = false;
      std::destroy_at(__builtin_addressof(value_));
    }
  }
  constexpr bool has_value() const noexcept { return engaged_; }
  constexpr T& operator*() noexcept { return value_; }
  constexpr const T& operator*() const noexcept { return value_; }
  constexpr T* operator->() noexcept { return __builtin_addressof(value_); }
  constexpr const T* operator->() const noexcept { return __builtin_addressof(value_); }

  template <class... Args>
  constexpr T& emplace(Args&&... args) {
    reset();
    std::construct_at(__builtin_addressof(value_), static_cast<Args&&>(args)...);
    engaged_ = true;
    return value_;
  }
  // Direct-non-list-initialization from *i: a prvalue *i initializes the value in place.
  template <class I>
  constexpr T& emplace_deref(const I& i) {
    reset();
    ::new (static_cast<void*>(__builtin_addressof(value_))) T(*i);
    engaged_ = true;
    return value_;
  }
};

// A non-propagating-cache member that is present only when Present is true.
struct empty_cache {};
template <bool Present, class T>
struct cache_select {
  using type = empty_cache;
};
template <class T>
struct cache_select<true, T> {
  using type = non_propagating_cache<T>;
};
template <bool Present, class T>
using cache_if = typename cache_select<Present, T>::type;

} // namespace ycxx::detail

namespace ycxx::adl_free {
// Bases that give a view's iterator its iterator_category member, or none ("not always present").
struct no_iterator_category {};
template <class Tag>
struct with_iterator_category {
  using iterator_category = Tag;
};
} // namespace ycxx::adl_free

namespace ycxx::detail {

// Tag is void: no iterator_category member.
template <class Tag>
using category_base = std::conditional_t<std::is_void_v<Tag>, ycxx::adl_free::no_iterator_category,
                                         ycxx::adl_free::with_iterator_category<Tag>>;

// The iterator_concept most views give their iterator: the strength of the underlying range,
// capped at random access.
template <class Base>
consteval auto range_strength() {
  if constexpr (std::ranges::random_access_range<Base>)
    return std::random_access_iterator_tag{};
  else if constexpr (std::ranges::bidirectional_range<Base>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (std::ranges::forward_range<Base>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}
template <class Base>
using range_strength_t = decltype(::ycxx::detail::range_strength<Base>());

// Access to the private members of the views' iterators and sentinels from the hidden friends of
// their sibling classes (befriending a class does not reliably extend to its hidden friends).
// Every such iterator and sentinel declares `friend ycxx::detail::view_access;`.
struct view_access {
  template <class T>
  static constexpr auto&& current(T&& t) noexcept {
    return static_cast<T&&>(t).current_;
  }
  template <class T>
  static constexpr auto&& parent(T&& t) noexcept {
    return static_cast<T&&>(t).parent_;
  }
  template <class T>
  static constexpr auto&& end(T&& t) noexcept {
    return static_cast<T&&>(t).end_;
  }
};

} // namespace ycxx::detail
