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

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _OuterAlloc, class... _InnerAllocs>
class scoped_allocator_adaptor;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __no_inner_allocator {};

template <class... _InnerAllocs>
struct __scoped_inner {
  using type = std::scoped_allocator_adaptor<_InnerAllocs...>;
};
template <>
struct __scoped_inner<> {
  using type = __no_inner_allocator;
};

// OUTERMOST(x) ([allocator.adaptor.members]/1).
template <class _Ap>
constexpr auto& __scoped_outermost(_Ap& a) noexcept {
  if constexpr (requires { a.outer_allocator(); })
    return ::__ycxx::__detail::__scoped_outermost(a.outer_allocator());
  else
    return a;
}

struct __scoped_select_tag {};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _OuterAlloc, class... _InnerAllocs>
class scoped_allocator_adaptor : public _OuterAlloc {
  using _OuterTraits = allocator_traits<_OuterAlloc>;
  static constexpr bool __has_inner = sizeof...(_InnerAllocs) != 0;
  using __inner_storage = typename __ycxx::__detail::__scoped_inner<_InnerAllocs...>::type;

  template <class, class...>
  friend class scoped_allocator_adaptor;

  [[no_unique_address]] __inner_storage __inner;

  // For select_on_container_copy_construction: the outer allocator and a ready inner adaptor.
  scoped_allocator_adaptor(__ycxx::__detail::__scoped_select_tag, _OuterAlloc&& outer, __inner_storage&& in) noexcept
      : _OuterAlloc(static_cast<_OuterAlloc&&>(outer)), __inner(static_cast<__inner_storage&&>(in)) {}

public:
  using outer_allocator_type = _OuterAlloc;
  using inner_allocator_type = conditional_t<__has_inner, __inner_storage, scoped_allocator_adaptor>;
  using value_type = typename _OuterTraits::value_type;
  using size_type = typename _OuterTraits::size_type;
  using difference_type = typename _OuterTraits::difference_type;
  using pointer = typename _OuterTraits::pointer;
  using const_pointer = typename _OuterTraits::const_pointer;
  using void_pointer = typename _OuterTraits::void_pointer;
  using const_void_pointer = typename _OuterTraits::const_void_pointer;
  using propagate_on_container_copy_assignment =
      bool_constant<(_OuterTraits::propagate_on_container_copy_assignment::value || ... ||
                     allocator_traits<_InnerAllocs>::propagate_on_container_copy_assignment::value)>;
  using propagate_on_container_move_assignment =
      bool_constant<(_OuterTraits::propagate_on_container_move_assignment::value || ... ||
                     allocator_traits<_InnerAllocs>::propagate_on_container_move_assignment::value)>;
  using propagate_on_container_swap =
      bool_constant<(_OuterTraits::propagate_on_container_swap::value || ... ||
                     allocator_traits<_InnerAllocs>::propagate_on_container_swap::value)>;
  using is_always_equal =
      bool_constant<(_OuterTraits::is_always_equal::value && ... && allocator_traits<_InnerAllocs>::is_always_equal::value)>;

  template <class _Tp_>
  struct rebind {
    using other = scoped_allocator_adaptor<typename _OuterTraits::template rebind_alloc<_Tp_>, _InnerAllocs...>;
  };

  // [allocator.adaptor.cnstr]
  scoped_allocator_adaptor() : _OuterAlloc(), __inner() {}
  template <class _OuterA2>
    requires is_constructible_v<_OuterAlloc, _OuterA2>
  scoped_allocator_adaptor(_OuterA2&& __outerAlloc, const _InnerAllocs&... __innerAllocs) noexcept
      : _OuterAlloc(static_cast<_OuterA2&&>(__outerAlloc)), __inner(__innerAllocs...) {}
  scoped_allocator_adaptor(const scoped_allocator_adaptor& other) noexcept
      : _OuterAlloc(other.outer_allocator()), __inner(other.__inner) {}
  scoped_allocator_adaptor(scoped_allocator_adaptor&& other) noexcept
      : _OuterAlloc(static_cast<_OuterAlloc&&>(other.outer_allocator())), __inner(static_cast<__inner_storage&&>(other.__inner)) {}
  template <class _OuterA2>
    requires is_constructible_v<_OuterAlloc, const _OuterA2&>
  scoped_allocator_adaptor(const scoped_allocator_adaptor<_OuterA2, _InnerAllocs...>& other) noexcept
      : _OuterAlloc(other.outer_allocator()), __inner(other.__inner) {}
  template <class _OuterA2>
    requires is_constructible_v<_OuterAlloc, _OuterA2>
  scoped_allocator_adaptor(scoped_allocator_adaptor<_OuterA2, _InnerAllocs...>&& other) noexcept
      : _OuterAlloc(static_cast<_OuterA2&&>(other.outer_allocator())), __inner(static_cast<__inner_storage&&>(other.__inner)) {}

