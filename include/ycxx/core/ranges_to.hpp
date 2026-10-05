// libycxx core: range conversions ([range.utility.conv]: ranges::to and its closure form) and
// ranges::elements_of ([range.elementsof]).
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/cstddef.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class Container>
constexpr bool reservable_container =
    std::ranges::sized_range<Container> && requires(Container& c, std::ranges::range_size_t<Container> n) {
      c.reserve(n);
      { c.capacity() } -> std::same_as<decltype(n)>;
      { c.max_size() } -> std::same_as<decltype(n)>;
    };

template <class Container, class Ref>
constexpr bool container_appendable = requires(Container& c, Ref&& ref) {
  requires(requires { c.emplace_back(static_cast<Ref&&>(ref)); } ||
           requires { c.push_back(static_cast<Ref&&>(ref)); } ||
           requires { c.emplace_hint(c.end(), static_cast<Ref&&>(ref)); } ||
           requires { c.insert(c.end(), static_cast<Ref&&>(ref)); });
};

template <class Container>
struct container_appender {
  Container* c;
  template <class Ref>
  constexpr void operator()(Ref&& ref) const {
    if constexpr (requires(Container& cc) { cc.emplace_back(std::declval<Ref>()); })
      c->emplace_back(static_cast<Ref&&>(ref));
    else if constexpr (requires(Container& cc) { cc.push_back(std::declval<Ref>()); })
      c->push_back(static_cast<Ref&&>(ref));
    else if constexpr (requires(Container& cc) { cc.emplace_hint(cc.end(), std::declval<Ref>()); })
      c->emplace_hint(c->end(), static_cast<Ref&&>(ref));
    else
      c->insert(c->end(), static_cast<Ref&&>(ref));
  }
};
template <class Container>
constexpr auto container_append(Container& c) {
  return container_appender<Container>{__builtin_addressof(c)};
}

// [range.utility.conv.to]/2.1: C is not an input range, or R's elements convert to its values.
template <class C, class R>
concept to_direct = !std::ranges::input_range<C> ||
                    std::convertible_to<std::ranges::range_reference_t<R>, std::ranges::range_value_t<C>>;

// iterator_traits<iterator_t<R>>::iterator_category is valid and models
// derived_from<input_iterator_tag>.
template <class R>
concept to_cpp17_input = requires { typename std::iterator_traits<std::ranges::iterator_t<R>>::iterator_category; } &&
                         std::derived_from<typename std::iterator_traits<std::ranges::iterator_t<R>>::iterator_category,
                                           std::input_iterator_tag>;

// The exposition-only input-iterator of [range.utility.conv.to]/3: only its declarations matter.
template <class R>
struct to_input_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = std::ranges::range_value_t<R>;
  using difference_type = std::ptrdiff_t;
  using pointer = std::add_pointer_t<std::ranges::range_reference_t<R>>;
  using reference = std::ranges::range_reference_t<R>;
  reference operator*() const;
  pointer operator->() const;
  to_input_iterator& operator++();
  to_input_iterator operator++(int);
  bool operator==(const to_input_iterator&) const;
};

