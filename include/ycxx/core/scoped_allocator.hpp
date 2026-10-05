// libycxx core: scoped_allocator_adaptor ([allocator.adaptor]).
//
// The adaptor derives from OuterAlloc, as specified. The inner adaptor is a [[no_unique_address]]
// member whose type is an empty placeholder when there are no inner allocators (then
// inner_allocator() is *this), so a scoped_allocator_adaptor of one stateless allocator stays
// empty. construct() is uses-allocator construction with inner_allocator()
// (uses_allocator_construction_args, in tuple.hpp) through the outermost allocator's traits.
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class OuterAlloc, class... InnerAllocs>
class scoped_allocator_adaptor;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct no_inner_allocator {};

template <class... InnerAllocs>
struct scoped_inner {
  using type = std::scoped_allocator_adaptor<InnerAllocs...>;
};
template <>
struct scoped_inner<> {
  using type = no_inner_allocator;
};

// OUTERMOST(x) ([allocator.adaptor.members]/1).
template <class A>
constexpr auto& scoped_outermost(A& a) noexcept {
  if constexpr (requires { a.outer_allocator(); })
    return ::ycxx::detail::scoped_outermost(a.outer_allocator());
  else
    return a;
}

struct scoped_select_tag {};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class OuterAlloc, class... InnerAllocs>
class scoped_allocator_adaptor : public OuterAlloc {
  using OuterTraits = allocator_traits<OuterAlloc>;
  static constexpr bool has_inner = sizeof...(InnerAllocs) != 0;
  using inner_storage = typename ycxx::detail::scoped_inner<InnerAllocs...>::type;

  template <class, class...>
  friend class scoped_allocator_adaptor;

  [[no_unique_address]] inner_storage inner;

  // For select_on_container_copy_construction: the outer allocator and a ready inner adaptor.
  scoped_allocator_adaptor(ycxx::detail::scoped_select_tag, OuterAlloc&& outer, inner_storage&& in) noexcept
      : OuterAlloc(static_cast<OuterAlloc&&>(outer)), inner(static_cast<inner_storage&&>(in)) {}

public:
  using outer_allocator_type = OuterAlloc;
  using inner_allocator_type = conditional_t<has_inner, inner_storage, scoped_allocator_adaptor>;
  using value_type = typename OuterTraits::value_type;
  using size_type = typename OuterTraits::size_type;
  using difference_type = typename OuterTraits::difference_type;
  using pointer = typename OuterTraits::pointer;
  using const_pointer = typename OuterTraits::const_pointer;
  using void_pointer = typename OuterTraits::void_pointer;
  using const_void_pointer = typename OuterTraits::const_void_pointer;
  using propagate_on_container_copy_assignment =
      bool_constant<(OuterTraits::propagate_on_container_copy_assignment::value || ... ||
                     allocator_traits<InnerAllocs>::propagate_on_container_copy_assignment::value)>;
  using propagate_on_container_move_assignment =
      bool_constant<(OuterTraits::propagate_on_container_move_assignment::value || ... ||
                     allocator_traits<InnerAllocs>::propagate_on_container_move_assignment::value)>;
  using propagate_on_container_swap =
      bool_constant<(OuterTraits::propagate_on_container_swap::value || ... ||
                     allocator_traits<InnerAllocs>::propagate_on_container_swap::value)>;
  using is_always_equal =
      bool_constant<(OuterTraits::is_always_equal::value && ... && allocator_traits<InnerAllocs>::is_always_equal::value)>;

  template <class Tp>
  struct rebind {
    using other = scoped_allocator_adaptor<typename OuterTraits::template rebind_alloc<Tp>, InnerAllocs...>;
  };