  scoped_allocator_adaptor& operator=(const scoped_allocator_adaptor&) = default;
  scoped_allocator_adaptor& operator=(scoped_allocator_adaptor&&) = default;
  ~scoped_allocator_adaptor() = default;

  // [allocator.adaptor.members]
  inner_allocator_type& inner_allocator() noexcept {
    if constexpr (__has_inner)
      return __inner;
    else
      return *this;
  }
  const inner_allocator_type& inner_allocator() const noexcept {
    if constexpr (__has_inner)
      return __inner;
    else
      return *this;
  }
  outer_allocator_type& outer_allocator() noexcept { return static_cast<_OuterAlloc&>(*this); }
  const outer_allocator_type& outer_allocator() const noexcept { return static_cast<const _OuterAlloc&>(*this); }

  [[nodiscard]] pointer allocate(size_type n) { return _OuterTraits::allocate(outer_allocator(), n); }
  [[nodiscard]] pointer allocate(size_type n, const_void_pointer __hint) {
    return _OuterTraits::allocate(outer_allocator(), n, __hint);
  }
  void deallocate(pointer p, size_type n) noexcept { _OuterTraits::deallocate(outer_allocator(), p, n); }
  size_type max_size() const { return _OuterTraits::max_size(outer_allocator()); }

  template <class _Tp, class... _Args>
  void construct(_Tp* p, _Args&&... __args) {
    std::apply(
        [p, this](auto&&... __newargs) {
          auto& __outermost = ::__ycxx::__detail::__scoped_outermost(*this);
          allocator_traits<remove_reference_t<decltype(__outermost)>>::construct(
              __outermost, p, static_cast<decltype(__newargs)&&>(__newargs)...);
        },
        std::uses_allocator_construction_args<_Tp>(inner_allocator(), static_cast<_Args&&>(__args)...));
  }
  template <class _Tp>
  void destroy(_Tp* p) {
    auto& __outermost = ::__ycxx::__detail::__scoped_outermost(*this);
    allocator_traits<remove_reference_t<decltype(__outermost)>>::destroy(__outermost, p);
  }

  scoped_allocator_adaptor select_on_container_copy_construction() const {
    if constexpr (__has_inner)
      return scoped_allocator_adaptor(__ycxx::__detail::__scoped_select_tag{},
                                      _OuterTraits::select_on_container_copy_construction(outer_allocator()),
                                      __inner.select_on_container_copy_construction());
    else
      return scoped_allocator_adaptor(_OuterTraits::select_on_container_copy_construction(outer_allocator()));
  }
};

template <class _OuterAlloc, class... _InnerAllocs>
scoped_allocator_adaptor(_OuterAlloc, _InnerAllocs...) -> scoped_allocator_adaptor<_OuterAlloc, _InnerAllocs...>;

// [scoped.adaptor.operators]
template <class _OuterA1, class _OuterA2, class... _InnerAllocs>
bool operator==(const scoped_allocator_adaptor<_OuterA1, _InnerAllocs...>& a,
                const scoped_allocator_adaptor<_OuterA2, _InnerAllocs...>& b) noexcept {
  if constexpr (sizeof...(_InnerAllocs) == 0)
    return a.outer_allocator() == b.outer_allocator();
  else
    return a.outer_allocator() == b.outer_allocator() && a.inner_allocator() == b.inner_allocator();
}

} // namespace std
