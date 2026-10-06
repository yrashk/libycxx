// libycxx core: basic_string ([basic.string]), its non-member functions, hash support,
// literals, the pmr:: aliases and the integral to_string/to_wstring ([string.conversions]).
//
// Representation: {charT* ptr_, size_type size_, union {size_type cap_; charT buf_[N]}, alloc_}.
// A short string (at most sso_cap characters) lives in buf_ and ptr_ points at buf_; a long
// one lives in storage from the allocator and cap_ holds its capacity. is_long() is ptr_ != buf_.
// The same layout is used during constant evaluation: switching the union's active member is
// done with plain assignments (buf_[0] = ..., cap_ = ...), which [class.union]/6 allows there,
// and the characters of allocator storage are value-initialized with construct_at first, since
// Clang does not let an assignment begin their lifetime. So a string built during constant
// initialization (a global std::string) is an ordinary short string at run time.
//
// The numeric conversions that need the C library (sto*, the floating-point to_string and
// to_wstring) are only declared here; they are defined out of line in the hosted runtime
// (src/hosted/string.cpp), as the <stdexcept> members are (DECISIONS §3).
#pragma once

#include <initializer_list>
#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_string;

template <class __charT, class __traits, class _Allocator>
class basic_string {
  static_assert(is_same_v<typename __traits::char_type, __charT>,
                "std::basic_string: traits::char_type must be charT ([string.require])");
  static_assert(is_same_v<typename _Allocator::value_type, __charT>,
                "std::basic_string: Allocator::value_type must be charT ([string.require])");
  static_assert(!is_array_v<__charT> && is_trivially_copyable_v<__charT> && is_trivially_default_constructible_v<__charT> &&
                    is_standard_layout_v<__charT>,
                "std::basic_string: charT must be a char-like type");

  using __alloc_traits = allocator_traits<_Allocator>;
  using __sv_type = basic_string_view<__charT, __traits>;

public:
  // ---- types ----
  using traits_type = __traits;
  using value_type = __charT;
  using allocator_type = _Allocator;
  using size_type = typename __alloc_traits::size_type;
  using difference_type = typename __alloc_traits::difference_type;
  using pointer = typename __alloc_traits::pointer;
  using const_pointer = typename __alloc_traits::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = __ycxx::__adl_free::__contiguous_iter<__charT, basic_string, difference_type>;
  using const_iterator = __ycxx::__adl_free::__contiguous_iter<const __charT, basic_string, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  static constexpr size_type npos = size_type(-1);

private:
  // T is "string-view-like": the constraint of most string_view-taking members. Extension:
  // classes derived from basic_string are excluded, so that they bind to the basic_string
  // overloads (a derived rvalue is then moved from, not copied through a string_view).
  template <class _Tp>
  static constexpr bool __sv_like = is_convertible_v<const _Tp&, __sv_type> && !is_convertible_v<const _Tp&, const __charT*> &&
                                  !is_convertible_v<const _Tp*, const basic_string*>;
  // An iterator whose elements can be read through a charT pointer.
  template <class _It>
  static constexpr bool __char_ptr_iter = contiguous_iterator<_It> && is_same_v<iter_value_t<_It>, __charT>;

  static constexpr bool __pocca = __alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool __pocma = __alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool __pocs = __alloc_traits::propagate_on_container_swap::value;
  static constexpr bool __always_equal = __alloc_traits::is_always_equal::value;

  // Inline buffer: 16 bytes' worth of characters (at least one, for the terminator).
  static constexpr size_t __buf_len = 16 / sizeof(__charT) > 1 ? 16 / sizeof(__charT) : 1;
  static constexpr size_type __sso_cap = __buf_len - 1;

  __charT* __ptr_;
  size_type __size_;
  union {
    size_type __cap_;
    __charT __buf_[__buf_len];
  };
  [[no_unique_address]] _Allocator __alloc_;

  // ---- representation helpers ----
  constexpr bool __is_long() const noexcept { return __ptr_ != __buf_; }
  constexpr size_type __cap() const noexcept { return __is_long() ? __cap_ : __sso_cap; }

  // Makes buf_ the active union member (ending cap_'s lifetime) and points ptr_ at it. During
  // constant evaluation every element is initialized: a constant-initialized string must not
  // hold indeterminate values.
  constexpr void __activate_buf() noexcept {
    __buf_[0] = __charT();
    if consteval {
      for (size_t i = 1; i < __buf_len; ++i)
        __buf_[i] = __charT();
    }
    __ptr_ = __buf_;
  }
  // Makes *this an empty short string. Any long storage must already have been released (or
  // taken over).
  constexpr void __set_short_empty() noexcept {
    __activate_buf();
    __size_ = 0;
  }
  constexpr void __set_long(__charT* p, size_type c) noexcept {
    __ptr_ = p;
    __cap_ = c;
  }

  struct block {
    __charT* p;
    size_type __cap; // characters, excluding the terminator
  };
  // Storage for at least n characters plus the terminator.
  static constexpr block __allocate_block(_Allocator& a, size_type n) {
    auto r = __alloc_traits::allocate_at_least(a, n + 1);
    __charT* p = std::to_address(r.ptr);
    if consteval {
      for (size_type i = 0; i < r.count; ++i)
        std::construct_at(p + i);
    }
    return {p, static_cast<size_type>(r.count - 1)};
  }
  static constexpr void __deallocate_block(_Allocator& a, __charT* p, size_type c) noexcept {
    __alloc_traits::deallocate(a, pointer_traits<pointer>::pointer_to(*p), c + 1);
  }
  constexpr void __free_storage() noexcept {
    if (__is_long())
      __deallocate_block(__alloc_, __ptr_, __cap_);
  }

  constexpr void __check_length(size_type n, const char* what) const {
    if (n > max_size())
      __ycxx::__detail::__throw_length_error(what);
  }
  constexpr void __check_pos(size_type __pos, const char* what) const {
    if (__pos > __size_)
      __ycxx::__detail::__throw_out_of_range(what);
  }
  constexpr size_type clamp(size_type __pos, size_type n) const noexcept {
    return n < __size_ - __pos ? n : __size_ - __pos;
  }

  // Capacity for a string that must grow to new_size (> cap()): geometric growth.
  constexpr size_type __grow_cap(size_type __new_size) const {
    const size_type ms = max_size();
    if (__new_size > ms)
      __ycxx::__detail::__throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type c = __cap();
    size_type __nc = c <= ms / 2 ? 2 * c : ms;
    return __nc < __new_size ? __new_size : __nc;
  }

  // Initialisation of a freshly constructed object (no storage owned yet).
  constexpr void __init_copy(const __charT* s, size_type n) {
    if (n <= __sso_cap) {
      __activate_buf();
    } else {
      __check_length(n, "std::basic_string: length exceeds max_size()");
      block b = __allocate_block(__alloc_, n);
      __set_long(b.p, b.__cap);
    }
    __traits::copy(__ptr_, s, n);
    __traits::assign(__ptr_[n], __charT());
    __size_ = n;
  }
  constexpr void __init_fill(size_type n, __charT c) {
    if (n <= __sso_cap) {
      __activate_buf();
    } else {
      __check_length(n, "std::basic_string: length exceeds max_size()");
      block b = __allocate_block(__alloc_, n);
      __set_long(b.p, b.__cap);
    }
    __traits::assign(__ptr_, n, c);
    __traits::assign(__ptr_[n], __charT());
    __size_ = n;
  }
  // Takes over o's characters (and storage, if long); o becomes empty. *this owns nothing.
  constexpr void take(basic_string& __o) noexcept {
    if (__o.__is_long()) {
      __set_long(__o.__ptr_, __o.__cap_);
      __size_ = __o.__size_;
    } else {
      __activate_buf();
      __traits::copy(__buf_, __o.__buf_, __o.__size_ + 1);
      __size_ = __o.__size_;
    }
    __o.__set_short_empty();
  }

  // Moves the characters into a new block of capacity at least c (>= size_).
  constexpr void __reallocate(size_type c) {
    block b = __allocate_block(__alloc_, c);
    __traits::copy(b.p, __ptr_, __size_ + 1);
    __free_storage();
    __set_long(b.p, b.__cap);
  }

