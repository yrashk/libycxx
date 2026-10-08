// libycxx core: vector ([vector]) for element types other than bool, its non-member functions
// ([vector.syn], [vector.erasure]) and pmr::vector. vector<bool> is in vector_bool.hpp.
//
// Representation: three T* pointers {first_, last_, cap_} into one allocation obtained with
// allocate_at_least (all null while nothing is allocated), plus the allocator. Elements are
// constructed and destroyed only through allocator_traits ([container.alloc.reqmts]/2). At run
// time a trivially copyable T whose allocator does not customize construct/destroy is copied
// and relocated with memcpy.
//
// Exception safety ([vector.modifiers]/2, [vector.capacity]): a reallocation builds the new
// elements in the new storage first (so arguments that refer to old elements stay valid), then
// relocates the old elements by move_if_noexcept semantics; until the new storage is adopted,
// guards undo everything, so the old elements and storage are untouched on failure. Inserting
// several elements in the middle without reallocation constructs them at the end and rotates
// them into place with move assignments (the temporary is constructed through the allocator).
#pragma once

#include <initializer_list>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Allocator = allocator<_Tp>>
class vector;

template <class _Tp, class _Allocator>
class vector {
  static_assert(is_same_v<typename _Allocator::value_type, _Tp>,
                "std::vector: Allocator::value_type must be T ([container.alloc.reqmts]/5)");

  using __alloc_traits = allocator_traits<_Allocator>;

public:
  // ---- types ----
  using value_type = _Tp;
  using allocator_type = _Allocator;
  using pointer = typename __alloc_traits::pointer;
  using const_pointer = typename __alloc_traits::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename __alloc_traits::size_type;
  using difference_type = typename __alloc_traits::difference_type;
  using iterator = __ycxx::__adl_free::__contiguous_iter<_Tp, vector, difference_type>;
  using const_iterator = __ycxx::__adl_free::__contiguous_iter<const _Tp, vector, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr bool __pocca = __alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool __pocma = __alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool __pocs = __alloc_traits::propagate_on_container_swap::value;
  static constexpr bool __always_equal = __alloc_traits::is_always_equal::value;

  _Tp* __first_ = nullptr;
  _Tp* __last_ = nullptr;
  _Tp* __cap_ = nullptr;
  [[no_unique_address]] _Allocator __alloc_;

  // ---- element policies (functions, so that T need only be complete when they are used) ----
  // Elements may be copied as bytes at run time.
  static consteval bool __y_bitwise() {
    return is_trivially_copyable_v<_Tp> && !requires(_Allocator& a, _Tp* p) { a.destroy(p); } &&
           !requires(_Allocator& a, _Tp* p, const _Tp& __x) { a.construct(p, __x); } &&
           !requires(_Allocator& a, _Tp* p, _Tp&& __x) { a.construct(p, static_cast<_Tp&&>(__x)); };
  }
  // Relocation moves unless the move may throw and a copy is possible (move_if_noexcept).
  static consteval bool __relocate_by_move() { return is_nothrow_move_constructible_v<_Tp> || !is_copy_constructible_v<_Tp>; }
  // [first, last) can be memcpy'd into the storage: besides T being trivially copyable, the
  // constructor and assignment that *i selects must be trivial (a template taking T& can be
  // chosen over the trivial copy operations; [sequence.reqmts] constructs from *i).
  template <class _It, class _Sent>
  static consteval bool __bitwise_source() {
    if constexpr (is_same_v<_It, _Sent> && __ycxx::__detail::__is_plain_contiguous<_It>)
      return __y_bitwise() && is_same_v<remove_cvref_t<iter_reference_t<_It>>, _Tp> &&
             is_lvalue_reference_v<iter_reference_t<_It>> && !is_volatile_v<remove_reference_t<iter_reference_t<_It>>> &&
             is_trivially_constructible_v<_Tp, iter_reference_t<_It>> && is_trivially_assignable_v<_Tp&, iter_reference_t<_It>>;
    else
      return false;
  }

