// libycxx core: the range adaptor machinery ([range.adaptor.object], [range.move.wrap],
// [range.nonprop.cache], [range.utility.helpers], [range.adaptor.helpers]):
// range_adaptor_closure and the pipe operators, the bound-argument closures adaptor(args...)
// returns, movable-box and non-propagating-cache, and the exposition-only helper concepts the
// views share.
#pragma once

#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/bind.hpp>
#include <ycxx/core/memory_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {
// [range.adaptor.object]/2: a class derived from range_adaptor_closure<D> (and not a range) is a
// range adaptor closure object type. The pipe operators below are found through this base.
template <class _Dp>
  requires is_class_v<_Dp> && same_as<_Dp, remove_cv_t<_Dp>>
class range_adaptor_closure {};
}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <bool _Const, class _Tp>
using __maybe_const = std::conditional_t<_Const, const _Tp, _Tp>;

template <class _Ip>
concept __has_arrow = std::input_iterator<_Ip> && (std::is_pointer_v<_Ip> || requires(const _Ip i) { i.operator->(); });

template <class _Rp>
concept __range_with_movable_references = std::ranges::input_range<_Rp> &&
                                        std::move_constructible<std::ranges::range_reference_t<_Rp>> &&
                                        std::move_constructible<std::ranges::range_rvalue_reference_t<_Rp>>;

template <class _Tp>
constexpr _Tp& __as_lvalue(_Tp&& t) noexcept {
  return static_cast<_Tp&>(t);
}

// The type is publicly derived from range_adaptor_closure of itself, from no other specialization
// (deduction from a pointer then finds exactly one base), and is not a range.
template <class _Tp>
concept __range_adaptor_closure_object =
    !std::ranges::range<std::remove_cvref_t<_Tp>> &&
    std::derived_from<std::remove_cvref_t<_Tp>, std::ranges::range_adaptor_closure<std::remove_cvref_t<_Tp>>> &&
    requires(std::remove_cvref_t<_Tp>* p) { []<class _Up>(const std::ranges::range_adaptor_closure<_Up>*) {}(p); };

// iterator_category of the views' iterators: iterator_traits<I>::iterator_category.
template <class _Ip>
using __iter_category_t = typename std::iterator_traits<_Ip>::iterator_category;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// C | D ([range.adaptor.object]/1): a perfect forwarding call wrapper with call pattern d(c(arg)).
template <class _Cp, class _Dp>
struct __pipe_closure : std::ranges::range_adaptor_closure<__pipe_closure<_Cp, _Dp>> {
  [[no_unique_address]] _Cp c;
  [[no_unique_address]] _Dp d;

  template <class _CC, class _DD>
  constexpr __pipe_closure(__wrapper_init_t, _CC&& __cc, _DD&& __dd) : c(static_cast<_CC&&>(__cc)), d(static_cast<_DD&&>(__dd)) {}

  template <class _Self, class _Rp>
    requires __wrapper_castable<_Self, __pipe_closure> && std::invocable<__ycxx::__detail::__forward_like_t<_Self, _Cp>, _Rp> &&
             std::invocable<__ycxx::__detail::__forward_like_t<_Self, _Dp>,
                            std::invoke_result_t<__ycxx::__detail::__forward_like_t<_Self, _Cp>, _Rp>>
  constexpr decltype(auto) operator()(this _Self&& __self, _Rp&& r) noexcept(
      std::is_nothrow_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _Cp>, _Rp> &&
      std::is_nothrow_invocable_v<__ycxx::__detail::__forward_like_t<_Self, _Dp>,
                                  std::invoke_result_t<__ycxx::__detail::__forward_like_t<_Self, _Cp>, _Rp>>) {
    // Through the wrapper type: Self may be a class derived from it, even privately.
    auto&& __w = (__ycxx::__detail::__copy_cvref<_Self&&, __pipe_closure>)__self;
    return ::__ycxx::__detail::invoke(std::forward_like<_Self>(__w.d),
                                  ::__ycxx::__detail::invoke(std::forward_like<_Self>(__w.c), static_cast<_Rp&&>(r)));
  }
};