  // Index of s within [data(), data() + size()], or npos. Only equality comparisons are made
  // during constant evaluation, where pointers into unrelated objects cannot be ordered.
  constexpr size_type offset_of(const __charT* s) const noexcept {
    if consteval {
      for (size_type i = 0; i <= __size_; ++i)
        if (__ptr_ + i == s)
          return i;
      return npos;
    } else {
      const auto a = reinterpret_cast<__UINTPTR_TYPE__>(s);
      const auto b = reinterpret_cast<__UINTPTR_TYPE__>(__ptr_);
      if (a < b || a > b + __size_ * sizeof(__charT))
        return npos;
      return static_cast<size_type>((a - b) / sizeof(__charT));
    }
  }

  // Replaces [pos, pos + n1) with [s, s + n2); pos <= size_, n1 <= size_ - pos. The source may
  // lie inside *this. Strong guarantee: everything that can throw happens before any change.
  constexpr basic_string& __replace_impl(size_type __pos, size_type __n1, const __charT* s, size_type __n2) {
    const size_type __sz = __size_;
    if (__n2 > __n1 && __n2 - __n1 > max_size() - __sz)
      __ycxx::__detail::__throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type __new_size = __sz - __n1 + __n2;
    const size_type __tail = __sz - __pos - __n1;
    if (__new_size > __cap()) {
      // The old characters stay intact until the new block is complete, so s may alias them.
      block b = __allocate_block(__alloc_, __grow_cap(__new_size));
      __traits::copy(b.p, __ptr_, __pos);
      __traits::copy(b.p + __pos, s, __n2);
      __traits::copy(b.p + __pos + __n2, __ptr_ + __pos + __n1, __tail);
      __traits::assign(b.p[__new_size], __charT());
      __free_storage();
      __set_long(b.p, b.__cap);
      __size_ = __new_size;
      return *this;
    }
    __charT* const p = __ptr_;
    const size_type __off = __n2 == 0 ? npos : offset_of(s);
    if (__off == npos) {
      if (__n1 != __n2)
        __traits::move(p + __pos + __n2, p + __pos + __n1, __tail);
      __traits::copy(p + __pos, s, __n2);
    } else if (__n1 == __n2) {
      __traits::move(p + __pos, p + __off, __n2);
    } else if (__n2 < __n1) {
      // Shrinking: place the source first (it ends up inside the replaced range), then close
      // the gap.
      __traits::move(p + __pos, p + __off, __n2);
      __traits::move(p + __pos + __n2, p + __pos + __n1, __tail);
    } else {
      // Growing: open the gap first (moving the terminator too, which the source may include).
      // Source characters at or after pos + n1 move by n2 - n1.
      __traits::move(p + __pos + __n2, p + __pos + __n1, __tail + 1);
      if (__off + __n2 <= __pos + __n1) {
        __traits::move(p + __pos, p + __off, __n2);
      } else if (__off >= __pos + __n1) {
        __traits::copy(p + __pos, p + __off + (__n2 - __n1), __n2);
      } else {
        const size_type k = __pos + __n1 - __off; // unmoved prefix of the source
        __traits::move(p + __pos, p + __off, k);
        __traits::copy(p + __pos + k, p + __pos + __n2, __n2 - k);
      }
    }
    __traits::assign(p[__new_size], __charT());
    __size_ = __new_size;
    return *this;
  }

  // Replaces [pos, pos + n1) with n2 copies of c (same preconditions as replace_impl).
  constexpr basic_string& __replace_fill(size_type __pos, size_type __n1, size_type __n2, __charT c) {
    const size_type __sz = __size_;
    if (__n2 > __n1 && __n2 - __n1 > max_size() - __sz)
      __ycxx::__detail::__throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type __new_size = __sz - __n1 + __n2;
    const size_type __tail = __sz - __pos - __n1;
    if (__new_size > __cap()) {
      block b = __allocate_block(__alloc_, __grow_cap(__new_size));
      __traits::copy(b.p, __ptr_, __pos);
      __traits::assign(b.p + __pos, __n2, c);
      __traits::copy(b.p + __pos + __n2, __ptr_ + __pos + __n1, __tail);
      __traits::assign(b.p[__new_size], __charT());
      __free_storage();
      __set_long(b.p, b.__cap);
    } else {
      if (__n1 != __n2)
        __traits::move(__ptr_ + __pos + __n2, __ptr_ + __pos + __n1, __tail);
      __traits::assign(__ptr_ + __pos, __n2, c);
      __traits::assign(__ptr_[__new_size], __charT());
    }
    __size_ = __new_size;
    return *this;
  }

  constexpr void __erase_impl(size_type __pos, size_type n) noexcept {
    if (n == 0)
      return;
    __traits::move(__ptr_ + __pos, __ptr_ + __pos + n, __size_ - __pos - n);
    __size_ -= n;
    __traits::assign(__ptr_[__size_], __charT());
  }