  // ---- storage ----
  struct block {
    _Tp* p;
    size_type __cap;
  };
  static constexpr pointer __to_pointer(_Tp* p) noexcept {
    if constexpr (is_same_v<pointer, _Tp*>)
      return p;
    else
      return pointer_traits<pointer>::pointer_to(*p);
  }
  constexpr block __allocate_block(size_type n) {
    auto r = __alloc_traits::allocate_at_least(__alloc_, n);
    return {std::to_address(r.ptr), static_cast<size_type>(r.count)};
  }
  // Returns a block to the allocator on scope exit unless released (p == nullptr).
  struct __block_guard {
    _Allocator& a;
    block b;
    constexpr ~__block_guard() {
      if (b.p)
        __alloc_traits::deallocate(a, vector::__to_pointer(b.p), b.__cap);
    }
    constexpr block release() noexcept {
      block r = b;
      b.p = nullptr;
      return r;
    }
  };
  // Destroys [first, last) through the allocator on scope exit; released by first = last.
  struct __destroy_guard {
    _Allocator& a;
    _Tp* first;
    _Tp* last;
    constexpr ~__destroy_guard() {
      for (; first != last; ++first)
        __alloc_traits::destroy(a, first);
    }
  };

  constexpr void __destroy_range(_Tp* __f, _Tp* __l) noexcept {
    for (; __f != __l; ++__f)
      __alloc_traits::destroy(__alloc_, __f);
  }
  // Destroys the elements and frees the storage; *this then owns nothing.
  constexpr void __release_storage() noexcept {
    if (__first_) {
      __destroy_range(__first_, __last_);
      __alloc_traits::deallocate(__alloc_, __to_pointer(__first_), static_cast<size_type>(__cap_ - __first_));
      __first_ = __last_ = __cap_ = nullptr;
    }
  }
  constexpr void __adopt(block b, _Tp* __l) noexcept {
    __first_ = b.p;
    __last_ = __l;
    __cap_ = b.p + b.__cap;
  }
  constexpr void take(vector& __o) noexcept {
    __first_ = __o.__first_;
    __last_ = __o.__last_;
    __cap_ = __o.__cap_;
    __o.__first_ = __o.__last_ = __o.__cap_ = nullptr;
  }
  constexpr size_type __spare() const noexcept { return static_cast<size_type>(__cap_ - __last_); }
  constexpr _Tp* __mut(const_iterator p) const noexcept { return const_cast<_Tp*>(p.base()); }

  constexpr void __check_size(size_type n) const {
    if (n > max_size())
      __ycxx::__detail::__throw_length_error("std::vector: size exceeds max_size()");
  }
  // Capacity for size() + extra elements (geometric growth).
  constexpr size_type __grow_to(size_type __extra) const {
    const size_type ms = max_size();
    const size_type __sz = size();
    if (__extra > ms - __sz)
      __ycxx::__detail::__throw_length_error("std::vector: size exceeds max_size()");
    const size_type c = capacity();
    const size_type __nc = c <= ms / 2 ? 2 * c : ms;
    return __nc < __sz + __extra ? __sz + __extra : __nc;
  }

  // ---- constructing elements in raw storage; each either completes or constructs nothing ----
  // n elements at dst from [first, last) (or from first, n times, when Sent is unreachable).
  template <class _It, class _Sent>
  constexpr _Tp* __construct_from(_Tp* __dst, _It first, _Sent last, size_type n) {
    if constexpr (__bitwise_source<_It, _Sent>()) {
      if !consteval {
        if (n != 0)
          __builtin_memcpy(static_cast<void*>(__dst), static_cast<const void*>(__ycxx::__detail::__plain_address(first)),
                           n * sizeof(_Tp));
        return __dst + n;
      }
    }
    (void)last;
    __destroy_guard __g{__alloc_, __dst, __dst};
    for (size_type i = 0; i != n; ++i) {
      __alloc_traits::construct(__alloc_, __g.last, *first);
      ++__g.last;
      ++first;
    }
    __g.first = __g.last;
    return __g.last;
  }
  // n elements at dst, each constructed from args (none: value-initialized; one: a copy).
  template <class... _Args>
  constexpr _Tp* __construct_n(_Tp* __dst, size_type n, const _Args&... __args) {
    __destroy_guard __g{__alloc_, __dst, __dst};
    for (size_type i = 0; i != n; ++i) {
      __alloc_traits::construct(__alloc_, __g.last, __args...);
      ++__g.last;
    }
    __g.first = __g.last;
    return __g.last;
  }
  // Relocates [f, l) into dst by move_if_noexcept (or memcpy); the sources stay to be destroyed.
  constexpr _Tp* __relocate(_Tp* __f, _Tp* __l, _Tp* __dst) {
    if constexpr (__y_bitwise()) {
      if !consteval {
        const size_type n = static_cast<size_type>(__l - __f);
        if (n != 0)
          __builtin_memcpy(static_cast<void*>(__dst), static_cast<const void*>(__f), n * sizeof(_Tp));
        return __dst + n;
      }
    }
    __destroy_guard __g{__alloc_, __dst, __dst};
    for (; __f != __l; ++__f) {
      if constexpr (__relocate_by_move())
        __alloc_traits::construct(__alloc_, __g.last, static_cast<_Tp&&>(*__f));
      else
        __alloc_traits::construct(__alloc_, __g.last, static_cast<const _Tp&>(*__f));
      ++__g.last;
    }
    __g.first = __g.last;
    return __g.last;
  }