template <template <class...> class C, class R, class... Args>
consteval auto to_deduce() {
  if constexpr (requires { C(std::declval<R>(), std::declval<Args>()...); })
    return std::type_identity<decltype(C(std::declval<R>(), std::declval<Args>()...))>{};
  else if constexpr (requires { C(std::from_range, std::declval<R>(), std::declval<Args>()...); })
    return std::type_identity<decltype(C(std::from_range, std::declval<R>(), std::declval<Args>()...))>{};
  else if constexpr (requires {
                       C(std::declval<to_input_iterator<R>>(), std::declval<to_input_iterator<R>>(),
                         std::declval<Args>()...);
                     })
    return std::type_identity<decltype(C(std::declval<to_input_iterator<R>>(), std::declval<to_input_iterator<R>>(),
                                         std::declval<Args>()...))>{};
  else
    return std::type_identity<void>{};
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

template <class C, input_range R, class... Args>
  requires(!view<C>)
constexpr C to(R&& r, Args&&... args) {
  if constexpr (!(is_class_v<C> || is_union_v<C>) || is_const_v<C> || is_volatile_v<C>) {
    static_assert(false, "ranges::to: C must be a cv-unqualified class type");
  } else if constexpr (ycxx::detail::to_direct<C, R>) {
    if constexpr (constructible_from<C, R, Args...>) {
      return C(static_cast<R&&>(r), static_cast<Args&&>(args)...);
    } else if constexpr (constructible_from<C, from_range_t, R, Args...>) {
      return C(from_range, static_cast<R&&>(r), static_cast<Args&&>(args)...);
    } else if constexpr (common_range<R> && ycxx::detail::to_cpp17_input<R> &&
                         constructible_from<C, iterator_t<R>, sentinel_t<R>, Args...>) {
      return C(ranges::begin(r), ranges::end(r), static_cast<Args&&>(args)...);
    } else if constexpr (constructible_from<C, Args...> &&
                         ycxx::detail::container_appendable<C, range_reference_t<R>>) {
      C c(static_cast<Args&&>(args)...);
      if constexpr (approximately_sized_range<R> && ycxx::detail::reservable_container<C>)
        c.reserve(static_cast<range_size_t<C>>(ranges::reserve_hint(r)));
      auto append = ycxx::detail::container_append(c);
      auto last = ranges::end(r);
      for (auto first = ranges::begin(r); first != last; ++first)
        append(*first);
      return c;
    } else {
      static_assert(false, "ranges::to: C cannot be constructed from the range");
    }
  } else if constexpr (input_range<range_reference_t<R>>) {
    return ranges::to<C>(ref_view(r) | views::transform([](auto&& elem) {
                           return ranges::to<range_value_t<C>>(static_cast<decltype(elem)&&>(elem));
                         }),
                         static_cast<Args&&>(args)...);
  } else {
    static_assert(false, "ranges::to: the range's elements do not convert to C's value type");
  }
}

template <template <class...> class C, input_range R, class... Args>
constexpr auto to(R&& r, Args&&... args) {
  using T = typename decltype(ycxx::detail::to_deduce<C, R, Args...>())::type;
  if constexpr (is_void_v<T>)
    static_assert(false, "ranges::to: cannot deduce the template arguments of C");
  else
    return ranges::to<T>(static_cast<R&&>(r), static_cast<Args&&>(args)...);
}

}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class C>
struct to_fn {
  template <class R, class... Args>
    requires requires { std::ranges::to<C>(std::declval<R>(), std::declval<Args>()...); }
  constexpr C operator()(R&& r, Args&&... args) const {
    return std::ranges::to<C>(static_cast<R&&>(r), static_cast<Args&&>(args)...);
  }
};
template <template <class...> class C>
struct to_template_fn {
  template <class R, class... Args>
    requires requires { std::ranges::to<C>(std::declval<R>(), std::declval<Args>()...); }
  constexpr auto operator()(R&& r, Args&&... args) const {
    return std::ranges::to<C>(static_cast<R&&>(r), static_cast<Args&&>(args)...);
  }
};
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

template <class C, class... Args>
  requires(!view<C>)
constexpr auto to(Args&&... args) {
  if constexpr (!(is_class_v<C> || is_union_v<C>) || is_const_v<C> || is_volatile_v<C>)
    static_assert(false, "ranges::to: C must be a cv-unqualified class type");
  else
    return ycxx::adl_free::adaptor_closure<ycxx::detail::to_fn<C>, decay_t<Args>...>(
        ycxx::adl_free::wrapper_init_t{}, ycxx::detail::to_fn<C>{}, static_cast<Args&&>(args)...);
}
template <template <class...> class C, class... Args>
constexpr auto to(Args&&... args) {
  return ycxx::adl_free::adaptor_closure<ycxx::detail::to_template_fn<C>, decay_t<Args>...>(
      ycxx::adl_free::wrapper_init_t{}, ycxx::detail::to_template_fn<C>{}, static_cast<Args&&>(args)...);
}

// [range.elementsof]
template <range R, class Allocator = allocator<byte>>
struct elements_of {
  [[no_unique_address]] R range;
  [[no_unique_address]] Allocator allocator = Allocator();
};
template <class R, class Allocator = allocator<byte>>
elements_of(R&&, Allocator = Allocator()) -> elements_of<R&&, Allocator>;

}} // namespace std::ranges