  // Appends [first, last) to *this element by element; the elements must not live in *this.
  template <class _It, class _Sent>
  constexpr void __append_elements(_It first, _Sent last) {
    if constexpr (forward_iterator<_It>) {
      const auto d = ranges::distance(first, last);
      if (static_cast<make_unsigned_t<decltype(d)>>(d) > max_size() - __size_)
        __ycxx::__detail::__throw_length_error("std::basic_string: length exceeds max_size()");
      const size_type n = static_cast<size_type>(d);
      if (__size_ + n > __cap())
        __reallocate(__grow_cap(__size_ + n));
      __charT* p = __ptr_ + __size_;
      for (; first != last; ++first) // no ',' on the user's iterator
        __traits::assign(*p++, static_cast<__charT>(*first));
      __size_ += n;
      __traits::assign(__ptr_[__size_], __charT());
    } else {
      for (; first != last; ++first)
        push_back(static_cast<__charT>(*first));
    }
  }
  // A string holding [first, last), with this string's allocator; used where the elements may
  // live in *this.
  template <class _It, class _Sent>
  constexpr basic_string __temp_of(_It first, _Sent last) const {
    basic_string t(__alloc_);
    t.__append_elements(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
    return t;
  }
  // Contiguous charT storage that traits::copy can read: not volatile charT (its range_value_t is
  // charT too), whose elements are read one by one.
  template <class _Rp>
  static constexpr bool __char_contiguous_range =
      ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp> && is_same_v<ranges::range_value_t<_Rp>, __charT> &&
      !is_volatile_v<remove_reference_t<ranges::range_reference_t<_Rp>>>;

public:
  // ---- [string.cons] ----
  constexpr basic_string() noexcept(noexcept(_Allocator())) : basic_string(_Allocator()) {}
  constexpr explicit basic_string(const _Allocator& a) noexcept : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __set_short_empty();
  }
  constexpr basic_string(const basic_string& str)
      : __ptr_(nullptr), __size_(0), __alloc_(__alloc_traits::select_on_container_copy_construction(str.__alloc_)) {
    __init_copy(str.__ptr_, str.__size_);
  }
  constexpr basic_string(basic_string&& str) noexcept
      : __ptr_(nullptr), __size_(0), __alloc_(static_cast<_Allocator&&>(str.__alloc_)) {
    take(str);
  }
  constexpr basic_string(const basic_string& str, size_type __pos, const _Allocator& a = _Allocator())
      : basic_string(str, __pos, npos, a) {}
  constexpr basic_string(const basic_string& str, size_type __pos, size_type n, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    str.__check_pos(__pos, "std::basic_string: pos > str.size()");
    __init_copy(str.__ptr_ + __pos, str.clamp(__pos, n));
  }
  constexpr basic_string(basic_string&& str, size_type __pos, const _Allocator& a = _Allocator())
      : basic_string(static_cast<basic_string&&>(str), __pos, npos, a) {}
  constexpr basic_string(basic_string&& str, size_type __pos, size_type n, const _Allocator& a = _Allocator())
      : basic_string(a) {
    str.__check_pos(__pos, "std::basic_string: pos > str.size()");
    const size_type __rlen = str.clamp(__pos, n);
    if (__always_equal || __alloc_ == str.__alloc_) {
      // Reuse str's storage ([string.cons]/8).
      take(str);
      __traits::move(__ptr_, __ptr_ + __pos, __rlen);
      __size_ = __rlen;
      __traits::assign(__ptr_[__rlen], __charT());
    } else {
      assign(str.__ptr_ + __pos, __rlen);
    }
  }
  template <class _Tp>
    requires is_convertible_v<const _Tp&, basic_string_view<__charT, __traits>>
  constexpr basic_string(const _Tp& t, __ycxx::__detail::__alloc_size_t<_Allocator> __pos, __ycxx::__detail::__alloc_size_t<_Allocator> n, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    const __sv_type sv = __sv_type(t).substr(__pos, n);
    __init_copy(sv.data(), sv.size());
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr explicit basic_string(const _Tp& t, const _Allocator& a = _Allocator()) : __ptr_(nullptr), __size_(0), __alloc_(a) {
    const __sv_type sv = t;
    __init_copy(sv.data(), sv.size());
  }
  constexpr basic_string(const __charT* s, __ycxx::__detail::__alloc_size_t<_Allocator> n, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __ycxx::__detail::__precondition(s != nullptr || n == 0, "std::basic_string: null pointer with nonzero length");
    __init_copy(s, n);
  }
  constexpr basic_string(const __charT* s, const _Allocator& a = _Allocator())
    requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __ycxx::__detail::__precondition(s != nullptr, "std::basic_string: null pointer");
    __init_copy(s, __traits::length(s));
  }
  basic_string(nullptr_t) = delete;
  constexpr basic_string(__ycxx::__detail::__alloc_size_t<_Allocator> n, __charT c, const _Allocator& a = _Allocator())
    requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __init_fill(n, c);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr basic_string(_InputIterator begin, _InputIterator end, const _Allocator& a = _Allocator()) : basic_string(a) {
    __append_elements(static_cast<_InputIterator&&>(begin), static_cast<_InputIterator&&>(end));
  }
  template <__ycxx::__detail::__container_compatible_range<__charT> _Rp>
  constexpr basic_string(from_range_t, _Rp&& __rg, const _Allocator& a = _Allocator()) : basic_string(a) {
    if constexpr (__char_contiguous_range<_Rp>)
      append(ranges::data(__rg), static_cast<size_type>(ranges::size(__rg)));
    else
      __append_elements(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr basic_string(initializer_list<__charT> il, const _Allocator& a = _Allocator())
      : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __init_copy(il.begin(), il.size());
  }
  constexpr basic_string(const basic_string& str, const _Allocator& a) : __ptr_(nullptr), __size_(0), __alloc_(a) {
    __init_copy(str.__ptr_, str.__size_);
  }
  constexpr basic_string(basic_string&& str, const _Allocator& a) : __ptr_(nullptr), __size_(0), __alloc_(a) {
    if (__always_equal || __alloc_ == str.__alloc_)
      take(str);
    else
      __init_copy(str.__ptr_, str.__size_);
  }

  constexpr ~basic_string() { __free_storage(); }

  constexpr basic_string& operator=(const basic_string& str) {
    if (this == __builtin_addressof(str))
      return *this;
    if constexpr (__pocca) {
      if (!__always_equal && __alloc_ != str.__alloc_) {
        // The new allocator must own the storage: allocate with it before releasing ours.
        _Allocator __na = str.__alloc_;
        if (str.__size_ <= __sso_cap) {
          __free_storage();
          __alloc_ = __na;
          __set_short_empty();
        } else {
          block b = __allocate_block(__na, str.__size_);
          __free_storage();
          __alloc_ = __na;
          __set_long(b.p, b.__cap);
        }
        __traits::copy(__ptr_, str.__ptr_, str.__size_ + 1);
        __size_ = str.__size_;
        return *this;
      }
      __alloc_ = str.__alloc_;
    }
    return __replace_impl(0, __size_, str.__ptr_, str.__size_);
  }
  constexpr basic_string& operator=(basic_string&& str) noexcept(__pocma || __always_equal) {
    if (this == __builtin_addressof(str))
      return *this;
    if constexpr (__pocma || __always_equal) {
      __free_storage();
      if constexpr (__pocma)
        __alloc_ = static_cast<_Allocator&&>(str.__alloc_);
      take(str);
    } else {
      if (__alloc_ == str.__alloc_) {
        __free_storage();
        take(str);
      } else {
        __replace_impl(0, __size_, str.__ptr_, str.__size_);
      }
    }
    return *this;
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& operator=(const _Tp& t) {
    const __sv_type sv = t;
    return assign(sv);
  }
  constexpr basic_string& operator=(const __charT* s) { return *this = __sv_type(s); }
  basic_string& operator=(nullptr_t) = delete;
  constexpr basic_string& operator=(__charT c) { return *this = __sv_type(__builtin_addressof(c), 1); }
  constexpr basic_string& operator=(initializer_list<__charT> il) { return *this = __sv_type(il.begin(), il.size()); }

  // ---- [string.iterators] ----
  constexpr iterator begin() noexcept { return iterator(__ptr_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__ptr_); }
  constexpr iterator end() noexcept { return iterator(__ptr_ + __size_); }
  constexpr const_iterator end() const noexcept { return const_iterator(__ptr_ + __size_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [string.capacity] ----
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type length() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    // One element of every allocation holds the terminator; pointer differences over the string
    // must be representable, and no object is larger than PTRDIFF_MAX bytes.
    const size_type __by_alloc = __alloc_traits::max_size(__alloc_);
    const auto __diff_max = static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max()) /
                          sizeof(__charT);
    const size_type __by_diff = __diff_max < numeric_limits<size_type>::max() ? static_cast<size_type>(__diff_max)
                                                                          : numeric_limits<size_type>::max();
    return (__by_alloc < __by_diff ? __by_alloc : __by_diff) - 1;
  }
  constexpr void resize(size_type n, __charT c) {
    if (n <= __size_) {
      __size_ = n;
      __traits::assign(__ptr_[n], __charT());
    } else {
      append(n - __size_, c);
    }
  }
  constexpr void resize(size_type n) { resize(n, __charT()); }
  template <class _Operation>
  constexpr void resize_and_overwrite(size_type n, _Operation op) {
    using _Rp = decltype(static_cast<_Operation&&>(op)(declval<__charT*>(), declval<size_type>()));
    static_assert(__ycxx::__detail::__integer_like<_Rp>,
                  "std::basic_string::resize_and_overwrite: the operation must return an integer-like type");
    if (n > __cap()) {
      __check_length(n, "std::basic_string::resize_and_overwrite: n > max_size()");
      __reallocate(n);
    }
    __charT* const p = __ptr_;
    const size_type m = n;
    // [string.capacity]/7 calls p and m "values": they are passed as prvalues, so the operation
    // may take them by value or by rvalue reference.
    const _Rp r = static_cast<_Operation&&>(op)(static_cast<__charT*>(p), static_cast<size_type>(m));
    if constexpr (is_signed_v<_Rp>)
      __ycxx::__detail::__precondition(r >= 0, "std::basic_string::resize_and_overwrite: negative result");
    __ycxx::__detail::__precondition(static_cast<make_unsigned_t<_Rp>>(r) <= m,
                               "std::basic_string::resize_and_overwrite: result greater than n");
    __size_ = static_cast<size_type>(r);
    __traits::assign(p[__size_], __charT());
  }
  constexpr size_type capacity() const noexcept { return __cap(); }
  constexpr void reserve(size_type __res_arg) {
    __check_length(__res_arg, "std::basic_string::reserve: argument exceeds max_size()");
    if (__res_arg > __cap())
      __reallocate(__res_arg);
  }
  constexpr void shrink_to_fit() {
    if (!__is_long())
      return;
    if (__size_ <= __sso_cap) {
      __charT* const __old = __ptr_;
      const size_type __old_cap = __cap_;
      __activate_buf();
      __traits::copy(__buf_, __old, __size_ + 1);
      __deallocate_block(__alloc_, __old, __old_cap);
    } else if (__cap_ > __size_) {
      block b = __allocate_block(__alloc_, __size_);
      if (b.__cap >= __cap_) { // the allocator gave nothing back
        __deallocate_block(__alloc_, b.p, b.__cap);
        return;
      }
      __traits::copy(b.p, __ptr_, __size_ + 1);
      __free_storage();
      __set_long(b.p, b.__cap);
    }
  }
  constexpr void clear() noexcept {
    __size_ = 0;
    __traits::assign(__ptr_[0], __charT());
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }

  // ---- [string.access] ----
  constexpr const_reference operator[](size_type __pos) const {
    __ycxx::__detail::__precondition(__pos <= __size_, "std::basic_string::operator[]: index out of range");
    return __ptr_[__pos];
  }
  constexpr reference operator[](size_type __pos) {
    __ycxx::__detail::__precondition(__pos <= __size_, "std::basic_string::operator[]: index out of range");
    return __ptr_[__pos];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= __size_)
      __ycxx::__detail::__throw_out_of_range("std::basic_string::at: index out of range");
    return __ptr_[n];
  }
  constexpr reference at(size_type n) {
    if (n >= __size_)
      __ycxx::__detail::__throw_out_of_range("std::basic_string::at: index out of range");
    return __ptr_[n];
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string::front: empty string");
    return __ptr_[0];
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string::front: empty string");
    return __ptr_[0];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string::back: empty string");
    return __ptr_[__size_ - 1];
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string::back: empty string");
    return __ptr_[__size_ - 1];
  }

  // ---- [string.op.append] ----
  constexpr basic_string& operator+=(const basic_string& str) { return append(str); }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& operator+=(const _Tp& t) {
    const __sv_type sv = t;
    return append(sv.data(), sv.size());
  }
  constexpr basic_string& operator+=(const __charT* s) { return append(s); }
  constexpr basic_string& operator+=(__charT c) {
    push_back(c);
    return *this;
  }
  constexpr basic_string& operator+=(initializer_list<__charT> il) { return append(il); }

  // ---- [string.append] ----
  constexpr basic_string& append(const basic_string& str) { return append(str.__ptr_, str.__size_); }
  constexpr basic_string& append(const basic_string& str, size_type __pos, size_type n = npos) {
    return append(__sv_type(str).substr(__pos, n));
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& append(const _Tp& t) {
    const __sv_type sv = t;
    return append(sv.data(), sv.size());
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& append(const _Tp& t, size_type __pos, size_type n = npos) {
    const __sv_type sv = t;
    return append(sv.substr(__pos, n));
  }
  constexpr basic_string& append(const __charT* s, size_type n) {
    __ycxx::__detail::__precondition(s != nullptr || n == 0, "std::basic_string::append: null pointer");
    if (n <= __cap() - __size_) {
      // In place: the destination lies past every character, so even a source inside *this
      // stays intact (move() also covers a source that includes the terminator).
      __traits::move(__ptr_ + __size_, s, n);
      __size_ += n;
      __traits::assign(__ptr_[__size_], __charT());
      return *this;
    }
    return __replace_impl(__size_, 0, s, n);
  }
  constexpr basic_string& append(const __charT* s) {
    __ycxx::__detail::__precondition(s != nullptr, "std::basic_string::append: null pointer");
    return append(s, __traits::length(s));
  }
  constexpr basic_string& append(const __charT* s, size_type __pos, size_type n) { return append(__sv_type(s).substr(__pos, n)); }
  constexpr basic_string& append(size_type n, __charT c) { return __replace_fill(__size_, 0, n, c); }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr basic_string& append(_InputIterator first, _InputIterator last) {
    if constexpr (__char_ptr_iter<_InputIterator>)
      return append(std::to_address(first), static_cast<size_type>(last - first));
    else
      return append(__temp_of(first, last));
  }
  template <__ycxx::__detail::__container_compatible_range<__charT> _Rp>
  constexpr basic_string& append_range(_Rp&& __rg) {
    if constexpr (__char_contiguous_range<_Rp>)
      return append(ranges::data(__rg), static_cast<size_type>(ranges::size(__rg)));
    else
      return append(__temp_of(ranges::begin(__rg), ranges::end(__rg)));
  }
  constexpr basic_string& append(initializer_list<__charT> il) { return append(il.begin(), il.size()); }
  constexpr void push_back(__charT c) {
    const size_type n = __size_;
    if (n == __cap())
      __reallocate(__grow_cap(n + 1));
    // Through locals: a store of a char could alias ptr_ and size_ and force their reload.
    __charT* const p = __ptr_;
    __traits::assign(p[n], c);
    __traits::assign(p[n + 1], __charT());
    __size_ = n + 1;
  }

  // ---- [string.assign] ----
  constexpr basic_string& assign(const basic_string& str) { return *this = str; }
  constexpr basic_string& assign(basic_string&& str) noexcept(__pocma || __always_equal) {
    return *this = static_cast<basic_string&&>(str);
  }
  constexpr basic_string& assign(const basic_string& str, size_type __pos, size_type n = npos) {
    return assign(__sv_type(str).substr(__pos, n));
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& assign(const _Tp& t) {
    const __sv_type sv = t;
    return assign(sv.data(), sv.size());
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& assign(const _Tp& t, size_type __pos, size_type n = npos) {
    const __sv_type sv = t;
    return assign(sv.substr(__pos, n));
  }
  constexpr basic_string& assign(const __charT* s, size_type n) {
    __ycxx::__detail::__precondition(s != nullptr || n == 0, "std::basic_string::assign: null pointer");
    return __replace_impl(0, __size_, s, n);
  }
  constexpr basic_string& assign(const __charT* s) {
    __ycxx::__detail::__precondition(s != nullptr, "std::basic_string::assign: null pointer");
    return assign(s, __traits::length(s));
  }
  constexpr basic_string& assign(const __charT* s, size_type __pos, size_type n) { return assign(__sv_type(s).substr(__pos, n)); }
  constexpr basic_string& assign(initializer_list<__charT> il) { return assign(il.begin(), il.size()); }
  constexpr basic_string& assign(size_type n, __charT c) { return __replace_fill(0, __size_, n, c); }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr basic_string& assign(_InputIterator first, _InputIterator last) {
    if constexpr (__char_ptr_iter<_InputIterator>)
      return assign(std::to_address(first), static_cast<size_type>(last - first));
    else
      return assign(__temp_of(first, last));
  }
  template <__ycxx::__detail::__container_compatible_range<__charT> _Rp>
  constexpr basic_string& assign_range(_Rp&& __rg) {
    if constexpr (__char_contiguous_range<_Rp>)
      return assign(ranges::data(__rg), static_cast<size_type>(ranges::size(__rg)));
    else
      return assign(__temp_of(ranges::begin(__rg), ranges::end(__rg)));
  }

  // ---- [string.insert] ----
  constexpr basic_string& insert(size_type __pos, const basic_string& str) { return insert(__pos, str.__ptr_, str.__size_); }
  constexpr basic_string& insert(size_type __pos1, const basic_string& str, size_type __pos2, size_type n = npos) {
    return insert(__pos1, __sv_type(str), __pos2, n);
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& insert(size_type __pos, const _Tp& t) {
    const __sv_type sv = t;
    return insert(__pos, sv.data(), sv.size());
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& insert(size_type __pos1, const _Tp& t, size_type __pos2, size_type n = npos) {
    const __sv_type sv = t;
    return insert(__pos1, sv.substr(__pos2, n));
  }
  constexpr basic_string& insert(size_type __pos, const __charT* s, size_type n) {
    __ycxx::__detail::__precondition(s != nullptr || n == 0, "std::basic_string::insert: null pointer");
    __check_pos(__pos, "std::basic_string::insert: pos > size()");
    return __replace_impl(__pos, 0, s, n);
  }
  constexpr basic_string& insert(size_type __pos, const __charT* s) {
    __ycxx::__detail::__precondition(s != nullptr, "std::basic_string::insert: null pointer");
    return insert(__pos, s, __traits::length(s));
  }
  constexpr basic_string& insert(size_type __pos, size_type n, __charT c) {
    __check_pos(__pos, "std::basic_string::insert: pos > size()");
    return __replace_fill(__pos, 0, n, c);
  }
  constexpr iterator insert(const_iterator p, __charT c) {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    __replace_fill(__pos, 0, 1, c);
    return begin() + static_cast<difference_type>(__pos);
  }
  constexpr iterator insert(const_iterator p, size_type n, __charT c) {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    __replace_fill(__pos, 0, n, c);
    return begin() + static_cast<difference_type>(__pos);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator p, _InputIterator first, _InputIterator last) {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    if constexpr (__char_ptr_iter<_InputIterator>) {
      __replace_impl(__pos, 0, std::to_address(first), static_cast<size_type>(last - first));
    } else {
      const basic_string t = __temp_of(first, last);
      __replace_impl(__pos, 0, t.__ptr_, t.__size_);
    }
    return begin() + static_cast<difference_type>(__pos);
  }
  template <__ycxx::__detail::__container_compatible_range<__charT> _Rp>
  constexpr iterator insert_range(const_iterator p, _Rp&& __rg) {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    if constexpr (__char_contiguous_range<_Rp>) {
      __replace_impl(__pos, 0, ranges::data(__rg), static_cast<size_type>(ranges::size(__rg)));
    } else {
      const basic_string t = __temp_of(ranges::begin(__rg), ranges::end(__rg));
      __replace_impl(__pos, 0, t.__ptr_, t.__size_);
    }
    return begin() + static_cast<difference_type>(__pos);
  }
  constexpr iterator insert(const_iterator p, initializer_list<__charT> il) {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    __replace_impl(__pos, 0, il.begin(), il.size());
    return begin() + static_cast<difference_type>(__pos);
  }

  // ---- [string.erase] ----
  constexpr basic_string& erase(size_type __pos = 0, size_type n = npos) {
    __check_pos(__pos, "std::basic_string::erase: pos > size()");
    __erase_impl(__pos, clamp(__pos, n));
    return *this;
  }
  constexpr iterator erase(const_iterator p) noexcept {
    const size_type __pos = static_cast<size_type>(p - cbegin());
    __ycxx::__detail::__precondition(__pos < __size_, "std::basic_string::erase: iterator not dereferenceable");
    __erase_impl(__pos, 1);
    return begin() + static_cast<difference_type>(__pos);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
    const size_type __pos = static_cast<size_type>(first - cbegin());
    __erase_impl(__pos, static_cast<size_type>(last - first));
    return begin() + static_cast<difference_type>(__pos);
  }
  constexpr void pop_back() noexcept {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string::pop_back: empty string");
    --__size_;
    __traits::assign(__ptr_[__size_], __charT());
  }

  // ---- [string.replace] ----
  constexpr basic_string& replace(size_type __pos1, size_type __n1, const basic_string& str) {
    return replace(__pos1, __n1, str.__ptr_, str.__size_);
  }
  constexpr basic_string& replace(size_type __pos1, size_type __n1, const basic_string& str, size_type __pos2,
                                  size_type __n2 = npos) {
    return replace(__pos1, __n1, __sv_type(str).substr(__pos2, __n2));
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& replace(size_type __pos1, size_type __n1, const _Tp& t) {
    const __sv_type sv = t;
    return replace(__pos1, __n1, sv.data(), sv.size());
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& replace(size_type __pos1, size_type __n1, const _Tp& t, size_type __pos2, size_type __n2 = npos) {
    const __sv_type sv = t;
    return replace(__pos1, __n1, sv.substr(__pos2, __n2));
  }
  constexpr basic_string& replace(size_type __pos1, size_type __n1, const __charT* s, size_type __n2) {
    __ycxx::__detail::__precondition(s != nullptr || __n2 == 0, "std::basic_string::replace: null pointer");
    __check_pos(__pos1, "std::basic_string::replace: pos > size()");
    return __replace_impl(__pos1, clamp(__pos1, __n1), s, __n2);
  }
  constexpr basic_string& replace(size_type __pos, size_type __n1, const __charT* s) {
    __ycxx::__detail::__precondition(s != nullptr, "std::basic_string::replace: null pointer");
    return replace(__pos, __n1, s, __traits::length(s));
  }
  constexpr basic_string& replace(size_type __pos1, size_type __n1, size_type __n2, __charT c) {
    __check_pos(__pos1, "std::basic_string::replace: pos > size()");
    return __replace_fill(__pos1, clamp(__pos1, __n1), __n2, c);
  }
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, const basic_string& str) {
    return replace(__i1, __i2, __sv_type(str));
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, const _Tp& t) {
    const __sv_type sv = t;
    return __replace_impl(static_cast<size_type>(__i1 - cbegin()), static_cast<size_type>(__i2 - __i1), sv.data(), sv.size());
  }
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, const __charT* s, size_type n) {
    return replace(__i1, __i2, __sv_type(s, n));
  }
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, const __charT* s) {
    return replace(__i1, __i2, __sv_type(s));
  }
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, size_type n, __charT c) {
    return __replace_fill(static_cast<size_type>(__i1 - cbegin()), static_cast<size_type>(__i2 - __i1), n, c);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, _InputIterator __j1, _InputIterator __j2) {
    const size_type __pos = static_cast<size_type>(__i1 - cbegin());
    const size_type __n1 = static_cast<size_type>(__i2 - __i1);
    if constexpr (__char_ptr_iter<_InputIterator>) {
      return __replace_impl(__pos, __n1, std::to_address(__j1), static_cast<size_type>(__j2 - __j1));
    } else {
      const basic_string t = __temp_of(__j1, __j2);
      return __replace_impl(__pos, __n1, t.__ptr_, t.__size_);
    }
  }
  template <__ycxx::__detail::__container_compatible_range<__charT> _Rp>
  constexpr basic_string& replace_with_range(const_iterator __i1, const_iterator __i2, _Rp&& __rg) {
    const size_type __pos = static_cast<size_type>(__i1 - cbegin());
    const size_type __n1 = static_cast<size_type>(__i2 - __i1);
    if constexpr (__char_contiguous_range<_Rp>) {
      return __replace_impl(__pos, __n1, ranges::data(__rg), static_cast<size_type>(ranges::size(__rg)));
    } else {
      const basic_string t = __temp_of(ranges::begin(__rg), ranges::end(__rg));
      return __replace_impl(__pos, __n1, t.__ptr_, t.__size_);
    }
  }
  constexpr basic_string& replace(const_iterator __i1, const_iterator __i2, initializer_list<__charT> il) {
    return replace(__i1, __i2, il.begin(), il.size());
  }

  // ---- [string.copy], [string.swap] ----
  constexpr size_type copy(__charT* s, size_type n, size_type __pos = 0) const {
    return static_cast<size_type>(__sv_type(*this).copy(s, n, __pos));
  }
  constexpr void swap(basic_string& s) noexcept(__pocs || __always_equal) {
    if (this == __builtin_addressof(s))
      return;
    if constexpr (__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__alloc_, s.__alloc_);
    else
      __ycxx::__detail::__precondition(__always_equal || __alloc_ == s.__alloc_,
                                 "std::basic_string::swap: unequal allocators that do not propagate");
    const bool __l1 = __is_long(), __l2 = s.__is_long();
    if (__l1 && __l2) {
      __charT* const p = __ptr_;
      const size_type c = __cap_;
      __set_long(s.__ptr_, s.__cap_);
      s.__set_long(p, c);
    } else if (!__l1 && !__l2) {
      __charT __tmp[__buf_len];
      __traits::copy(__tmp, __buf_, __size_ + 1);
      __traits::copy(__buf_, s.__buf_, s.__size_ + 1);
      __traits::copy(s.__buf_, __tmp, __size_ + 1);
    } else {
      basic_string& __lng = __l1 ? *this : s;
      basic_string& __shrt = __l1 ? s : *this;
      __charT __tmp[__buf_len];
      __traits::copy(__tmp, __shrt.__buf_, __shrt.__size_ + 1);
      __shrt.__set_long(__lng.__ptr_, __lng.__cap_);
      __lng.__activate_buf();
      __traits::copy(__lng.__buf_, __tmp, __shrt.__size_ + 1);
    }
    const size_type n = __size_;
    __size_ = s.__size_;
    s.__size_ = n;
  }

  // ---- [string.accessors] ----
  constexpr const __charT* c_str() const noexcept { return __ptr_; }
  constexpr const __charT* data() const noexcept { return __ptr_; }
  constexpr __charT* data() noexcept { return __ptr_; }
  constexpr operator basic_string_view<__charT, __traits>() const noexcept { return __sv_type(__ptr_, __size_); }
  constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

  // ---- [string.find] ----
private:
  static constexpr size_type __to_npos(size_t r) noexcept { return r == __sv_type::npos ? npos : static_cast<size_type>(r); }

public:
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type find(const _Tp& t, size_type __pos = 0) const noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).find(sv, __pos));
  }
  constexpr size_type find(const basic_string& str, size_type __pos = 0) const noexcept {
    return __to_npos(__sv_type(*this).find(__sv_type(str), __pos));
  }
  constexpr size_type find(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).find(__sv_type(s, n), __pos));
  }
  constexpr size_type find(const __charT* s, size_type __pos = 0) const {
    return __to_npos(__sv_type(*this).find(__sv_type(s), __pos));
  }
  constexpr size_type find(__charT c, size_type __pos = 0) const noexcept { return __to_npos(__sv_type(*this).find(c, __pos)); }

  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type rfind(const _Tp& t, size_type __pos = npos) const noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).rfind(sv, __pos));
  }
  constexpr size_type rfind(const basic_string& str, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).rfind(__sv_type(str), __pos));
  }
  constexpr size_type rfind(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).rfind(__sv_type(s, n), __pos));
  }
  constexpr size_type rfind(const __charT* s, size_type __pos = npos) const {
    return __to_npos(__sv_type(*this).rfind(__sv_type(s), __pos));
  }
  constexpr size_type rfind(__charT c, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).rfind(c, __pos));
  }

  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type find_first_of(const _Tp& t, size_type __pos = 0) const
      noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).find_first_of(sv, __pos));
  }
  constexpr size_type find_first_of(const basic_string& str, size_type __pos = 0) const noexcept {
    return __to_npos(__sv_type(*this).find_first_of(__sv_type(str), __pos));
  }
  constexpr size_type find_first_of(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).find_first_of(__sv_type(s, n), __pos));
  }
  constexpr size_type find_first_of(const __charT* s, size_type __pos = 0) const {
    return __to_npos(__sv_type(*this).find_first_of(__sv_type(s), __pos));
  }
  constexpr size_type find_first_of(__charT c, size_type __pos = 0) const noexcept {
    return __to_npos(__sv_type(*this).find_first_of(c, __pos));
  }

  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type find_last_of(const _Tp& t, size_type __pos = npos) const
      noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).find_last_of(sv, __pos));
  }
  constexpr size_type find_last_of(const basic_string& str, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).find_last_of(__sv_type(str), __pos));
  }
  constexpr size_type find_last_of(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).find_last_of(__sv_type(s, n), __pos));
  }
  constexpr size_type find_last_of(const __charT* s, size_type __pos = npos) const {
    return __to_npos(__sv_type(*this).find_last_of(__sv_type(s), __pos));
  }
  constexpr size_type find_last_of(__charT c, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).find_last_of(c, __pos));
  }

  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type find_first_not_of(const _Tp& t, size_type __pos = 0) const
      noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).find_first_not_of(sv, __pos));
  }
  constexpr size_type find_first_not_of(const basic_string& str, size_type __pos = 0) const noexcept {
    return __to_npos(__sv_type(*this).find_first_not_of(__sv_type(str), __pos));
  }
  constexpr size_type find_first_not_of(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).find_first_not_of(__sv_type(s, n), __pos));
  }
  constexpr size_type find_first_not_of(const __charT* s, size_type __pos = 0) const {
    return __to_npos(__sv_type(*this).find_first_not_of(__sv_type(s), __pos));
  }
  constexpr size_type find_first_not_of(__charT c, size_type __pos = 0) const noexcept {
    return __to_npos(__sv_type(*this).find_first_not_of(c, __pos));
  }

  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr size_type find_last_not_of(const _Tp& t, size_type __pos = npos) const
      noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    const __sv_type sv = t;
    return __to_npos(__sv_type(*this).find_last_not_of(sv, __pos));
  }
  constexpr size_type find_last_not_of(const basic_string& str, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).find_last_not_of(__sv_type(str), __pos));
  }
  constexpr size_type find_last_not_of(const __charT* s, size_type __pos, size_type n) const {
    return __to_npos(__sv_type(*this).find_last_not_of(__sv_type(s, n), __pos));
  }
  constexpr size_type find_last_not_of(const __charT* s, size_type __pos = npos) const {
    return __to_npos(__sv_type(*this).find_last_not_of(__sv_type(s), __pos));
  }
  constexpr size_type find_last_not_of(__charT c, size_type __pos = npos) const noexcept {
    return __to_npos(__sv_type(*this).find_last_not_of(c, __pos));
  }

  // ---- [string.substr] ----
  constexpr basic_string substr(size_type __pos = 0, size_type n = npos) const& { return basic_string(*this, __pos, n); }
  constexpr basic_string substr(size_type __pos = 0, size_type n = npos) && {
    return basic_string(static_cast<basic_string&&>(*this), __pos, n);
  }
  constexpr basic_string_view<__charT, __traits> subview(size_type __pos = 0, size_type n = npos) const {
    return __sv_type(*this).subview(__pos, n);
  }

  // ---- [string.compare] ----
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr int compare(const _Tp& t) const noexcept(is_nothrow_convertible_v<const _Tp&, __sv_type>) {
    return __sv_type(*this).compare(t);
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr int compare(size_type __pos1, size_type __n1, const _Tp& t) const {
    return __sv_type(*this).substr(__pos1, __n1).compare(t);
  }
  template <class _Tp>
    requires __sv_like<_Tp>
  constexpr int compare(size_type __pos1, size_type __n1, const _Tp& t, size_type __pos2, size_type __n2 = npos) const {
    const __sv_type s = *this, sv = t;
    return s.substr(__pos1, __n1).compare(sv.substr(__pos2, __n2));
  }
  constexpr int compare(const basic_string& str) const noexcept { return __sv_type(*this).compare(__sv_type(str)); }
  constexpr int compare(size_type __pos1, size_type __n1, const basic_string& str) const {
    return __sv_type(*this).substr(__pos1, __n1).compare(__sv_type(str));
  }
  constexpr int compare(size_type __pos1, size_type __n1, const basic_string& str, size_type __pos2,
                        size_type __n2 = npos) const {
    return __sv_type(*this).substr(__pos1, __n1).compare(__sv_type(str).substr(__pos2, __n2));
  }
  constexpr int compare(const __charT* s) const { return __sv_type(*this).compare(__sv_type(s)); }
  constexpr int compare(size_type __pos1, size_type __n1, const __charT* s) const {
    return __sv_type(*this).substr(__pos1, __n1).compare(__sv_type(s));
  }
  constexpr int compare(size_type __pos1, size_type __n1, const __charT* s, size_type __n2) const {
    return __sv_type(*this).substr(__pos1, __n1).compare(__sv_type(s, __n2));
  }

  // ---- [string.starts.with], [string.ends.with], [string.contains] ----
  constexpr bool starts_with(basic_string_view<__charT, __traits> __x) const noexcept { return __sv_type(*this).starts_with(__x); }
  constexpr bool starts_with(__charT __x) const noexcept { return __sv_type(*this).starts_with(__x); }
  constexpr bool starts_with(const __charT* __x) const { return __sv_type(*this).starts_with(__x); }
  constexpr bool ends_with(basic_string_view<__charT, __traits> __x) const noexcept { return __sv_type(*this).ends_with(__x); }
  constexpr bool ends_with(__charT __x) const noexcept { return __sv_type(*this).ends_with(__x); }
  constexpr bool ends_with(const __charT* __x) const { return __sv_type(*this).ends_with(__x); }
  constexpr bool contains(basic_string_view<__charT, __traits> __x) const noexcept { return __sv_type(*this).contains(__x); }
  constexpr bool contains(__charT __x) const noexcept { return __sv_type(*this).contains(__x); }
  constexpr bool contains(const __charT* __x) const { return __sv_type(*this).contains(__x); }
};