  // Moves the elements to new storage of capacity >= new_cap, where make(p) constructs n new
  // elements at p (index off) first. Strong guarantee unless a throwing move of a
  // non-copyable T is used.
  template <class _Make>
  constexpr void __realloc_insert(size_type __off, size_type n, size_type __new_cap, _Make&& __make) {
    __block_guard __bg{__alloc_, __allocate_block(__new_cap)};
    _Tp* const __np = __bg.b.p;
    static_cast<_Make&&>(__make)(__np + __off);
    __destroy_guard __g{__alloc_, __np + __off, __np + __off + n};
    __relocate(__first_, __first_ + __off, __np);
    __g.first = __np;
    _Tp* const __nl = __relocate(__first_ + __off, __last_, __np + __off + n);
    __g.first = __g.last;
    const block b = __bg.release();
    __release_storage();
    __adopt(b, __nl);
  }

  // Fresh storage for exactly the n elements of [first, last); *this owns nothing yet.
  template <class _It, class _Sent>
  constexpr void __init_counted(_It first, _Sent last, size_type n) {
    if (n == 0)
      return;
    __check_size(n);
    __block_guard __bg{__alloc_, __allocate_block(n)};
    _Tp* const e = __construct_from(__bg.b.p, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n);
    __adopt(__bg.release(), e);
  }
  template <class... _Args>
  constexpr void __init_n(size_type n, const _Args&... __args) {
    if (n == 0)
      return;
    __check_size(n);
    __block_guard __bg{__alloc_, __allocate_block(n)};
    _Tp* const e = __construct_n(__bg.b.p, n, __args...);
    __adopt(__bg.release(), e);
  }
  // Appends the elements of a single-pass sequence; on an exception the appended elements are
  // removed again.
  template <bool _MayOverlap = false, class _It, class _Sent>
  constexpr void __append_input(_It first, _Sent last) {
    struct __rollback {
      vector& __v;
      size_type n;
      constexpr ~__rollback() {
        if (n != size_type(-1)) {
          __v.__destroy_range(__v.__first_ + n, __v.__last_);
          __v.__last_ = __v.__first_ + n;
        }
      }
    } __g{*this, size()};
    for (; first != last; ++first) {
      if (_MayOverlap && __last_ == __cap_) {
        // Growing frees the storage the rest of the range may refer to: append_range, unlike
        // insert_range, has no precondition that rg does not overlap *this ([sequence.reqmts]).
        // Collect the rest first, then move it in.
        vector __rest(__alloc_);
        for (; first != last; ++first)
          __rest.emplace_back(*first);
        reserve(__grow_to(__rest.size()));
        for (_Tp& __x : __rest)
          emplace_back(static_cast<_Tp&&>(__x));
        break;
      }
      emplace_back(*first);
    }
    __g.n = size_type(-1);
  }

