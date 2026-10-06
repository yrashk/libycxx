// libycxx core: range conversions ([range.utility.conv]: ranges::to and its closure form) and
// ranges::elements_of ([range.elementsof]).
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/cstddef.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Container>
constexpr bool __reservable_container =
    std::ranges::sized_range<_Container> && requires(_Container& c, std::ranges::range_size_t<_Container> n) {
      c.reserve(n);
      { c.capacity() } -> std::same_as<decltype(n)>;
      { c.max_size() } -> std::same_as<decltype(n)>;
    };

template <class _Container, class _Ref>
constexpr bool __container_appendable = requires(_Container& c, _Ref&& ref) {
  requires(requires { c.emplace_back(static_cast<_Ref&&>(ref)); } ||
           requires { c.push_back(static_cast<_Ref&&>(ref)); } ||
           requires { c.emplace_hint(c.end(), static_cast<_Ref&&>(ref)); } ||
           requires { c.insert(c.end(), static_cast<_Ref&&>(ref)); });
};

template <class _Container>
struct __container_appender {
  _Container* c;
  template <class _Ref>
  constexpr void operator()(_Ref&& ref) const {
    if constexpr (requires(_Container& __cc) { __cc.emplace_back(std::declval<_Ref>()); })
      c->emplace_back(static_cast<_Ref&&>(ref));
    else if constexpr (requires(_Container& __cc) { __cc.push_back(std::declval<_Ref>()); })
      c->push_back(static_cast<_Ref&&>(ref));
    else if constexpr (requires(_Container& __cc) { __cc.emplace_hint(__cc.end(), std::declval<_Ref>()); })
      c->emplace_hint(c->end(), static_cast<_Ref&&>(ref));
    else
      c->insert(c->end(), static_cast<_Ref&&>(ref));
  }
};
template <class _Container>
constexpr auto __container_append(_Container& c) {
  return __container_appender<_Container>{__builtin_addressof(c)};
}

// [range.utility.conv.to]/2.1: C is not an input range, or R's elements convert to its values.
template <class _Cp, class _Rp>
concept __to_direct = !std::ranges::input_range<_Cp> ||
                    std::convertible_to<std::ranges::range_reference_t<_Rp>, std::ranges::range_value_t<_Cp>>;

// iterator_traits<iterator_t<R>>::iterator_category is valid and models
// derived_from<input_iterator_tag>.
template <class _Rp>
concept __to_cpp17_input = requires { typename std::iterator_traits<std::ranges::iterator_t<_Rp>>::iterator_category; } &&
                         std::derived_from<typename std::iterator_traits<std::ranges::iterator_t<_Rp>>::iterator_category,
                                           std::input_iterator_tag>;

// The exposition-only input-iterator of [range.utility.conv.to]/3: only its declarations matter.
template <class _Rp>
struct __to_input_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = std::ranges::range_value_t<_Rp>;
  using difference_type = std::ptrdiff_t;
  using pointer = std::add_pointer_t<std::ranges::range_reference_t<_Rp>>;
  using reference = std::ranges::range_reference_t<_Rp>;
  reference operator*() const;
  pointer operator->() const;
  __to_input_iterator& operator++();
  __to_input_iterator operator++(int);
  bool operator==(const __to_input_iterator&) const;
};