// ---- deduction guides ([string.cons]) ----
template <class _InputIterator, class _Allocator = allocator<typename iterator_traits<_InputIterator>::value_type>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
basic_string(_InputIterator, _InputIterator, _Allocator = _Allocator())
    -> basic_string<typename iterator_traits<_InputIterator>::value_type,
                    char_traits<typename iterator_traits<_InputIterator>::value_type>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
basic_string(from_range_t, _Rp&&, _Allocator = _Allocator())
    -> basic_string<ranges::range_value_t<_Rp>, char_traits<ranges::range_value_t<_Rp>>, _Allocator>;
template <class __charT, class __traits, class _Allocator = allocator<__charT>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
explicit basic_string(basic_string_view<__charT, __traits>, const _Allocator& = _Allocator())
    -> basic_string<__charT, __traits, _Allocator>;
template <class __charT, class __traits, class _Allocator = allocator<__charT>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
basic_string(basic_string_view<__charT, __traits>, typename basic_string<__charT, __traits, _Allocator>::size_type,
             typename basic_string<__charT, __traits, _Allocator>::size_type, const _Allocator& = _Allocator())
    -> basic_string<__charT, __traits, _Allocator>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// lhs + rhs as a new string with allocator a ([string.op.plus]: a copy of one operand, then an
