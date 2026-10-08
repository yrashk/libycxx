// libycxx core: pointer tagging ([ptrtag]) -- max_pointer_bits_available, pointer_bits_available,
// pointer_tag_pair and its tuple interface. Freestanding.
//
// The tag lives in the low bits of the pointer's address (DECISIONS §9). During constant
// evaluation no compiler can set bits of a pointer, so there the member holds the untagged
// pointer and only the tag 0 can be stored.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [ptrtag.bits]/1: only the bits an alignment leaves zero are used, and a size_t alignment has
// at most (width - 1) trailing zeros, so the limit is the pointer width minus 1.
inline constexpr unsigned max_pointer_bits_available = __ycxx::__detail::__cfg::__pointer_bits - 1;

// [ptrtag.bits]/3-7
constexpr unsigned pointer_bits_available(size_t alignment) {
  __ycxx::__detail::__precondition(alignment != 0 && (alignment & (alignment - 1)) == 0,
                                   "std::pointer_bits_available: alignment is not a power of two");
  const unsigned __n = static_cast<unsigned>(std::countr_zero(alignment));
  return __n < max_pointer_bits_available ? __n : max_pointer_bits_available;
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

using __ptrtag_word = __UINTPTR_TYPE__;

// bits-available, element-of ([memory.syn])
template <class _Tp>
inline constexpr unsigned __bits_available = std::pointer_bits_available(alignof(_Tp));
template <class _Up>
using __element_of = typename std::pointer_traits<_Up>::element_type;

// tagging-compatible-pointee ([ptrtag.pair.general]) with an explicit alignment ...
template <class _Up, class _PtrT, unsigned _BitsRequested, std::size_t _Alignment>
concept __tagging_compatible_pointee_aligned =
    std::convertible_to<_Up*, _PtrT> && (std::pointer_bits_available(_Alignment) >= _BitsRequested) &&
    (std::is_void_v<__element_of<_PtrT>> || std::is_scalar_v<__element_of<_PtrT>> ||
     std::is_union_v<__element_of<_PtrT>> || std::is_pointer_interconvertible_base_of_v<__element_of<_PtrT>, _Up>);
// ... and with the draft's default, alignof(U): a U without an alignment (void, an incomplete
// type) does not satisfy it.
template <class _Up, class _PtrT, unsigned _BitsRequested>
concept __tagging_compatible_pointee =
    requires { alignof(_Up); } && __tagging_compatible_pointee_aligned<_Up, _PtrT, _BitsRequested, alignof(_Up)>;

// The unsigned type a tag is (TagT, or its underlying type for an enumeration).
template <class _TagT>
struct __ptrtag_unsigned {
  using type = _TagT;
};
template <class _TagT>
  requires std::is_enum_v<_TagT>
struct __ptrtag_unsigned<_TagT> {
  using type = std::underlying_type_t<_TagT>;
};

template <class _TagT>
constexpr __ptrtag_word __ptrtag_tag_word(_TagT __v) noexcept {
  return static_cast<__ptrtag_word>(static_cast<typename __ptrtag_unsigned<_TagT>::type>(__v));
}

// tag-bit-width ([ptrtag.pair.general]/5); bool and the character types, which bit_width does
// not take, through the same value as an unsigned integer.
template <class _TagT>
constexpr unsigned __tag_bit_width(_TagT __v) noexcept {
  return static_cast<unsigned>(std::bit_width(__ycxx::__detail::__ptrtag_tag_word(__v)));
}

// Whether `a <=> b` / `a == b` on two tags is certainly the built-in operator: an integer type,
// or an enumeration for which no operator function can be called (only user-declared ones
// can be named in a call; [ptrtag.pair.comp]/2, /4).
template <class _Tp>
concept __ptrtag_named_three_way = requires(_Tp __a) { operator<=>(__a, __a); };
template <class _Tp>
concept __ptrtag_named_equal = requires(_Tp __a) { operator==(__a, __a); };
template <class _Tp>
inline constexpr bool __ptrtag_builtin_three_way =
    std::is_integral_v<_Tp> || (std::is_enum_v<_Tp> && !__ptrtag_named_three_way<_Tp>);
template <class _Tp>
inline constexpr bool __ptrtag_builtin_equal =
    std::is_integral_v<_Tp> || (std::is_enum_v<_Tp> && !__ptrtag_named_equal<_Tp>);

// Called (it is not constexpr, so the evaluation is not a constant expression) when a non-zero
// tag would have to be stored during constant evaluation: neither GCC 16 nor Clang 23 can set
// bits of a pointer there (DECISIONS §9).
inline void __pointer_tag_pair_cannot_store_a_nonzero_tag_during_constant_evaluation() noexcept {}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Ptr, unsigned _BitsRequested = __ycxx::__detail::__bits_available<__ycxx::__detail::__element_of<_Ptr>>,
          class _TagT = unsigned>
class pointer_tag_pair {
  // [ptrtag.pair.general]/4
  static_assert(is_same_v<remove_cvref_t<_Ptr>, _Ptr>, "std::pointer_tag_pair: Ptr must be a cv-unqualified type");
  static_assert(is_same_v<remove_cvref_t<_TagT>, _TagT>, "std::pointer_tag_pair: TagT must be a cv-unqualified type");
  static_assert(is_pointer_v<_Ptr> && !is_function_v<remove_pointer_t<_Ptr>>,
                "std::pointer_tag_pair: Ptr must be an object pointer type");
  static_assert(is_unsigned_v<typename __ycxx::__detail::__ptrtag_unsigned<_TagT>::type>,
                "std::pointer_tag_pair: TagT must be an unsigned integer type or an enumeration with one as its "
                "underlying type");
  static_assert(sizeof(_TagT) <= sizeof(void*), "std::pointer_tag_pair: TagT must not be larger than a pointer");
  static_assert(_BitsRequested <= max_pointer_bits_available,
                "std::pointer_tag_pair: BitsRequested exceeds max_pointer_bits_available");

public:
  using pointer_type = _Ptr;
  using element_type = typename pointer_traits<_Ptr>::element_type;
  // [ptrtag.pair.tagops]/1: cv void* for a pointer to cv U.
  using tagged_pointer_type = __ycxx::__detail::__copy_cv<remove_pointer_t<_Ptr>, void>*;
  using tag_type = _TagT;
  static constexpr unsigned bits_requested = _BitsRequested;

private:
  using __word = __ycxx::__detail::__ptrtag_word;
  static constexpr __word __tag_mask = (__word(1) << _BitsRequested) - 1;

  // The pointer with the tag in its low bits (during constant evaluation: the pointer, tag 0).
  tagged_pointer_type __tp_;

  static constexpr tagged_pointer_type __make_tagged(pointer_type __p, tag_type __tv) {
    __ycxx::__detail::__precondition(__ycxx::__detail::__tag_bit_width(__tv) <= _BitsRequested,
                                     "std::pointer_tag_pair: the tag needs more than bits_requested bits");
    if consteval {
      if (__ycxx::__detail::__ptrtag_tag_word(__tv) != 0)
        __ycxx::__detail::__pointer_tag_pair_cannot_store_a_nonzero_tag_during_constant_evaluation();
      return static_cast<tagged_pointer_type>(__p);
    } else {
      const __word __a = reinterpret_cast<__word>(static_cast<tagged_pointer_type>(__p));
      __ycxx::__detail::__precondition((__a & __tag_mask) == 0,
                                       "std::pointer_tag_pair: the pointer is not aligned enough for the tag");
      return reinterpret_cast<tagged_pointer_type>(__a | __ycxx::__detail::__ptrtag_tag_word(__tv));
    }
  }

  __word __word_value() const noexcept { return reinterpret_cast<__word>(__tp_); }

public:
  // [ptrtag.pair.cons]
  constexpr pointer_tag_pair() noexcept : __tp_(nullptr) {}

  template <__ycxx::__detail::__tagging_compatible_pointee<pointer_type, bits_requested> _Up>
  constexpr pointer_tag_pair(_Up* p, tag_type t) : __tp_(__make_tagged(p, t)) {}

  constexpr pointer_tag_pair(nullptr_t, tag_type t) : __tp_(__make_tagged(nullptr, t)) {}

  // [ptrtag.pair.overalign]
  template <size_t _PromisedAlignment,
            __ycxx::__detail::__tagging_compatible_pointee_aligned<pointer_type, bits_requested, _PromisedAlignment> _Up>
  static constexpr pointer_tag_pair from_overaligned(_Up* p, tag_type t) {
    if consteval {
      if constexpr (__ycxx::__detail::__y_builtin::__has_is_aligned<_Up>)
        __ycxx::__detail::__precondition(p == nullptr || __builtin_is_aligned(p, _PromisedAlignment),
                                         "std::pointer_tag_pair::from_overaligned: the pointer is not aligned to "
                                         "PromisedAlignment");
    } else {
      __ycxx::__detail::__precondition(
          p == nullptr || reinterpret_cast<__word>(static_cast<tagged_pointer_type>(p)) % _PromisedAlignment == 0,
          "std::pointer_tag_pair::from_overaligned: the pointer is not aligned to PromisedAlignment");
    }
    pointer_tag_pair __r;
    __r.__tp_ = __make_tagged(p, t);
    return __r;
  }

  // [ptrtag.pair.tagops]
  tagged_pointer_type tagged_pointer() const noexcept { return __tp_; }

  static pointer_tag_pair from_tagged(tagged_pointer_type p) noexcept {
    pointer_tag_pair __r;
    __r.__tp_ = p;
    return __r;
  }

  // [ptrtag.pair.accessors]
  constexpr pointer_type pointer() const noexcept {
    if consteval {
      return static_cast<pointer_type>(__tp_);
    } else {
      return static_cast<pointer_type>(reinterpret_cast<tagged_pointer_type>(__word_value() & ~__tag_mask));
    }
  }

  constexpr tag_type tag() const noexcept {
    using _Ut = typename __ycxx::__detail::__ptrtag_unsigned<_TagT>::type;
    if consteval {
      return static_cast<tag_type>(_Ut(0));
    } else {
      return static_cast<tag_type>(static_cast<_Ut>(__word_value() & __tag_mask));
    }
  }

  // [ptrtag.pair.swap]
  constexpr void swap(pointer_tag_pair& __o) noexcept {
    const tagged_pointer_type __tv = __tp_;
    __tp_ = __o.__tp_;
    __o.__tp_ = __tv;
  }

  // [ptrtag.pair.comp]: (pointer(), tag()) compared as a pair; at run time, when the tag's
  // operator is the built-in one, the tagged words directly (the address bits are above the
  // tag bits, so the order is the same).
  friend constexpr auto operator<=>(pointer_tag_pair __lhs, pointer_tag_pair __rhs) noexcept
    requires three_way_comparable<tag_type>
  {
    using _Rp = common_comparison_category_t<__ycxx::__detail::__synth_three_way_result<pointer_type>,
                                             __ycxx::__detail::__synth_three_way_result<tag_type>>;
    if !consteval {
      if constexpr (__ycxx::__detail::__ptrtag_builtin_three_way<tag_type>)
        return static_cast<_Rp>(__lhs.__word_value() <=> __rhs.__word_value());
    }
    if (const auto c = __ycxx::__detail::__synth_three_way(__lhs.pointer(), __rhs.pointer()); c != 0)
      return static_cast<_Rp>(c);
    return static_cast<_Rp>(__ycxx::__detail::__synth_three_way(__lhs.tag(), __rhs.tag()));
  }

  friend constexpr bool operator==(pointer_tag_pair __lhs, pointer_tag_pair __rhs) noexcept
    requires equality_comparable<tag_type>
  {
    if !consteval {
      if constexpr (__ycxx::__detail::__ptrtag_builtin_equal<tag_type>)
        return __lhs.__word_value() == __rhs.__word_value();
    }
    return __lhs.pointer() == __rhs.pointer() && __lhs.tag() == __rhs.tag();
  }
};

// [ptrtag.pair.general]: the deduction guides. The first is as the draft declares it, although
// no constructor takes a pointer alone. The second's bits-available<element-of<Ptr>> cannot
// be formed for the pointee type Ptr (pointer_traits<int> has no element_type): the pointee's
// alignment is meant, bits-available<Ptr> (DECISIONS §9; STATUS, draft issues).
template <class _Ptr>
pointer_tag_pair(_Ptr*) -> pointer_tag_pair<_Ptr*>;
template <class _Ptr, class _TagT>
pointer_tag_pair(_Ptr*, _TagT) -> pointer_tag_pair<_Ptr*, __ycxx::__detail::__bits_available<_Ptr>, _TagT>;

// [memory.syn]: the tuple interface ([ptrtag.pair.get])
template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_size<pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> : integral_constant<size_t, 2> {};
template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<0, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type = _Ptr;
};
template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<1, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type = _TagT;
};
template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<0, const pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type = _Ptr;
};
template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<1, const pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type = _TagT;
};

template <size_t _Ip, class _Ptr, unsigned _BitsRequested, class _TagT>
constexpr tuple_element_t<_Ip, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>>
get(pointer_tag_pair<_Ptr, _BitsRequested, _TagT> p) noexcept {
  static_assert(_Ip < 2, "std::get<I>(pointer_tag_pair): I must be 0 or 1");
  if constexpr (_Ip == 0)
    return p.pointer();
  else
    return p.tag();
}

}} // namespace std