  // [first, last) holds n elements: replaces the contents with them.
  template <class _It, class _Sent>
  constexpr void __assign_counted(_It first, _Sent last, size_type n) {
    if (n > capacity()) {
      __check_size(n);
      __block_guard __bg{__alloc_, __allocate_block(n)};
      _Tp* const e = __construct_from(__bg.b.p, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n);
      const block b = __bg.release();
      __release_storage();
      __adopt(b, e);
      return;
    }
    const size_type __sz = size();
    _Tp* p = __first_;
    const size_type common = n < __sz ? n : __sz;
    for (size_type i = 0; i != common; ++i) {
      *p = *first;
      ++p;
      ++first;
    }
    if (n <= __sz) {
      __destroy_range(p, __last_);
      __last_ = p;
    } else {
      __last_ = __construct_from(__last_, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n - __sz);
    }
  }
  template <class _It, class _Sent>
  constexpr void __assign_input(_It first, _Sent last) {
    _Tp* p = __first_;
    for (; p != __last_ && first != last; ++first) {
      *p = *first;
      ++p;
    }
    if (p != __last_) {
      __destroy_range(p, __last_);
      __last_ = p;
    } else {
      __append_input(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
    }
  }

  // Inserts the n elements of [first, last) at index off.
  template <class _It, class _Sent>
  constexpr iterator __insert_counted(size_type __off, _It first, _Sent last, size_type n) {
    if (n != 0) {
      if (n <= __spare()) {
        _Tp* const __old_last = __last_;
        __last_ = __construct_from(__last_, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n);
        __ycxx::__detail::__rotate_elements<__ycxx::__detail::__alloc_temp<_Tp, _Allocator>>(__first_ + __off, __old_last, __last_, __alloc_);
      } else {
        __realloc_insert(__off, n, __grow_to(n),
                       [&](_Tp* d) { __construct_from(d, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n); });
      }
    }
    return begin() + static_cast<difference_type>(__off);
  }
  // insert_counted at the end: nothing moves, so T need not be move-assignable (append_range).
  template <class _It, class _Sent>
  constexpr void __append_counted(_It first, _Sent last, size_type n) {
    if (n <= __spare())
      __last_ = __construct_from(__last_, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n);
    else
      __realloc_insert(size(), n, __grow_to(n),
                     [&](_Tp* d) { __construct_from(d, static_cast<_It&&>(first), static_cast<_Sent&&>(last), n); });
  }
  template <class _It, class _Sent>
  constexpr iterator __insert_input(size_type __off, _It first, _Sent last) {
    const size_type __old_size = size();
    __append_input(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
    __ycxx::__detail::__rotate_elements<__ycxx::__detail::__alloc_temp<_Tp, _Allocator>>(__first_ + __off, __first_ + __old_size, __last_,
                                                                          __alloc_);
    return begin() + static_cast<difference_type>(__off);
  }
  // Element moves by assignment are memmove for trivially copyable elements (outside constant
  // evaluation): the loops they replace stay for constant evaluation and other types.
  static constexpr bool __memmove_assign = is_trivially_copyable_v<_Tp> && is_trivially_move_assignable_v<_Tp>;

  // Inserts at index off < size() with spare capacity, from a value that is not an element.
  constexpr void __shift_in(size_type __off, _Tp&& __x) {
    _Tp* const p = __first_ + __off;
    __alloc_traits::construct(__alloc_, __last_, static_cast<_Tp&&>(__last_[-1]));
    _Tp* const __old_last = __last_;
    ++__last_;
    if constexpr (__memmove_assign) {
      if !consteval {
        __builtin_memmove(static_cast<void*>(p + 1), static_cast<const void*>(p),
                          static_cast<size_t>(__old_last - 1 - p) * sizeof(_Tp));
        *p = static_cast<_Tp&&>(__x);
        return;
      }
    }
    for (_Tp* d = __old_last - 1; d != p; --d)
      *d = static_cast<_Tp&&>(d[-1]);
    *p = static_cast<_Tp&&>(__x);
  }

  template <class _Rp>
  static constexpr bool __counted_range = ranges::forward_range<_Rp> || ranges::sized_range<_Rp>;
  // The constructors that do work delegate to this one, so that the destructor cleans up if
  // they throw; it is not noexcept, so a (non-conforming) throwing allocator copy propagates.
  struct __with_alloc {
    explicit __with_alloc() = default;
  };
  constexpr vector(__with_alloc, const _Allocator& a) : __alloc_(__ycxx::__detail::__alloc_copy(a)) {}

public:
  // ---- [vector.cons] ----
  constexpr vector() noexcept(is_nothrow_default_constructible_v<_Allocator>) : vector(_Allocator()) {}
  constexpr explicit vector(const _Allocator& a) noexcept : __alloc_(__ycxx::__detail::__alloc_copy(a)) {}
  constexpr explicit vector(size_type n, const _Allocator& a = _Allocator()) : vector(__with_alloc{}, a) { __init_n(n); }
  constexpr vector(__ycxx::__detail::__alloc_size_t<_Allocator> n, const _Tp& value, const _Allocator& a = _Allocator())
      : vector(__with_alloc{}, a) {
    __init_n(n, value);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr vector(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator())
      : vector(__with_alloc{}, a) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __init_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), static_cast<size_type>(n));
    } else {
      __append_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr vector(_Tag, _Rp&& __rg, const _Allocator& a = _Allocator()) : vector(__with_alloc{}, a) {
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __init_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<_Rp>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(__rg));
        reserve(h < max_size() ? h : max_size());
      }
      __append_input(ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr vector(const vector& __x)
      : vector(__with_alloc{}, __alloc_traits::select_on_container_copy_construction(__x.__alloc_)) {
    __init_counted(static_cast<const _Tp*>(__x.__first_), static_cast<const _Tp*>(__x.__last_), __x.size());
  }
  constexpr vector(vector&& __x) noexcept : __alloc_(static_cast<_Allocator&&>(__x.__alloc_)) { take(__x); }
  constexpr vector(const vector& __x, const type_identity_t<_Allocator>& a) : vector(__with_alloc{}, a) {
    __init_counted(static_cast<const _Tp*>(__x.__first_), static_cast<const _Tp*>(__x.__last_), __x.size());
  }
  constexpr vector(vector&& __x, const type_identity_t<_Allocator>& a) noexcept(__always_equal) : vector(__with_alloc{}, a) {
    if (__always_equal || __alloc_ == __x.__alloc_)
      take(__x);
    else
      __init_counted(std::move_iterator<_Tp*>(__x.__first_), std::move_iterator<_Tp*>(__x.__last_), __x.size());
  }
  // Two overloads instead of a default argument, not delegating (init_counted leaves *this
  // owning nothing if it throws): GCC 16 crashes (ICE in cxx_eval_indirect_ref) on nested braced
  // initializers (vector<vector<vector<int>>>{{{1}}}) when the allocator is copied from the
  // default argument. Equivalent: the default argument is a value-initialized Allocator.
  constexpr vector(initializer_list<_Tp> il) : __alloc_() { __init_counted(il.begin(), il.end(), il.size()); }
  constexpr vector(initializer_list<_Tp> il, const _Allocator& a) : __alloc_(__ycxx::__detail::__alloc_copy(a)) {
    __init_counted(il.begin(), il.end(), il.size());
  }

  constexpr ~vector() { __release_storage(); }

  constexpr vector& operator=(const vector& __x) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocca) {
      // Storage from the old allocator must go back to it before the allocator is replaced.
      if (!__always_equal && __alloc_ != __x.__alloc_)
        __release_storage();
      __alloc_ = __x.__alloc_;
    }
    __assign_counted(static_cast<const _Tp*>(__x.__first_), static_cast<const _Tp*>(__x.__last_), __x.size());
    return *this;
  }
  constexpr vector& operator=(vector&& __x) noexcept(__pocma || __always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocma || __always_equal) {
      __release_storage();
      if constexpr (__pocma)
        __alloc_ = static_cast<_Allocator&&>(__x.__alloc_);
      take(__x);
    } else {
      if (__alloc_ == __x.__alloc_) {
        __release_storage();
        take(__x);
      } else {
        __assign_counted(std::move_iterator<_Tp*>(__x.__first_), std::move_iterator<_Tp*>(__x.__last_), __x.size());
      }
    }
    return *this;
  }
  constexpr vector& operator=(initializer_list<_Tp> il) {
    __assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void assign(_InputIterator first, _InputIterator last) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __assign_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      __assign_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void assign_range(_Rp&& __rg) {
    static_assert(assignable_from<_Tp&, ranges::range_reference_t<_Rp>>,
                  "std::vector::assign_range: T& must be assignable from the range's reference type");
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __assign_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      __assign_input(ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr void assign(size_type n, const _Tp& __u) {
    if (n > capacity()) {
      // u may be an element: build the new contents before releasing the old ones.
      __check_size(n);
      __block_guard __bg{__alloc_, __allocate_block(n)};
      _Tp* const e = __construct_n(__bg.b.p, n, __u);
      const block b = __bg.release();
      __release_storage();
      __adopt(b, e);
      return;
    }
    const size_type __sz = size();
    const size_type common = n < __sz ? n : __sz;
    for (size_type i = 0; i != common; ++i)
      __first_[i] = __u;
    if (n <= __sz) {
      __destroy_range(__first_ + n, __last_);
      __last_ = __first_ + n;
    } else {
      __last_ = __construct_n(__last_, n - __sz, __u);
    }
  }
  constexpr void assign(initializer_list<_Tp> il) { __assign_counted(il.begin(), il.end(), il.size()); }
  constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(__first_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__first_); }
  constexpr iterator end() noexcept { return iterator(__last_); }
  constexpr const_iterator end() const noexcept { return const_iterator(__last_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [vector.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __first_ == __last_; }
  constexpr size_type size() const noexcept { return static_cast<size_type>(__last_ - __first_); }
  constexpr size_type max_size() const noexcept {
    // No object is larger than PTRDIFF_MAX bytes.
    const size_type __by_alloc = __alloc_traits::max_size(__alloc_);
    const auto __diff_max =
        static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max()) / sizeof(_Tp);
    return __diff_max < __by_alloc ? static_cast<size_type>(__diff_max) : __by_alloc;
  }
  constexpr size_type capacity() const noexcept { return static_cast<size_type>(__cap_ - __first_); }
  constexpr void resize(size_type __sz) {
    const size_type cur = size();
    if (__sz <= cur) {
      __destroy_range(__first_ + __sz, __last_);
      __last_ = __first_ + __sz;
    } else if (__sz - cur <= __spare()) {
      __last_ = __construct_n(__last_, __sz - cur);
    } else {
      const size_type n = __sz - cur;
      __realloc_insert(cur, n, __grow_to(n), [&](_Tp* d) { __construct_n(d, n); });
    }
  }
  constexpr void resize(size_type __sz, const _Tp& c) {
    const size_type cur = size();
    if (__sz <= cur) {
      __destroy_range(__first_ + __sz, __last_);
      __last_ = __first_ + __sz;
    } else if (__sz - cur <= __spare()) {
      __last_ = __construct_n(__last_, __sz - cur, c);
    } else {
      // c may be an element: the copies are made before the old elements move.
      const size_type n = __sz - cur;
      __realloc_insert(cur, n, __grow_to(n), [&](_Tp* d) { __construct_n(d, n, c); });
    }
  }
  constexpr void reserve(size_type n) {
    if (n > max_size())
      __ycxx::__detail::__throw_length_error("std::vector::reserve: argument exceeds max_size()");
    if (n > capacity())
      __realloc_insert(size(), 0, n, [](_Tp*) {});
  }
  constexpr void shrink_to_fit() {
    if (__last_ == __cap_)
      return;
    if (__first_ == __last_) {
      __release_storage();
      return;
    }
    // A non-binding request: if the allocation fails, nothing changes and nothing is thrown.
    block b{nullptr, 0};
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        b = __allocate_block(size());
      } catch (...) {
        return;
      }
    } else {
      b = __allocate_block(size());
    }
    __block_guard __bg{__alloc_, b};
    if (b.__cap >= capacity())
      return; // the allocator gave nothing back
    _Tp* const e = __relocate(__first_, __last_, b.p);
    __bg.release();
    __release_storage();
    __adopt(b, e);
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    __ycxx::__detail::__precondition(n < size(), "std::vector::operator[]: index out of range");
    return __first_[n];
  }
  constexpr const_reference operator[](size_type n) const {
    __ycxx::__detail::__precondition(n < size(), "std::vector::operator[]: index out of range");
    return __first_[n];
  }
  constexpr reference at(size_type n) {
    if (n >= size())
      __ycxx::__detail::__throw_out_of_range("std::vector::at: index out of range");
    return __first_[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size())
      __ycxx::__detail::__throw_out_of_range("std::vector::at: index out of range");
    return __first_[n];
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(__first_ != __last_, "std::vector::front: empty vector");
    return *__first_;
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__first_ != __last_, "std::vector::front: empty vector");
    return *__first_;
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(__first_ != __last_, "std::vector::back: empty vector");
    return __last_[-1];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__first_ != __last_, "std::vector::back: empty vector");
    return __last_[-1];
  }

  // ---- [vector.data] ----
  constexpr _Tp* data() noexcept { return __first_; }
  constexpr const _Tp* data() const noexcept { return __first_; }

  // ---- [vector.modifiers] ----
  template <class... _Args>
  [[__gnu__::__always_inline__]] constexpr reference emplace_back(_Args&&... __args) {
    if (__last_ != __cap_) {
      __alloc_traits::construct(__alloc_, __last_, static_cast<_Args&&>(__args)...);
      ++__last_;
    } else {
      __realloc_insert(size(), 1, __grow_to(1),
                     [&](_Tp* d) { __alloc_traits::construct(__alloc_, d, static_cast<_Args&&>(__args)...); });
    }
    return __last_[-1];
  }
  constexpr void push_back(const _Tp& __x) { emplace_back(__x); }
  constexpr void push_back(_Tp&& __x) { emplace_back(static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void append_range(_Rp&& __rg) {
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __append_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<_Rp>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(__rg));
        if (h > __spare())
          reserve(size() + (h < max_size() - size() ? h : max_size() - size()));
      }
      __append_input<true>(ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr void pop_back() {
    __ycxx::__detail::__precondition(__first_ != __last_, "std::vector::pop_back: empty vector");
    --__last_;
    __alloc_traits::destroy(__alloc_, __last_);
  }

  // emplace's single argument (a non-const rvalue of type T) is not one of the elements (pointer
  // comparison through the integer values, which orders unrelated objects too; during constant
  // evaluation, where that is not available, always false).
  template <class... _Args>
  constexpr bool __emplace_moves_directly(const remove_reference_t<_Args>&... __args) const noexcept {
    if consteval {
      return false;
    } else {
      const auto p = reinterpret_cast<__UINTPTR_TYPE__>(__builtin_addressof(__args...[0]));
      return p < reinterpret_cast<__UINTPTR_TYPE__>(__first_) || p >= reinterpret_cast<__UINTPTR_TYPE__>(__last_);
    }
  }
  template <class... _Args>
  constexpr iterator emplace(const_iterator position, _Args&&... __args) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    if (__last_ == __cap_) {
      __realloc_insert(__off, 1, __grow_to(1),
                     [&](_Tp* d) { __alloc_traits::construct(__alloc_, d, static_cast<_Args&&>(__args)...); });
    } else if (__first_ + __off == __last_) {
      __alloc_traits::construct(__alloc_, __last_, static_cast<_Args&&>(__args)...);
      ++__last_;
    } else {
      if constexpr (sizeof...(_Args) == 1 && (is_same_v<_Args, _Tp> && ...)) {
        // A single non-const rvalue of type T that is not an element: moved in directly, without
        // a temporary (as insert(position, T&&)). [sequence.reqmts] emplace, Note 1: the
        // arguments may refer to elements, so an element (moved from with std::move) takes the
        // temporary below.
        if (__emplace_moves_directly<_Args...>(__args...)) {
          __shift_in(__off, static_cast<_Args&&>(__args)...);
          return begin() + static_cast<difference_type>(__off);
        }
      }
      // The arguments may refer to elements that are about to move.
      __ycxx::__detail::__alloc_temp<_Tp, _Allocator> __tmp(__alloc_, static_cast<_Args&&>(__args)...);
      __shift_in(__off, static_cast<_Tp&&>(__tmp.__v));
    }
    return begin() + static_cast<difference_type>(__off);
  }
  constexpr iterator insert(const_iterator position, const _Tp& __x) { return emplace(position, __x); }
  constexpr iterator insert(const_iterator position, _Tp&& __x) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    if (__last_ != __cap_ && __first_ + __off != __last_) { // x is not an element ([res.on.arguments]/1.3)
      __shift_in(__off, static_cast<_Tp&&>(__x));
      return begin() + static_cast<difference_type>(__off);
    }
    return emplace(position, static_cast<_Tp&&>(__x));
  }
  constexpr iterator insert(const_iterator position, size_type n, const _Tp& __x) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    if (n == 0)
      return begin() + static_cast<difference_type>(__off);
    if (n > __spare()) {
      __realloc_insert(__off, n, __grow_to(n), [&](_Tp* d) { __construct_n(d, n, __x); });
      return begin() + static_cast<difference_type>(__off);
    }
    // In place ([sequence.reqmts]: T is Cpp17CopyAssignable). x may be an element.
    __ycxx::__detail::__alloc_temp<_Tp, _Allocator> __tmp(__alloc_, __x);
    const _Tp& __v = __tmp.__v;
    _Tp* const p = __first_ + __off;
    _Tp* const __old_last = __last_;
    const size_type __after = static_cast<size_type>(__old_last - p);
    if (__after > n) {
      __destroy_guard __g{__alloc_, __old_last, __old_last};
      for (_Tp* s = __old_last - n; s != __old_last; ++s) {
        __alloc_traits::construct(__alloc_, __g.last, static_cast<_Tp&&>(*s));
        ++__g.last;
      }
      __g.first = __g.last;
      __last_ = __g.last;
      for (_Tp *s = __old_last - n, *d = __old_last; s != p;)
        *--d = static_cast<_Tp&&>(*--s);
      for (_Tp* d = p; d != p + n; ++d)
        *d = __v;
    } else {
      __last_ = __construct_n(__old_last, n - __after, __v);
      __destroy_guard __g{__alloc_, __last_, __last_};
      for (_Tp* s = p; s != __old_last; ++s) {
        __alloc_traits::construct(__alloc_, __g.last, static_cast<_Tp&&>(*s));
        ++__g.last;
      }
      __g.first = __g.last;
      __last_ = __g.last;
      for (_Tp* d = p; d != __old_last; ++d)
        *d = __v;
    }
    return begin() + static_cast<difference_type>(__off);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator position, _InputIterator first, _InputIterator last) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      return __insert_counted(__off, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                            static_cast<size_type>(n));
    } else {
      return __insert_input(__off, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr iterator insert_range(const_iterator position, _Rp&& __rg) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      return __insert_counted(__off, ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      if constexpr (ranges::approximately_sized_range<_Rp>) {
        const auto h = static_cast<size_type>(ranges::reserve_hint(__rg));
        if (h > __spare())
          reserve(size() + (h < max_size() - size() ? h : max_size() - size()));
      }
      return __insert_input(__off, ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<_Tp> il) {
    return __insert_counted(static_cast<size_type>(position - cbegin()), il.begin(), il.end(), il.size());
  }

  constexpr iterator erase(const_iterator position) {
    __ycxx::__detail::__precondition(position.base() != __last_, "std::vector::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    _Tp* const p = __mut(first);
    _Tp* const __q = __mut(last);
    if (p != __q) {
      _Tp* d = p;
      if constexpr (__memmove_assign) {
        if !consteval {
          const auto n = static_cast<size_t>(__last_ - __q);
          __builtin_memmove(static_cast<void*>(p), static_cast<const void*>(__q), n * sizeof(_Tp));
          d = p + n;
        }
      }
      if (d == p)
        for (_Tp* s = __q; s != __last_; ++s) {
          *d = static_cast<_Tp&&>(*s);
          ++d;
        }
      __destroy_range(d, __last_);
      __last_ = d;
    }
    return iterator(p);
  }
  constexpr void swap(vector& __x) noexcept(__pocs || __always_equal) {
    if constexpr (__pocs)
      __ycxx::__detail::__swap_adl::__do_swap(__alloc_, __x.__alloc_);
    else
      __ycxx::__detail::__precondition(__always_equal || __alloc_ == __x.__alloc_,
                                 "std::vector::swap: unequal allocators that do not propagate");
    _Tp* const __f = __first_;
    _Tp* const __l = __last_;
    _Tp* const c = __cap_;
    __first_ = __x.__first_;
    __last_ = __x.__last_;
    __cap_ = __x.__cap_;
    __x.__first_ = __f;
    __x.__last_ = __l;
    __x.__cap_ = c;
  }
  constexpr void clear() noexcept {
    __destroy_range(__first_, __last_);
    __last_ = __first_;
  }
};

// ---- deduction guides ([vector.overview]) ----
template <class _InputIterator, class _Allocator = allocator<typename iterator_traits<_InputIterator>::value_type>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
vector(_InputIterator, _InputIterator,
       _Allocator = _Allocator()) -> vector<typename iterator_traits<_InputIterator>::value_type, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
vector(from_range_t, _Rp&&, _Allocator = _Allocator()) -> vector<ranges::range_value_t<_Rp>, _Allocator>;

// ---- comparisons ([vector.syn], [container.reqmts]) ----
template <class _Tp, class _Allocator>
constexpr bool operator==(const vector<_Tp, _Allocator>& __x, const vector<_Tp, _Allocator>& y) {
  const auto n = __x.size();
  if (n != y.size())
    return false;
  const _Tp* a = __x.data();
  const _Tp* b = y.data();
  for (decltype(__x.size()) i = 0; i != n; ++i)
    if (!static_cast<bool>(a[i] == b[i]))
      return false;
  return true;
}
template <class _Tp, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Tp> operator<=>(const vector<_Tp, _Allocator>& __x,
                                                              const vector<_Tp, _Allocator>& y) {
  const auto __nx = __x.size();
  const auto __ny = y.size();
  const auto n = __nx < __ny ? __nx : __ny;
  const _Tp* a = __x.data();
  const _Tp* b = y.data();
  for (decltype(__x.size()) i = 0; i != n; ++i)
    if (auto c = __ycxx::__detail::__synth_three_way(a[i], b[i]); c != 0)
      return c;
  return __nx <=> __ny;
}

template <class _Tp, class _Allocator>
constexpr void swap(vector<_Tp, _Allocator>& __x, vector<_Tp, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [vector.erasure] ----
template <class _Tp, class _Allocator, class _Predicate>
constexpr typename vector<_Tp, _Allocator>::size_type erase_if(vector<_Tp, _Allocator>& c, _Predicate pred) {
  // remove_if, then erase the tail.
  _Tp* const base = c.data();
  _Tp* first = base;
  _Tp* const last = base + c.size();
  while (first != last && !static_cast<bool>(pred(*first)))
    ++first;
  _Tp* out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!static_cast<bool>(pred(*first))) {
        *out = static_cast<_Tp&&>(*first);
        ++out;
      }
  }
  const auto r = static_cast<typename vector<_Tp, _Allocator>::size_type>(last - out);
  c.erase(c.begin() + (out - base), c.end());
  return r;
}
template <class _Tp, class _Allocator, class _Up = _Tp>
constexpr typename vector<_Tp, _Allocator>::size_type erase(vector<_Tp, _Allocator>& c, const _Up& value) {
  return std::erase_if(c, [&value](const _Tp& e) { return static_cast<bool>(e == value); });
}

namespace pmr {
template <class _Tp>
using vector = std::vector<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

}} // namespace std