// append or insert), sized once.
template <class _Sp>
constexpr _Sp __string_concat(const typename _Sp::allocator_type& a, const typename _Sp::value_type* __l,
                          typename _Sp::size_type __nl, const typename _Sp::value_type* r, typename _Sp::size_type __nr) {
  _Sp s(a);
  if (__nr > s.max_size() || __nl > s.max_size() - __nr)
    ::__ycxx::__detail::__throw_length_error("std::operator+: length exceeds max_size()");
  s.reserve(__nl + __nr);
  s.append(__l, __nl);
  s.append(r, __nr);
  return s;
}
template <class _Sp>
constexpr typename _Sp::allocator_type __copy_alloc(const _Sp& s) {
  return std::allocator_traits<typename _Sp::allocator_type>::select_on_container_copy_construction(s.get_allocator());
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [string.op.plus] ----
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const basic_string<__charT, __traits, _Allocator>& __lhs,
                                                           const basic_string<__charT, __traits, _Allocator>& __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__lhs), __lhs.data(), __lhs.size(), __rhs.data(), __rhs.size());
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(basic_string<__charT, __traits, _Allocator>&& __lhs,
                                                           const basic_string<__charT, __traits, _Allocator>& __rhs) {
  __lhs.append(__rhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__lhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const basic_string<__charT, __traits, _Allocator>& __lhs,
                                                           basic_string<__charT, __traits, _Allocator>&& __rhs) {
  __rhs.insert(0, __lhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(basic_string<__charT, __traits, _Allocator>&& __lhs,
                                                           basic_string<__charT, __traits, _Allocator>&& __rhs) {
  // Note 1: with equal allocators either operand's storage may be reused; take rhs's when only
  // it has room for the result.
  if (__lhs.capacity() - __lhs.size() < __rhs.size() && __rhs.capacity() - __rhs.size() >= __lhs.size() &&
      __lhs.get_allocator() == __rhs.get_allocator()) {
    __rhs.insert(0, __lhs);
    return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__rhs);
  }
  __lhs.append(__rhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__lhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const __charT* __lhs,
                                                           const basic_string<__charT, __traits, _Allocator>& __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__rhs), __lhs,
                                        static_cast<typename _Sp::size_type>(__traits::length(__lhs)), __rhs.data(), __rhs.size());
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const __charT* __lhs, basic_string<__charT, __traits, _Allocator>&& __rhs) {
  __rhs.insert(0, __lhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(__charT __lhs, const basic_string<__charT, __traits, _Allocator>& __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__rhs), __builtin_addressof(__lhs), 1, __rhs.data(), __rhs.size());
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(__charT __lhs, basic_string<__charT, __traits, _Allocator>&& __rhs) {
  __rhs.insert(__rhs.begin(), __lhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const basic_string<__charT, __traits, _Allocator>& __lhs,
                                                           const __charT* __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__lhs), __lhs.data(), __lhs.size(), __rhs,
                                        static_cast<typename _Sp::size_type>(__traits::length(__rhs)));
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(basic_string<__charT, __traits, _Allocator>&& __lhs, const __charT* __rhs) {
  __lhs.append(__rhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__lhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const basic_string<__charT, __traits, _Allocator>& __lhs, __charT __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__lhs), __lhs.data(), __lhs.size(), __builtin_addressof(__rhs), 1);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(basic_string<__charT, __traits, _Allocator>&& __lhs, __charT __rhs) {
  __lhs.push_back(__rhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__lhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(const basic_string<__charT, __traits, _Allocator>& __lhs,
                                                           type_identity_t<basic_string_view<__charT, __traits>> __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__lhs), __lhs.data(), __lhs.size(), __rhs.data(),
                                        static_cast<typename _Sp::size_type>(__rhs.size()));
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(basic_string<__charT, __traits, _Allocator>&& __lhs,
                                                           type_identity_t<basic_string_view<__charT, __traits>> __rhs) {
  __lhs.append(__rhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__lhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(type_identity_t<basic_string_view<__charT, __traits>> __lhs,
                                                           const basic_string<__charT, __traits, _Allocator>& __rhs) {
  using _Sp = basic_string<__charT, __traits, _Allocator>;
  return __ycxx::__detail::__string_concat<_Sp>(__ycxx::__detail::__copy_alloc(__rhs), __lhs.data(),
                                        static_cast<typename _Sp::size_type>(__lhs.size()), __rhs.data(), __rhs.size());
}
template <class __charT, class __traits, class _Allocator>
constexpr basic_string<__charT, __traits, _Allocator> operator+(type_identity_t<basic_string_view<__charT, __traits>> __lhs,
                                                           basic_string<__charT, __traits, _Allocator>&& __rhs) {
  __rhs.insert(0, __lhs);
  return static_cast<basic_string<__charT, __traits, _Allocator>&&>(__rhs);
}

// ---- [string.cmp] ----
template <class __charT, class __traits, class _Allocator>
constexpr bool operator==(const basic_string<__charT, __traits, _Allocator>& __lhs,
                          const basic_string<__charT, __traits, _Allocator>& __rhs) noexcept {
  return basic_string_view<__charT, __traits>(__lhs) == basic_string_view<__charT, __traits>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr bool operator==(const basic_string<__charT, __traits, _Allocator>& __lhs, const __charT* __rhs) {
  return basic_string_view<__charT, __traits>(__lhs) == basic_string_view<__charT, __traits>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr auto operator<=>(const basic_string<__charT, __traits, _Allocator>& __lhs,
                           const basic_string<__charT, __traits, _Allocator>& __rhs) noexcept
    -> decltype(basic_string_view<__charT, __traits>(__lhs) <=> basic_string_view<__charT, __traits>(__rhs)) {
  return basic_string_view<__charT, __traits>(__lhs) <=> basic_string_view<__charT, __traits>(__rhs);
}
template <class __charT, class __traits, class _Allocator>
constexpr auto operator<=>(const basic_string<__charT, __traits, _Allocator>& __lhs, const __charT* __rhs)
    -> decltype(basic_string_view<__charT, __traits>(__lhs) <=> basic_string_view<__charT, __traits>(__rhs)) {
  return basic_string_view<__charT, __traits>(__lhs) <=> basic_string_view<__charT, __traits>(__rhs);
}

// ---- [string.special] ----
template <class __charT, class __traits, class _Allocator>
constexpr void swap(basic_string<__charT, __traits, _Allocator>& __lhs,
                    basic_string<__charT, __traits, _Allocator>& __rhs) noexcept(noexcept(__lhs.swap(__rhs))) {
  __lhs.swap(__rhs);
}

// ---- [string.erasure] ----
template <class __charT, class __traits, class _Allocator, class _Predicate>
constexpr typename basic_string<__charT, __traits, _Allocator>::size_type erase_if(basic_string<__charT, __traits, _Allocator>& c,
                                                                              _Predicate pred) {
  // remove_if, then erase the tail.
  auto first = c.begin();
  const auto last = c.end();
  while (first != last && !bool(pred(*first)))
    ++first;
  auto out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!bool(pred(*first)))
        *out++ = static_cast<__charT&&>(*first);
  }
  const auto r = static_cast<typename basic_string<__charT, __traits, _Allocator>::size_type>(last - out);
  c.erase(out, last);
  return r;
}
template <class __charT, class __traits, class _Allocator, class _Up = __charT>
constexpr typename basic_string<__charT, __traits, _Allocator>::size_type erase(basic_string<__charT, __traits, _Allocator>& c,
                                                                           const _Up& value) {
  return std::erase_if(c, [&value](const __charT& e) { return bool(e == value); });
}

// ---- typedef-names ----
using string = basic_string<char>;
using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
using wstring = basic_string<wchar_t>;

namespace pmr {
template <class __charT, class __traits = char_traits<__charT>>
using basic_string = std::basic_string<__charT, __traits, polymorphic_allocator<__charT>>;
using string = basic_string<char>;
using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
using wstring = basic_string<wchar_t>;
} // namespace pmr

} // namespace std

// ---- [basic.string.hash] ----
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// hash<S>()(s) == hash<SV>()(SV(s)) for the five standard string types.
template <class _Sp>
struct __string_hash {
  [[nodiscard]] std::size_t operator()(const _Sp& s) const noexcept {
    return std::hash<std::basic_string_view<typename _Sp::value_type>>()(
        std::basic_string_view<typename _Sp::value_type>(s.data(), s.size()));
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ap>
struct hash<basic_string<char, char_traits<char>, _Ap>>
    : __ycxx::__adl_free::__string_hash<basic_string<char, char_traits<char>, _Ap>> {};
template <class _Ap>
struct hash<basic_string<char8_t, char_traits<char8_t>, _Ap>>
    : __ycxx::__adl_free::__string_hash<basic_string<char8_t, char_traits<char8_t>, _Ap>> {};
template <class _Ap>
struct hash<basic_string<char16_t, char_traits<char16_t>, _Ap>>
    : __ycxx::__adl_free::__string_hash<basic_string<char16_t, char_traits<char16_t>, _Ap>> {};
template <class _Ap>
struct hash<basic_string<char32_t, char_traits<char32_t>, _Ap>>
    : __ycxx::__adl_free::__string_hash<basic_string<char32_t, char_traits<char32_t>, _Ap>> {};
template <class _Ap>
struct hash<basic_string<wchar_t, char_traits<wchar_t>, _Ap>>
    : __ycxx::__adl_free::__string_hash<basic_string<wchar_t, char_traits<wchar_t>, _Ap>> {};

// ---- [basic.string.literals] ----
inline namespace literals {
inline namespace string_literals {
constexpr string operator""s(const char* str, size_t __len) { return string(str, __len); }
constexpr u8string operator""s(const char8_t* str, size_t __len) { return u8string(str, __len); }
constexpr u16string operator""s(const char16_t* str, size_t __len) { return u16string(str, __len); }
constexpr u32string operator""s(const char32_t* str, size_t __len) { return u32string(str, __len); }
constexpr wstring operator""s(const wchar_t* str, size_t __len) { return wstring(str, __len); }
} // namespace string_literals
} // namespace literals

} // namespace std

// ---- [string.conversions] ----
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// format("{}", v) for an integer: decimal digits, a leading '-' for negative values.
template <class __charT, class _Tp>
constexpr std::basic_string<__charT> __integer_to_string(_Tp __v) {
  using _Up = std::make_unsigned_t<_Tp>;
  _Up __u = static_cast<_Up>(__v);
  bool __neg = false;
  if constexpr (std::is_signed_v<_Tp>) {
    if (__v < 0) {
      __neg = true;
      __u = static_cast<_Up>(_Up(0) - __u);
    }
  }
  [[indeterminate]] __charT __buf[std::numeric_limits<_Up>::digits10 + 2];
  __charT* const end = __buf + sizeof(__buf) / sizeof(__charT);
  __charT* p = end;
  // Two digits per division.
  static constexpr auto __pairs = [] {
    struct table {
      char c[200];
    } t{};
    for (int i = 0; i < 100; ++i) {
      t.c[2 * i] = static_cast<char>('0' + i / 10);
      t.c[2 * i + 1] = static_cast<char>('0' + i % 10);
    }
    return t;
  }();
  while (__u >= 100) {
    const auto r = static_cast<unsigned>(__u % 100);
    __u /= 100;
    p -= 2;
    p[0] = static_cast<__charT>(__pairs.c[2 * r]);
    p[1] = static_cast<__charT>(__pairs.c[2 * r + 1]);
  }
  if (__u >= 10) {
    p -= 2;
    p[0] = static_cast<__charT>(__pairs.c[2 * __u]);
    p[1] = static_cast<__charT>(__pairs.c[2 * __u + 1]);
  } else {
    *--p = static_cast<__charT>('0' + static_cast<int>(__u));
  }
  if (__neg)
    *--p = static_cast<__charT>('-');
  return std::basic_string<__charT>(p, static_cast<std::size_t>(end - p));
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// Defined in the hosted runtime (src/hosted/string.cpp): they call the C library.
int stoi(const string& str, size_t* __idx = nullptr, int base = 10);
long stol(const string& str, size_t* __idx = nullptr, int base = 10);
unsigned long stoul(const string& str, size_t* __idx = nullptr, int base = 10);
long long stoll(const string& str, size_t* __idx = nullptr, int base = 10);
unsigned long long stoull(const string& str, size_t* __idx = nullptr, int base = 10);
float stof(const string& str, size_t* __idx = nullptr);
double stod(const string& str, size_t* __idx = nullptr);
long double stold(const string& str, size_t* __idx = nullptr);
int stoi(const wstring& str, size_t* __idx = nullptr, int base = 10);
long stol(const wstring& str, size_t* __idx = nullptr, int base = 10);
unsigned long stoul(const wstring& str, size_t* __idx = nullptr, int base = 10);
long long stoll(const wstring& str, size_t* __idx = nullptr, int base = 10);
unsigned long long stoull(const wstring& str, size_t* __idx = nullptr, int base = 10);
float stof(const wstring& str, size_t* __idx = nullptr);
double stod(const wstring& str, size_t* __idx = nullptr);
long double stold(const wstring& str, size_t* __idx = nullptr);
string to_string(float __val);
string to_string(double __val);
string to_string(long double __val);
wstring to_wstring(float __val);
wstring to_wstring(double __val);
wstring to_wstring(long double __val);

constexpr string to_string(int __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr string to_string(unsigned __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr string to_string(long __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr string to_string(unsigned long __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr string to_string(long long __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr string to_string(unsigned long long __val) { return __ycxx::__detail::__integer_to_string<char>(__val); }
constexpr wstring to_wstring(int __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }
constexpr wstring to_wstring(unsigned __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }
constexpr wstring to_wstring(long __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }
constexpr wstring to_wstring(unsigned long __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }
constexpr wstring to_wstring(long long __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }
constexpr wstring to_wstring(unsigned long long __val) { return __ycxx::__detail::__integer_to_string<wchar_t>(__val); }

// [string.io]: declared against the iostreams' forward declarations; defined with the streams
// (ycxx/hosted/istream.hpp, ycxx/hosted/ostream.hpp), so <string> does not include them.
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str);
template <class __charT, class __traits, class _Allocator>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                         const basic_string<__charT, __traits, _Allocator>& str);
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str,
                                      __charT __delim);
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>&& is, basic_string<__charT, __traits, _Allocator>& str,
                                      __charT __delim);
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>& is, basic_string<__charT, __traits, _Allocator>& str);
template <class __charT, class __traits, class _Allocator>
basic_istream<__charT, __traits>& getline(basic_istream<__charT, __traits>&& is, basic_string<__charT, __traits, _Allocator>& str);

} // namespace std

// The <stdexcept> constructors taking `const string&`, now that string is complete.
#include <ycxx/core/stdexcept_string.hpp>