// adaptor(args...) ([range.adaptor.object]/8): the call pattern adaptor(r, bound_args...) is
// bind_back's, so the closure reuses its wrapper.
template <class _Adaptor, class... _Bound>
struct __adaptor_closure : __partial_wrapper<false, _Adaptor, _Bound...>,
                         std::ranges::range_adaptor_closure<__adaptor_closure<_Adaptor, _Bound...>> {
  using __wrapper = __partial_wrapper<false, _Adaptor, _Bound...>;
  using __wrapper::__wrapper;

  // The call pattern adaptor(r, bound_args...) has exactly one call argument: unlike bind_back's
  // wrapper, the closure is not callable with none or several (hides the wrapper's operator()).
  template <class _Self, class _Rp>
    requires(!std::is_volatile_v<std::remove_reference_t<_Self>>) && __wrapper_castable<_Self, __adaptor_closure> &&
            __wrapper::template __callable<_Self, _Rp>
  constexpr decltype(auto) operator()(this _Self&& __self, _Rp&& r) noexcept(__wrapper::template nothrow<_Self, _Rp>) {
    return ((__ycxx::__detail::__copy_cvref<_Self&&, __wrapper>)__self)(static_cast<_Rp&&>(r));
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

// R | C is C(R); C | D composes. Declared in std::ranges, an associated namespace of every
// closure type through its range_adaptor_closure base.
template <class _Rp, class _Cp>
  requires(!__ycxx::__detail::__range_adaptor_closure_object<_Rp>) && __ycxx::__detail::__range_adaptor_closure_object<_Cp> &&
          invocable<_Cp, _Rp>
constexpr decltype(auto) operator|(_Rp&& r, _Cp&& c) noexcept(is_nothrow_invocable_v<_Cp, _Rp>) {
  return ::__ycxx::__detail::invoke(static_cast<_Cp&&>(c), static_cast<_Rp&&>(r));
}

template <class _Cp, class _Dp>
  requires __ycxx::__detail::__range_adaptor_closure_object<_Cp> && __ycxx::__detail::__range_adaptor_closure_object<_Dp> &&
           constructible_from<decay_t<_Cp>, _Cp> && constructible_from<decay_t<_Dp>, _Dp>
constexpr auto operator|(_Cp&& c, _Dp&& d) noexcept(is_nothrow_constructible_v<decay_t<_Cp>, _Cp> &&
                                                is_nothrow_constructible_v<decay_t<_Dp>, _Dp>) {
  return __ycxx::__adl_free::__pipe_closure<decay_t<_Cp>, decay_t<_Dp>>(__ycxx::__adl_free::__wrapper_init_t{}, static_cast<_Cp&&>(c),
                                                              static_cast<_Dp&&>(d));
}

}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// adaptor(args...): the closure binding args (decayed copies) after the range argument.
template <class _Adaptor, class... _Args>
  requires(std::constructible_from<std::decay_t<_Args>, _Args> && ...)
constexpr auto __bind_adaptor(const _Adaptor& a, _Args&&... __args) noexcept(
    (std::is_nothrow_constructible_v<std::decay_t<_Args>, _Args> && ...)) {
  return __ycxx::__adl_free::__adaptor_closure<_Adaptor, std::decay_t<_Args>...>(__ycxx::__adl_free::__wrapper_init_t{}, a,
                                                                         static_cast<_Args&&>(__args)...);
}

// ---- [range.move.wrap] movable-box -----------------------------------------------------------
// Following the recommended practice, only a T is stored when assignment can be done without an
// empty state: T is copyable (movable, for a move-only T), or construction cannot throw.
// Assignment by destroy-and-reconstruct writes the whole object, so that form keeps T in a plain
// member: a [[no_unique_address]] T could share its tail padding with a neighbour.
template <class _Tp>
concept __boxable = std::move_constructible<_Tp> && std::is_object_v<_Tp>;
template <class _Tp>
concept __box_assignable = (std::copy_constructible<_Tp> && std::copyable<_Tp>) || (!std::copy_constructible<_Tp> && std::movable<_Tp>);
template <class _Tp>
concept __box_nothrow =
    std::is_nothrow_move_constructible_v<_Tp> && (!std::copy_constructible<_Tp> || std::is_nothrow_copy_constructible_v<_Tp>);

template <class _Tp>
  requires __boxable<_Tp>
class __movable_box {
  // The general form: an optional<T>.
  union {
    _Tp __value_;
  };
  bool __engaged_ = false;

  template <class... _Args>
  constexpr void construct(_Args&&... __args) {
    std::construct_at(__builtin_addressof(__value_), static_cast<_Args&&>(__args)...);
    __engaged_ = true;
  }

public:
  constexpr __movable_box() noexcept(std::is_nothrow_default_constructible_v<_Tp>)
    requires std::default_initializable<_Tp>
      : __value_(), __engaged_(true) {}
  template <class... _Args>
    requires std::constructible_from<_Tp, _Args...>
  constexpr explicit __movable_box(std::in_place_t, _Args&&... __args) noexcept(std::is_nothrow_constructible_v<_Tp, _Args...>)
      : __value_(static_cast<_Args&&>(__args)...), __engaged_(true) {}

  constexpr __movable_box(const __movable_box& __o) noexcept(std::is_nothrow_copy_constructible_v<_Tp>)
    requires std::copy_constructible<_Tp>
  {
    if (__o.__engaged_)
      construct(__o.__value_);
  }
  constexpr __movable_box(__movable_box&& __o) noexcept(std::is_nothrow_move_constructible_v<_Tp>) {
    if (__o.__engaged_)
      construct(std::move(__o.__value_));
  }
  constexpr __movable_box& operator=(const __movable_box& __o) noexcept(std::is_nothrow_copy_constructible_v<_Tp>)
    requires std::copy_constructible<_Tp>
  {
    if (this != __builtin_addressof(__o)) {
      reset();
      if (__o.__engaged_)
        construct(__o.__value_);
    }
    return *this;
  }
  constexpr __movable_box& operator=(__movable_box&& __o) noexcept(std::is_nothrow_move_constructible_v<_Tp>) {
    if (this != __builtin_addressof(__o)) {
      reset();
      if (__o.__engaged_)
        construct(std::move(__o.__value_));
    }
    return *this;
  }
  constexpr ~__movable_box() { reset(); }

  constexpr void reset() noexcept {
    if (__engaged_) {
      std::destroy_at(__builtin_addressof(__value_));
      __engaged_ = false;
    }
  }
  constexpr bool has_value() const noexcept { return __engaged_; }
  constexpr _Tp& operator*() & noexcept { return __value_; }
  constexpr const _Tp& operator*() const& noexcept { return __value_; }
  constexpr _Tp&& operator*() && noexcept { return std::move(__value_); }
  constexpr const _Tp&& operator*() const&& noexcept { return std::move(__value_); }
  constexpr _Tp* operator->() noexcept { return __builtin_addressof(__value_); }
  constexpr const _Tp* operator->() const noexcept { return __builtin_addressof(__value_); }
};

template <class _Tp>
  requires __boxable<_Tp> && __box_assignable<_Tp>
class __movable_box<_Tp> {
  [[no_unique_address]] _Tp __value_;

public:
  constexpr __movable_box() noexcept(std::is_nothrow_default_constructible_v<_Tp>)
    requires std::default_initializable<_Tp>
      : __value_() {}
  template <class... _Args>
    requires std::constructible_from<_Tp, _Args...>
  constexpr explicit __movable_box(std::in_place_t, _Args&&... __args) noexcept(std::is_nothrow_constructible_v<_Tp, _Args...>)
      : __value_(static_cast<_Args&&>(__args)...) {}

  constexpr bool has_value() const noexcept { return true; }
  constexpr _Tp& operator*() & noexcept { return __value_; }
  constexpr const _Tp& operator*() const& noexcept { return __value_; }
  constexpr _Tp&& operator*() && noexcept { return std::move(__value_); }
  constexpr const _Tp&& operator*() const&& noexcept { return std::move(__value_); }
  constexpr _Tp* operator->() noexcept { return __builtin_addressof(__value_); }
  constexpr const _Tp* operator->() const noexcept { return __builtin_addressof(__value_); }
};

template <class _Tp>
  requires __boxable<_Tp> && (!__box_assignable<_Tp>) && __box_nothrow<_Tp>
class __movable_box<_Tp> {
  _Tp __value_;

public:
  constexpr __movable_box() noexcept(std::is_nothrow_default_constructible_v<_Tp>)
    requires std::default_initializable<_Tp>
      : __value_() {}
  template <class... _Args>
    requires std::constructible_from<_Tp, _Args...>
  constexpr explicit __movable_box(std::in_place_t, _Args&&... __args) noexcept(std::is_nothrow_constructible_v<_Tp, _Args...>)
      : __value_(static_cast<_Args&&>(__args)...) {}

  __movable_box(const __movable_box&) = default;
  __movable_box(__movable_box&&) = default;
  constexpr __movable_box& operator=(const __movable_box& __o) noexcept
    requires std::copy_constructible<_Tp>
  {
    if (this != __builtin_addressof(__o)) {
      std::destroy_at(__builtin_addressof(__value_));
      std::construct_at(__builtin_addressof(__value_), __o.__value_);
    }
    return *this;
  }
  constexpr __movable_box& operator=(__movable_box&& __o) noexcept {
    if (this != __builtin_addressof(__o)) {
      std::destroy_at(__builtin_addressof(__value_));
      std::construct_at(__builtin_addressof(__value_), std::move(__o.__value_));
    }
    return *this;
  }

  constexpr bool has_value() const noexcept { return true; }
  constexpr _Tp& operator*() & noexcept { return __value_; }
  constexpr const _Tp& operator*() const& noexcept { return __value_; }
  constexpr _Tp&& operator*() && noexcept { return std::move(__value_); }
  constexpr const _Tp&& operator*() const&& noexcept { return std::move(__value_); }
  constexpr _Tp* operator->() noexcept { return __builtin_addressof(__value_); }
  constexpr const _Tp* operator->() const noexcept { return __builtin_addressof(__value_); }
};

// ---- [range.nonprop.cache] non-propagating-cache ----------------------------------------------
// An optional<T> that is emptied rather than copied or moved.
template <class _Tp>
  requires std::is_object_v<_Tp>
class __non_propagating_cache {
  union {
    _Tp __value_;
  };
  bool __engaged_ = false;

public:
  constexpr __non_propagating_cache() noexcept {}
  constexpr __non_propagating_cache(const __non_propagating_cache&) noexcept {}
  constexpr __non_propagating_cache(__non_propagating_cache&& other) noexcept { other.reset(); }
  constexpr __non_propagating_cache& operator=(const __non_propagating_cache& other) noexcept {
    if (__builtin_addressof(other) != this)
      reset();
    return *this;
  }
  constexpr __non_propagating_cache& operator=(__non_propagating_cache&& other) noexcept {
    reset();
    other.reset();
    return *this;
  }
  constexpr ~__non_propagating_cache() { reset(); }

  constexpr void reset() noexcept {
    if (__engaged_) {
      __engaged_ = false;
      std::destroy_at(__builtin_addressof(__value_));
    }
  }
  constexpr bool has_value() const noexcept { return __engaged_; }
  constexpr _Tp& operator*() noexcept { return __value_; }
  constexpr const _Tp& operator*() const noexcept { return __value_; }
  constexpr _Tp* operator->() noexcept { return __builtin_addressof(__value_); }
  constexpr const _Tp* operator->() const noexcept { return __builtin_addressof(__value_); }

  template <class... _Args>
  constexpr _Tp& emplace(_Args&&... __args) {
    reset();
    std::construct_at(__builtin_addressof(__value_), static_cast<_Args&&>(__args)...);
    __engaged_ = true;
    return __value_;
  }
  // Direct-non-list-initialization from *i: a prvalue *i initializes the value in place.
  template <class _Ip>
  constexpr _Tp& __emplace_deref(const _Ip& i) {
    reset();
    ::new (static_cast<void*>(__builtin_addressof(__value_))) _Tp(*i);
    __engaged_ = true;
    return __value_;
  }
};

// The type of an absent member ("present only if").
struct __empty_cache {};

// The position cached by the begin() of filter_view, drop_view, drop_while_view and
// reverse_view: an iterator into R, or for a random-access range its offset from the start (no
// iterator plus engaged flag is stored). Like non-propagating-cache, it is emptied rather than
// copied or moved.
template <class _Rp>
class __position_cache {
  __non_propagating_cache<std::ranges::iterator_t<_Rp>> __it_;

public:
  constexpr bool has_value() const noexcept { return __it_.has_value(); }
  constexpr std::ranges::iterator_t<_Rp> get(_Rp&) const { return *__it_; }
  constexpr void set(_Rp&, const std::ranges::iterator_t<_Rp>& __it) { __it_.emplace(__it); }
};
template <std::ranges::random_access_range _Rp>
class __position_cache<_Rp> {
  std::ranges::range_difference_t<_Rp> __offset_ = -1;

public:
  constexpr __position_cache() noexcept = default;
  constexpr __position_cache(const __position_cache&) noexcept {}
  constexpr __position_cache(__position_cache&& other) noexcept { other.__offset_ = -1; }
  constexpr __position_cache& operator=(const __position_cache& other) noexcept {
    if (__builtin_addressof(other) != this)
      __offset_ = -1;
    return *this;
  }
  constexpr __position_cache& operator=(__position_cache&& other) noexcept {
    __offset_ = -1;
    other.__offset_ = -1;
    return *this;
  }
  constexpr bool has_value() const noexcept { return __offset_ >= 0; }
  constexpr std::ranges::iterator_t<_Rp> get(_Rp& r) const { return std::ranges::begin(r) + __offset_; }
  constexpr void set(_Rp& r, const std::ranges::iterator_t<_Rp>& __it) { __offset_ = __it - std::ranges::begin(r); }
};
template <bool _Present, class _Rp>
struct __position_cache_select {
  using type = __empty_cache;
};
template <class _Rp>
struct __position_cache_select<true, _Rp> {
  using type = __position_cache<_Rp>;
};
template <bool _Present, class _Rp>
using __position_cache_if = typename __position_cache_select<_Present, _Rp>::type;

// A non-propagating-cache member that is present only when Present is true.
template <bool _Present, class _Tp>
struct __cache_select {
  using type = __empty_cache;
};
template <class _Tp>
struct __cache_select<true, _Tp> {
  using type = __non_propagating_cache<_Tp>;
};
template <bool _Present, class _Tp>
using __cache_if = typename __cache_select<_Present, _Tp>::type;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// Bases that give a view's iterator its iterator_category member, or none ("not always present").
struct __no_iterator_category {};
template <class _Tag>
struct __with_iterator_category {
  using iterator_category = _Tag;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Tag is void: no iterator_category member.
template <class _Tag>
using __category_base = std::conditional_t<std::is_void_v<_Tag>, __ycxx::__adl_free::__no_iterator_category,
                                         __ycxx::__adl_free::__with_iterator_category<_Tag>>;

// The iterator_concept most views give their iterator: the strength of the underlying range,
// capped at random access.
template <class _Base>
consteval auto __range_strength() {
  if constexpr (std::ranges::random_access_range<_Base>)
    return std::random_access_iterator_tag{};
  else if constexpr (std::ranges::bidirectional_range<_Base>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (std::ranges::forward_range<_Base>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}
template <class _Base>
using __range_strength_t = decltype(::__ycxx::__detail::__range_strength<_Base>());

// Access to the private members of the views' iterators and sentinels from the hidden friends of
// their sibling classes (befriending a class does not reliably extend to its hidden friends).
// Every such iterator and sentinel declares `friend __ycxx::__detail::__view_access;`.
struct __view_access {
  template <class _Tp>
  static constexpr auto&& current(_Tp&& t) noexcept {
    return static_cast<_Tp&&>(t).__current_;
  }
  template <class _Tp>
  static constexpr auto&& __parent(_Tp&& t) noexcept {
    return static_cast<_Tp&&>(t).__parent_;
  }
  template <class _Tp>
  static constexpr auto&& end(_Tp&& t) noexcept {
    return static_cast<_Tp&&>(t).__end_;
  }
};

}} // namespace __ycxx::__detail