template <template <class...> class _Cp, class _Rp, class... _Args>
consteval auto __to_deduce() {
  if constexpr (requires { _Cp(std::declval<_Rp>(), std::declval<_Args>()...); })
    return std::type_identity<decltype(_Cp(std::declval<_Rp>(), std::declval<_Args>()...))>{};
  else if constexpr (requires { _Cp(std::from_range, std::declval<_Rp>(), std::declval<_Args>()...); })
    return std::type_identity<decltype(_Cp(std::from_range, std::declval<_Rp>(), std::declval<_Args>()...))>{};
  else if constexpr (requires {
                       _Cp(std::declval<__to_input_iterator<_Rp>>(), std::declval<__to_input_iterator<_Rp>>(),
                         std::declval<_Args>()...);
                     })
    return std::type_identity<decltype(_Cp(std::declval<__to_input_iterator<_Rp>>(), std::declval<__to_input_iterator<_Rp>>(),
                                         std::declval<_Args>()...))>{};
  else
    return std::type_identity<void>{};
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <class _Cp, input_range _Rp, class... _Args>
  requires(!view<_Cp>)
constexpr _Cp to(_Rp&& r, _Args&&... __args) {
  if constexpr (!(is_class_v<_Cp> || is_union_v<_Cp>) || is_const_v<_Cp> || is_volatile_v<_Cp>) {
    static_assert(false, "ranges::to: C must be a cv-unqualified class type");
  } else if constexpr (__ycxx::__detail::__to_direct<_Cp, _Rp>) {
    if constexpr (constructible_from<_Cp, _Rp, _Args...>) {
      return _Cp(static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
    } else if constexpr (constructible_from<_Cp, from_range_t, _Rp, _Args...>) {
      return _Cp(from_range, static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
    } else if constexpr (common_range<_Rp> && __ycxx::__detail::__to_cpp17_input<_Rp> &&
                         constructible_from<_Cp, iterator_t<_Rp>, sentinel_t<_Rp>, _Args...>) {
      return _Cp(ranges::begin(r), ranges::end(r), static_cast<_Args&&>(__args)...);
    } else if constexpr (constructible_from<_Cp, _Args...> &&
                         __ycxx::__detail::__container_appendable<_Cp, range_reference_t<_Rp>>) {
      _Cp c(static_cast<_Args&&>(__args)...);
      if constexpr (approximately_sized_range<_Rp> && __ycxx::__detail::__reservable_container<_Cp>)
        c.reserve(static_cast<range_size_t<_Cp>>(ranges::reserve_hint(r)));
      auto append = __ycxx::__detail::__container_append(c);
      auto last = ranges::end(r);
      for (auto first = ranges::begin(r); first != last; ++first)
        append(*first);
      return c;
    } else {
      static_assert(false, "ranges::to: C cannot be constructed from the range");
    }
  } else if constexpr (input_range<range_reference_t<_Rp>>) {
    return ranges::to<_Cp>(ref_view(r) | views::transform([](auto&& __elem) {
                           return ranges::to<range_value_t<_Cp>>(static_cast<decltype(__elem)&&>(__elem));
                         }),
                         static_cast<_Args&&>(__args)...);
  } else {
    static_assert(false, "ranges::to: the range's elements do not convert to C's value type");
  }
}

template <template <class...> class _Cp, input_range _Rp, class... _Args>
constexpr auto to(_Rp&& r, _Args&&... __args) {
  using _Tp = typename decltype(__ycxx::__detail::__to_deduce<_Cp, _Rp, _Args...>())::type;
  if constexpr (is_void_v<_Tp>)
    static_assert(false, "ranges::to: cannot deduce the template arguments of C");
  else
    return ranges::to<_Tp>(static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
}

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Cp>
struct __to_fn {
  template <class _Rp, class... _Args>
    requires requires { std::ranges::to<_Cp>(std::declval<_Rp>(), std::declval<_Args>()...); }
  constexpr _Cp operator()(_Rp&& r, _Args&&... __args) const {
    return std::ranges::to<_Cp>(static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
  }
};
template <template <class...> class _Cp>
struct __to_template_fn {
  template <class _Rp, class... _Args>
    requires requires { std::ranges::to<_Cp>(std::declval<_Rp>(), std::declval<_Args>()...); }
  constexpr auto operator()(_Rp&& r, _Args&&... __args) const {
    return std::ranges::to<_Cp>(static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
  }
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <class _Cp, class... _Args>
  requires(!view<_Cp>)
constexpr auto to(_Args&&... __args) {
  if constexpr (!(is_class_v<_Cp> || is_union_v<_Cp>) || is_const_v<_Cp> || is_volatile_v<_Cp>)
    static_assert(false, "ranges::to: C must be a cv-unqualified class type");
  else
    return __ycxx::__adl_free::__adaptor_closure<__ycxx::__detail::__to_fn<_Cp>, decay_t<_Args>...>(
        __ycxx::__adl_free::__wrapper_init_t{}, __ycxx::__detail::__to_fn<_Cp>{}, static_cast<_Args&&>(__args)...);
}
template <template <class...> class _Cp, class... _Args>
constexpr auto to(_Args&&... __args) {
  return __ycxx::__adl_free::__adaptor_closure<__ycxx::__detail::__to_template_fn<_Cp>, decay_t<_Args>...>(
      __ycxx::__adl_free::__wrapper_init_t{}, __ycxx::__detail::__to_template_fn<_Cp>{}, static_cast<_Args&&>(__args)...);
}

// [range.elementsof]
template <range _Rp, class _Allocator = allocator<byte>>
struct elements_of {
  [[no_unique_address]] _Rp range;
  [[no_unique_address]] _Allocator allocator = _Allocator();
};
template <class _Rp, class _Allocator = allocator<byte>>
elements_of(_Rp&&, _Allocator = _Allocator()) -> elements_of<_Rp&&, _Allocator>;

}} // namespace std::ranges