  // [allocator.adaptor.cnstr]
  scoped_allocator_adaptor() : OuterAlloc(), inner() {}
  template <class OuterA2>
    requires is_constructible_v<OuterAlloc, OuterA2>
  scoped_allocator_adaptor(OuterA2&& outerAlloc, const InnerAllocs&... innerAllocs) noexcept
      : OuterAlloc(static_cast<OuterA2&&>(outerAlloc)), inner(innerAllocs...) {}
  scoped_allocator_adaptor(const scoped_allocator_adaptor& other) noexcept
      : OuterAlloc(other.outer_allocator()), inner(other.inner) {}
  scoped_allocator_adaptor(scoped_allocator_adaptor&& other) noexcept
      : OuterAlloc(static_cast<OuterAlloc&&>(other.outer_allocator())), inner(static_cast<inner_storage&&>(other.inner)) {}
  template <class OuterA2>
    requires is_constructible_v<OuterAlloc, const OuterA2&>
  scoped_allocator_adaptor(const scoped_allocator_adaptor<OuterA2, InnerAllocs...>& other) noexcept
      : OuterAlloc(other.outer_allocator()), inner(other.inner) {}
  template <class OuterA2>
    requires is_constructible_v<OuterAlloc, OuterA2>
  scoped_allocator_adaptor(scoped_allocator_adaptor<OuterA2, InnerAllocs...>&& other) noexcept
      : OuterAlloc(static_cast<OuterA2&&>(other.outer_allocator())), inner(static_cast<inner_storage&&>(other.inner)) {}

  scoped_allocator_adaptor& operator=(const scoped_allocator_adaptor&) = default;
  scoped_allocator_adaptor& operator=(scoped_allocator_adaptor&&) = default;
  ~scoped_allocator_adaptor() = default;

  // [allocator.adaptor.members]
  inner_allocator_type& inner_allocator() noexcept {
    if constexpr (has_inner)
      return inner;
    else
      return *this;
  }
  const inner_allocator_type& inner_allocator() const noexcept {
    if constexpr (has_inner)
      return inner;
    else
      return *this;
  }
  outer_allocator_type& outer_allocator() noexcept { return static_cast<OuterAlloc&>(*this); }
  const outer_allocator_type& outer_allocator() const noexcept { return static_cast<const OuterAlloc&>(*this); }

  [[nodiscard]] pointer allocate(size_type n) { return OuterTraits::allocate(outer_allocator(), n); }
  [[nodiscard]] pointer allocate(size_type n, const_void_pointer hint) {
    return OuterTraits::allocate(outer_allocator(), n, hint);
  }
  void deallocate(pointer p, size_type n) noexcept { OuterTraits::deallocate(outer_allocator(), p, n); }
  size_type max_size() const { return OuterTraits::max_size(outer_allocator()); }

  template <class T, class... Args>
  void construct(T* p, Args&&... args) {
    std::apply(
        [p, this](auto&&... newargs) {
          auto& outermost = ::ycxx::detail::scoped_outermost(*this);
          allocator_traits<remove_reference_t<decltype(outermost)>>::construct(
              outermost, p, static_cast<decltype(newargs)&&>(newargs)...);
        },
        std::uses_allocator_construction_args<T>(inner_allocator(), static_cast<Args&&>(args)...));
  }
  template <class T>
  void destroy(T* p) {
    auto& outermost = ::ycxx::detail::scoped_outermost(*this);
    allocator_traits<remove_reference_t<decltype(outermost)>>::destroy(outermost, p);
  }

  scoped_allocator_adaptor select_on_container_copy_construction() const {
    if constexpr (has_inner)
      return scoped_allocator_adaptor(ycxx::detail::scoped_select_tag{},
                                      OuterTraits::select_on_container_copy_construction(outer_allocator()),
                                      inner.select_on_container_copy_construction());
    else
      return scoped_allocator_adaptor(OuterTraits::select_on_container_copy_construction(outer_allocator()));
  }
};

template <class OuterAlloc, class... InnerAllocs>
scoped_allocator_adaptor(OuterAlloc, InnerAllocs...) -> scoped_allocator_adaptor<OuterAlloc, InnerAllocs...>;

// [scoped.adaptor.operators]
template <class OuterA1, class OuterA2, class... InnerAllocs>
bool operator==(const scoped_allocator_adaptor<OuterA1, InnerAllocs...>& a,
                const scoped_allocator_adaptor<OuterA2, InnerAllocs...>& b) noexcept {
  if constexpr (sizeof...(InnerAllocs) == 0)
    return a.outer_allocator() == b.outer_allocator();
  else
    return a.outer_allocator() == b.outer_allocator() && a.inner_allocator() == b.inner_allocator();
}

} // namespace std
