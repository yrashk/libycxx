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

namespace std {

template <class charT, class traits = char_traits<charT>, class Allocator = allocator<charT>>
class basic_string;

template <class charT, class traits, class Allocator>
class basic_string {
  static_assert(is_same_v<typename traits::char_type, charT>,
                "std::basic_string: traits::char_type must be charT ([string.require])");
  static_assert(is_same_v<typename Allocator::value_type, charT>,
                "std::basic_string: Allocator::value_type must be charT ([string.require])");
  static_assert(!is_array_v<charT> && is_trivially_copyable_v<charT> && is_trivially_default_constructible_v<charT> &&
                    is_standard_layout_v<charT>,
                "std::basic_string: charT must be a char-like type");

  using alloc_traits = allocator_traits<Allocator>;
  using sv_type = basic_string_view<charT, traits>;

public:
  // ---- types ----
  using traits_type = traits;
  using value_type = charT;
  using allocator_type = Allocator;
  using size_type = typename alloc_traits::size_type;
  using difference_type = typename alloc_traits::difference_type;
  using pointer = typename alloc_traits::pointer;
  using const_pointer = typename alloc_traits::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = ycxx::adl_free::contiguous_iter<charT, basic_string, difference_type>;
  using const_iterator = ycxx::adl_free::contiguous_iter<const charT, basic_string, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  static constexpr size_type npos = size_type(-1);

private:
  // T is "string-view-like": the constraint of most string_view-taking members. Extension:
  // classes derived from basic_string are excluded, so that they bind to the basic_string
  // overloads (a derived rvalue is then moved from, not copied through a string_view).
  template <class T>
  static constexpr bool sv_like = is_convertible_v<const T&, sv_type> && !is_convertible_v<const T&, const charT*> &&
                                  !is_convertible_v<const T*, const basic_string*>;
  // An iterator whose elements can be read through a charT pointer.
  template <class It>
  static constexpr bool char_ptr_iter = contiguous_iterator<It> && is_same_v<iter_value_t<It>, charT>;

  static constexpr bool pocca = alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool pocma = alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool pocs = alloc_traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = alloc_traits::is_always_equal::value;

  // Inline buffer: 16 bytes' worth of characters (at least one, for the terminator).
  static constexpr size_t buf_len = 16 / sizeof(charT) > 1 ? 16 / sizeof(charT) : 1;
  static constexpr size_type sso_cap = buf_len - 1;

  charT* ptr_;
  size_type size_;
  union {
    size_type cap_;
    charT buf_[buf_len];
  };
  [[no_unique_address]] Allocator alloc_;

  // ---- representation helpers ----
  constexpr bool is_long() const noexcept { return ptr_ != buf_; }
  constexpr size_type cap() const noexcept { return is_long() ? cap_ : sso_cap; }

  // Makes buf_ the active union member (ending cap_'s lifetime) and points ptr_ at it. During
  // constant evaluation every element is initialized: a constant-initialized string must not
  // hold indeterminate values.
  constexpr void activate_buf() noexcept {
    buf_[0] = charT();
    if consteval {
      for (size_t i = 1; i < buf_len; ++i)
        buf_[i] = charT();
    }
    ptr_ = buf_;
  }
  // Makes *this an empty short string. Any long storage must already have been released (or
  // taken over).
  constexpr void set_short_empty() noexcept {
    activate_buf();
    size_ = 0;
  }
  constexpr void set_long(charT* p, size_type c) noexcept {
    ptr_ = p;
    cap_ = c;
  }

  struct block {
    charT* p;
    size_type cap; // characters, excluding the terminator
  };
  // Storage for at least n characters plus the terminator.
  static constexpr block allocate_block(Allocator& a, size_type n) {
    auto r = alloc_traits::allocate_at_least(a, n + 1);
    charT* p = std::to_address(r.ptr);
    if consteval {
      for (size_type i = 0; i < r.count; ++i)
        std::construct_at(p + i);
    }
    return {p, static_cast<size_type>(r.count - 1)};
  }
  static constexpr void deallocate_block(Allocator& a, charT* p, size_type c) noexcept {
    alloc_traits::deallocate(a, pointer_traits<pointer>::pointer_to(*p), c + 1);
  }
  constexpr void free_storage() noexcept {
    if (is_long())
      deallocate_block(alloc_, ptr_, cap_);
  }

  constexpr void check_length(size_type n, const char* what) const {
    if (n > max_size())
      ycxx::detail::throw_length_error(what);
  }
  constexpr void check_pos(size_type pos, const char* what) const {
    if (pos > size_)
      ycxx::detail::throw_out_of_range(what);
  }
  constexpr size_type clamp(size_type pos, size_type n) const noexcept {
    return n < size_ - pos ? n : size_ - pos;
  }

  // Capacity for a string that must grow to new_size (> cap()): geometric growth.
  constexpr size_type grow_cap(size_type new_size) const {
    const size_type ms = max_size();
    if (new_size > ms)
      ycxx::detail::throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type c = cap();
    size_type nc = c <= ms / 2 ? 2 * c : ms;
    return nc < new_size ? new_size : nc;
  }

  // Initialisation of a freshly constructed object (no storage owned yet).
  constexpr void init_copy(const charT* s, size_type n) {
    if (n <= sso_cap) {
      activate_buf();
    } else {
      check_length(n, "std::basic_string: length exceeds max_size()");
      block b = allocate_block(alloc_, n);
      set_long(b.p, b.cap);
    }
    traits::copy(ptr_, s, n);
    traits::assign(ptr_[n], charT());
    size_ = n;
  }
  constexpr void init_fill(size_type n, charT c) {
    if (n <= sso_cap) {
      activate_buf();
    } else {
      check_length(n, "std::basic_string: length exceeds max_size()");
      block b = allocate_block(alloc_, n);
      set_long(b.p, b.cap);
    }
    traits::assign(ptr_, n, c);
    traits::assign(ptr_[n], charT());
    size_ = n;
  }
  // Takes over o's characters (and storage, if long); o becomes empty. *this owns nothing.
  constexpr void take(basic_string& o) noexcept {
    if (o.is_long()) {
      set_long(o.ptr_, o.cap_);
      size_ = o.size_;
    } else {
      activate_buf();
      traits::copy(buf_, o.buf_, o.size_ + 1);
      size_ = o.size_;
    }
    o.set_short_empty();
  }

  // Moves the characters into a new block of capacity at least c (>= size_).
  constexpr void reallocate(size_type c) {
    block b = allocate_block(alloc_, c);
    traits::copy(b.p, ptr_, size_ + 1);
    free_storage();
    set_long(b.p, b.cap);
  }

  // Index of s within [data(), data() + size()], or npos. Only equality comparisons are made
  // during constant evaluation, where pointers into unrelated objects cannot be ordered.
  constexpr size_type offset_of(const charT* s) const noexcept {
    if consteval {
      for (size_type i = 0; i <= size_; ++i)
        if (ptr_ + i == s)
          return i;
      return npos;
    } else {
      const auto a = reinterpret_cast<__UINTPTR_TYPE__>(s);
      const auto b = reinterpret_cast<__UINTPTR_TYPE__>(ptr_);
      if (a < b || a > b + size_ * sizeof(charT))
        return npos;
      return static_cast<size_type>((a - b) / sizeof(charT));
    }
  }

  // Replaces [pos, pos + n1) with [s, s + n2); pos <= size_, n1 <= size_ - pos. The source may
  // lie inside *this. Strong guarantee: everything that can throw happens before any change.
  constexpr basic_string& replace_impl(size_type pos, size_type n1, const charT* s, size_type n2) {
    const size_type sz = size_;
    if (n2 > n1 && n2 - n1 > max_size() - sz)
      ycxx::detail::throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type new_size = sz - n1 + n2;
    const size_type tail = sz - pos - n1;
    if (new_size > cap()) {
      // The old characters stay intact until the new block is complete, so s may alias them.
      block b = allocate_block(alloc_, grow_cap(new_size));
      traits::copy(b.p, ptr_, pos);
      traits::copy(b.p + pos, s, n2);
      traits::copy(b.p + pos + n2, ptr_ + pos + n1, tail);
      traits::assign(b.p[new_size], charT());
      free_storage();
      set_long(b.p, b.cap);
      size_ = new_size;
      return *this;
    }
    charT* const p = ptr_;
    const size_type off = n2 == 0 ? npos : offset_of(s);
    if (off == npos) {
      if (n1 != n2)
        traits::move(p + pos + n2, p + pos + n1, tail);
      traits::copy(p + pos, s, n2);
    } else if (n1 == n2) {
      traits::move(p + pos, p + off, n2);
    } else if (n2 < n1) {
      // Shrinking: place the source first (it ends up inside the replaced range), then close
      // the gap.
      traits::move(p + pos, p + off, n2);
      traits::move(p + pos + n2, p + pos + n1, tail);
    } else {
      // Growing: open the gap first (moving the terminator too, which the source may include).
      // Source characters at or after pos + n1 move by n2 - n1.
      traits::move(p + pos + n2, p + pos + n1, tail + 1);
      if (off + n2 <= pos + n1) {
        traits::move(p + pos, p + off, n2);
      } else if (off >= pos + n1) {
        traits::copy(p + pos, p + off + (n2 - n1), n2);
      } else {
        const size_type k = pos + n1 - off; // unmoved prefix of the source
        traits::move(p + pos, p + off, k);
        traits::copy(p + pos + k, p + pos + n2, n2 - k);
      }
    }
    traits::assign(p[new_size], charT());
    size_ = new_size;
    return *this;
  }

  // Replaces [pos, pos + n1) with n2 copies of c (same preconditions as replace_impl).
  constexpr basic_string& replace_fill(size_type pos, size_type n1, size_type n2, charT c) {
    const size_type sz = size_;
    if (n2 > n1 && n2 - n1 > max_size() - sz)
      ycxx::detail::throw_length_error("std::basic_string: length exceeds max_size()");
    const size_type new_size = sz - n1 + n2;
    const size_type tail = sz - pos - n1;
    if (new_size > cap()) {
      block b = allocate_block(alloc_, grow_cap(new_size));
      traits::copy(b.p, ptr_, pos);
      traits::assign(b.p + pos, n2, c);
      traits::copy(b.p + pos + n2, ptr_ + pos + n1, tail);
      traits::assign(b.p[new_size], charT());
      free_storage();
      set_long(b.p, b.cap);
    } else {
      if (n1 != n2)
        traits::move(ptr_ + pos + n2, ptr_ + pos + n1, tail);
      traits::assign(ptr_ + pos, n2, c);
      traits::assign(ptr_[new_size], charT());
    }
    size_ = new_size;
    return *this;
  }

  constexpr void erase_impl(size_type pos, size_type n) noexcept {
    if (n == 0)
      return;
    traits::move(ptr_ + pos, ptr_ + pos + n, size_ - pos - n);
    size_ -= n;
    traits::assign(ptr_[size_], charT());
  }

  // Appends [first, last) to *this element by element; the elements must not live in *this.
  template <class It, class Sent>
  constexpr void append_elements(It first, Sent last) {
    if constexpr (forward_iterator<It>) {
      const auto d = ranges::distance(first, last);
      if (static_cast<make_unsigned_t<decltype(d)>>(d) > max_size() - size_)
        ycxx::detail::throw_length_error("std::basic_string: length exceeds max_size()");
      const size_type n = static_cast<size_type>(d);
      if (size_ + n > cap())
        reallocate(grow_cap(size_ + n));
      charT* p = ptr_ + size_;
      for (; first != last; ++first, ++p)
        traits::assign(*p, static_cast<charT>(*first));
      size_ += n;
      traits::assign(ptr_[size_], charT());
    } else {
      for (; first != last; ++first)
        push_back(static_cast<charT>(*first));
    }
  }
  // A string holding [first, last), with this string's allocator; used where the elements may
  // live in *this.
  template <class It, class Sent>
  constexpr basic_string temp_of(It first, Sent last) const {
    basic_string t(alloc_);
    t.append_elements(static_cast<It&&>(first), static_cast<Sent&&>(last));
    return t;
  }
  template <class R>
  static constexpr bool char_contiguous_range =
      ranges::contiguous_range<R> && ranges::sized_range<R> && is_same_v<ranges::range_value_t<R>, charT>;

public:
  // ---- [string.cons] ----
  constexpr basic_string() noexcept(noexcept(Allocator())) : basic_string(Allocator()) {}
  constexpr explicit basic_string(const Allocator& a) noexcept : ptr_(nullptr), size_(0), alloc_(a) {
    set_short_empty();
  }
  constexpr basic_string(const basic_string& str)
      : ptr_(nullptr), size_(0), alloc_(alloc_traits::select_on_container_copy_construction(str.alloc_)) {
    init_copy(str.ptr_, str.size_);
  }
  constexpr basic_string(basic_string&& str) noexcept
      : ptr_(nullptr), size_(0), alloc_(static_cast<Allocator&&>(str.alloc_)) {
    take(str);
  }
  constexpr basic_string(const basic_string& str, size_type pos, const Allocator& a = Allocator())
      : basic_string(str, pos, npos, a) {}
  constexpr basic_string(const basic_string& str, size_type pos, size_type n, const Allocator& a = Allocator())
      : ptr_(nullptr), size_(0), alloc_(a) {
    str.check_pos(pos, "std::basic_string: pos > str.size()");
    init_copy(str.ptr_ + pos, str.clamp(pos, n));
  }
  constexpr basic_string(basic_string&& str, size_type pos, const Allocator& a = Allocator())
      : basic_string(static_cast<basic_string&&>(str), pos, npos, a) {}
  constexpr basic_string(basic_string&& str, size_type pos, size_type n, const Allocator& a = Allocator())
      : basic_string(a) {
    str.check_pos(pos, "std::basic_string: pos > str.size()");
    const size_type rlen = str.clamp(pos, n);
    if (always_equal || alloc_ == str.alloc_) {
      // Reuse str's storage ([string.cons]/8).
      take(str);
      traits::move(ptr_, ptr_ + pos, rlen);
      size_ = rlen;
      traits::assign(ptr_[rlen], charT());
    } else {
      assign(str.ptr_ + pos, rlen);
    }
  }
  template <class T>
    requires is_convertible_v<const T&, basic_string_view<charT, traits>>
  constexpr basic_string(const T& t, ycxx::detail::alloc_size_t<Allocator> pos, ycxx::detail::alloc_size_t<Allocator> n, const Allocator& a = Allocator())
      : ptr_(nullptr), size_(0), alloc_(a) {
    const sv_type sv = sv_type(t).substr(pos, n);
    init_copy(sv.data(), sv.size());
  }
  template <class T>
    requires sv_like<T>
  constexpr explicit basic_string(const T& t, const Allocator& a = Allocator()) : ptr_(nullptr), size_(0), alloc_(a) {
    const sv_type sv = t;
    init_copy(sv.data(), sv.size());
  }
  constexpr basic_string(const charT* s, ycxx::detail::alloc_size_t<Allocator> n, const Allocator& a = Allocator())
      : ptr_(nullptr), size_(0), alloc_(a) {
    ycxx::detail::precondition(s != nullptr || n == 0, "std::basic_string: null pointer with nonzero length");
    init_copy(s, n);
  }
  constexpr basic_string(const charT* s, const Allocator& a = Allocator())
    requires ycxx::detail::qualifies_as_allocator<Allocator>
      : ptr_(nullptr), size_(0), alloc_(a) {
    ycxx::detail::precondition(s != nullptr, "std::basic_string: null pointer");
    init_copy(s, traits::length(s));
  }
  basic_string(nullptr_t) = delete;
  constexpr basic_string(ycxx::detail::alloc_size_t<Allocator> n, charT c, const Allocator& a = Allocator())
    requires ycxx::detail::qualifies_as_allocator<Allocator>
      : ptr_(nullptr), size_(0), alloc_(a) {
    init_fill(n, c);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr basic_string(InputIterator begin, InputIterator end, const Allocator& a = Allocator()) : basic_string(a) {
    append_elements(static_cast<InputIterator&&>(begin), static_cast<InputIterator&&>(end));
  }
  template <ycxx::detail::container_compatible_range<charT> R>
  constexpr basic_string(from_range_t, R&& rg, const Allocator& a = Allocator()) : basic_string(a) {
    if constexpr (char_contiguous_range<R>)
      append(ranges::data(rg), static_cast<size_type>(ranges::size(rg)));
    else
      append_elements(ranges::begin(rg), ranges::end(rg));
  }
  constexpr basic_string(initializer_list<charT> il, const Allocator& a = Allocator())
      : ptr_(nullptr), size_(0), alloc_(a) {
    init_copy(il.begin(), il.size());
  }
  constexpr basic_string(const basic_string& str, const Allocator& a) : ptr_(nullptr), size_(0), alloc_(a) {
    init_copy(str.ptr_, str.size_);
  }
  constexpr basic_string(basic_string&& str, const Allocator& a) : ptr_(nullptr), size_(0), alloc_(a) {
    if (always_equal || alloc_ == str.alloc_)
      take(str);
    else
      init_copy(str.ptr_, str.size_);
  }

  constexpr ~basic_string() { free_storage(); }

  constexpr basic_string& operator=(const basic_string& str) {
    if (this == __builtin_addressof(str))
      return *this;
    if constexpr (pocca) {
      if (!always_equal && alloc_ != str.alloc_) {
        // The new allocator must own the storage: allocate with it before releasing ours.
        Allocator na = str.alloc_;
        if (str.size_ <= sso_cap) {
          free_storage();
          alloc_ = na;
          set_short_empty();
        } else {
          block b = allocate_block(na, str.size_);
          free_storage();
          alloc_ = na;
          set_long(b.p, b.cap);
        }
        traits::copy(ptr_, str.ptr_, str.size_ + 1);
        size_ = str.size_;
        return *this;
      }
      alloc_ = str.alloc_;
    }
    return replace_impl(0, size_, str.ptr_, str.size_);
  }
  constexpr basic_string& operator=(basic_string&& str) noexcept(pocma || always_equal) {
    if (this == __builtin_addressof(str))
      return *this;
    if constexpr (pocma || always_equal) {
      free_storage();
      if constexpr (pocma)
        alloc_ = static_cast<Allocator&&>(str.alloc_);
      take(str);
    } else {
      if (alloc_ == str.alloc_) {
        free_storage();
        take(str);
      } else {
        replace_impl(0, size_, str.ptr_, str.size_);
      }
    }
    return *this;
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& operator=(const T& t) {
    const sv_type sv = t;
    return assign(sv);
  }
  constexpr basic_string& operator=(const charT* s) { return *this = sv_type(s); }
  basic_string& operator=(nullptr_t) = delete;
  constexpr basic_string& operator=(charT c) { return *this = sv_type(__builtin_addressof(c), 1); }
  constexpr basic_string& operator=(initializer_list<charT> il) { return *this = sv_type(il.begin(), il.size()); }

  // ---- [string.iterators] ----
  constexpr iterator begin() noexcept { return iterator(ptr_); }
  constexpr const_iterator begin() const noexcept { return const_iterator(ptr_); }
  constexpr iterator end() noexcept { return iterator(ptr_ + size_); }
  constexpr const_iterator end() const noexcept { return const_iterator(ptr_ + size_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [string.capacity] ----
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type length() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    // One element of every allocation holds the terminator; pointer differences over the string
    // must be representable, and no object is larger than PTRDIFF_MAX bytes.
    const size_type by_alloc = alloc_traits::max_size(alloc_);
    const auto diff_max = static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max()) /
                          sizeof(charT);
    const size_type by_diff = diff_max < numeric_limits<size_type>::max() ? static_cast<size_type>(diff_max)
                                                                          : numeric_limits<size_type>::max();
    return (by_alloc < by_diff ? by_alloc : by_diff) - 1;
  }
  constexpr void resize(size_type n, charT c) {
    if (n <= size_) {
      size_ = n;
      traits::assign(ptr_[n], charT());
    } else {
      append(n - size_, c);
    }
  }
  constexpr void resize(size_type n) { resize(n, charT()); }
  template <class Operation>
  constexpr void resize_and_overwrite(size_type n, Operation op) {
    using R = decltype(static_cast<Operation&&>(op)(declval<charT*>(), declval<size_type>()));
    static_assert(ycxx::detail::integer_like<R>,
                  "std::basic_string::resize_and_overwrite: the operation must return an integer-like type");
    if (n > cap()) {
      check_length(n, "std::basic_string::resize_and_overwrite: n > max_size()");
      reallocate(n);
    }
    charT* const p = ptr_;
    const size_type m = n;
    // [string.capacity]/7 calls p and m "values": they are passed as prvalues, so the operation
    // may take them by value or by rvalue reference.
    const R r = static_cast<Operation&&>(op)(static_cast<charT*>(p), static_cast<size_type>(m));
    if constexpr (is_signed_v<R>)
      ycxx::detail::precondition(r >= 0, "std::basic_string::resize_and_overwrite: negative result");
    ycxx::detail::precondition(static_cast<make_unsigned_t<R>>(r) <= m,
                               "std::basic_string::resize_and_overwrite: result greater than n");
    size_ = static_cast<size_type>(r);
    traits::assign(p[size_], charT());
  }
  constexpr size_type capacity() const noexcept { return cap(); }
  constexpr void reserve(size_type res_arg) {
    check_length(res_arg, "std::basic_string::reserve: argument exceeds max_size()");
    if (res_arg > cap())
      reallocate(res_arg);
  }
  constexpr void shrink_to_fit() {
    if (!is_long())
      return;
    if (size_ <= sso_cap) {
      charT* const old = ptr_;
      const size_type old_cap = cap_;
      activate_buf();
      traits::copy(buf_, old, size_ + 1);
      deallocate_block(alloc_, old, old_cap);
    } else if (cap_ > size_) {
      block b = allocate_block(alloc_, size_);
      if (b.cap >= cap_) { // the allocator gave nothing back
        deallocate_block(alloc_, b.p, b.cap);
        return;
      }
      traits::copy(b.p, ptr_, size_ + 1);
      free_storage();
      set_long(b.p, b.cap);
    }
  }
  constexpr void clear() noexcept {
    size_ = 0;
    traits::assign(ptr_[0], charT());
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  // ---- [string.access] ----
  constexpr const_reference operator[](size_type pos) const {
    ycxx::detail::precondition(pos <= size_, "std::basic_string::operator[]: index out of range");
    return ptr_[pos];
  }
  constexpr reference operator[](size_type pos) {
    ycxx::detail::precondition(pos <= size_, "std::basic_string::operator[]: index out of range");
    return ptr_[pos];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::basic_string::at: index out of range");
    return ptr_[n];
  }
  constexpr reference at(size_type n) {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::basic_string::at: index out of range");
    return ptr_[n];
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size_ != 0, "std::basic_string::front: empty string");
    return ptr_[0];
  }
  constexpr reference front() {
    ycxx::detail::precondition(size_ != 0, "std::basic_string::front: empty string");
    return ptr_[0];
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size_ != 0, "std::basic_string::back: empty string");
    return ptr_[size_ - 1];
  }
  constexpr reference back() {
    ycxx::detail::precondition(size_ != 0, "std::basic_string::back: empty string");
    return ptr_[size_ - 1];
  }

  // ---- [string.op.append] ----
  constexpr basic_string& operator+=(const basic_string& str) { return append(str); }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& operator+=(const T& t) {
    const sv_type sv = t;
    return append(sv.data(), sv.size());
  }
  constexpr basic_string& operator+=(const charT* s) { return append(s); }
  constexpr basic_string& operator+=(charT c) {
    push_back(c);
    return *this;
  }
  constexpr basic_string& operator+=(initializer_list<charT> il) { return append(il); }

  // ---- [string.append] ----
  constexpr basic_string& append(const basic_string& str) { return append(str.ptr_, str.size_); }
  constexpr basic_string& append(const basic_string& str, size_type pos, size_type n = npos) {
    return append(sv_type(str).substr(pos, n));
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& append(const T& t) {
    const sv_type sv = t;
    return append(sv.data(), sv.size());
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& append(const T& t, size_type pos, size_type n = npos) {
    const sv_type sv = t;
    return append(sv.substr(pos, n));
  }
  constexpr basic_string& append(const charT* s, size_type n) {
    ycxx::detail::precondition(s != nullptr || n == 0, "std::basic_string::append: null pointer");
    if (n <= cap() - size_) {
      // In place: the destination lies past every character, so even a source inside *this
      // stays intact (move() also covers a source that includes the terminator).
      traits::move(ptr_ + size_, s, n);
      size_ += n;
      traits::assign(ptr_[size_], charT());
      return *this;
    }
    return replace_impl(size_, 0, s, n);
  }
  constexpr basic_string& append(const charT* s) {
    ycxx::detail::precondition(s != nullptr, "std::basic_string::append: null pointer");
    return append(s, traits::length(s));
  }
  constexpr basic_string& append(const charT* s, size_type pos, size_type n) { return append(sv_type(s).substr(pos, n)); }
  constexpr basic_string& append(size_type n, charT c) { return replace_fill(size_, 0, n, c); }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr basic_string& append(InputIterator first, InputIterator last) {
    if constexpr (char_ptr_iter<InputIterator>)
      return append(std::to_address(first), static_cast<size_type>(last - first));
    else
      return append(temp_of(first, last));
  }
  template <ycxx::detail::container_compatible_range<charT> R>
  constexpr basic_string& append_range(R&& rg) {
    if constexpr (char_contiguous_range<R>)
      return append(ranges::data(rg), static_cast<size_type>(ranges::size(rg)));
    else
      return append(temp_of(ranges::begin(rg), ranges::end(rg)));
  }
  constexpr basic_string& append(initializer_list<charT> il) { return append(il.begin(), il.size()); }
  constexpr void push_back(charT c) {
    if (size_ == cap())
      reallocate(grow_cap(size_ + 1));
    traits::assign(ptr_[size_], c);
    ++size_;
    traits::assign(ptr_[size_], charT());
  }

  // ---- [string.assign] ----
  constexpr basic_string& assign(const basic_string& str) { return *this = str; }
  constexpr basic_string& assign(basic_string&& str) noexcept(pocma || always_equal) {
    return *this = static_cast<basic_string&&>(str);
  }
  constexpr basic_string& assign(const basic_string& str, size_type pos, size_type n = npos) {
    return assign(sv_type(str).substr(pos, n));
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& assign(const T& t) {
    const sv_type sv = t;
    return assign(sv.data(), sv.size());
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& assign(const T& t, size_type pos, size_type n = npos) {
    const sv_type sv = t;
    return assign(sv.substr(pos, n));
  }
  constexpr basic_string& assign(const charT* s, size_type n) {
    ycxx::detail::precondition(s != nullptr || n == 0, "std::basic_string::assign: null pointer");
    return replace_impl(0, size_, s, n);
  }
  constexpr basic_string& assign(const charT* s) {
    ycxx::detail::precondition(s != nullptr, "std::basic_string::assign: null pointer");
    return assign(s, traits::length(s));
  }
  constexpr basic_string& assign(const charT* s, size_type pos, size_type n) { return assign(sv_type(s).substr(pos, n)); }
  constexpr basic_string& assign(initializer_list<charT> il) { return assign(il.begin(), il.size()); }
  constexpr basic_string& assign(size_type n, charT c) { return replace_fill(0, size_, n, c); }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr basic_string& assign(InputIterator first, InputIterator last) {
    if constexpr (char_ptr_iter<InputIterator>)
      return assign(std::to_address(first), static_cast<size_type>(last - first));
    else
      return assign(temp_of(first, last));
  }
  template <ycxx::detail::container_compatible_range<charT> R>
  constexpr basic_string& assign_range(R&& rg) {
    if constexpr (char_contiguous_range<R>)
      return assign(ranges::data(rg), static_cast<size_type>(ranges::size(rg)));
    else
      return assign(temp_of(ranges::begin(rg), ranges::end(rg)));
  }

  // ---- [string.insert] ----
  constexpr basic_string& insert(size_type pos, const basic_string& str) { return insert(pos, str.ptr_, str.size_); }
  constexpr basic_string& insert(size_type pos1, const basic_string& str, size_type pos2, size_type n = npos) {
    return insert(pos1, sv_type(str), pos2, n);
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& insert(size_type pos, const T& t) {
    const sv_type sv = t;
    return insert(pos, sv.data(), sv.size());
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& insert(size_type pos1, const T& t, size_type pos2, size_type n = npos) {
    const sv_type sv = t;
    return insert(pos1, sv.substr(pos2, n));
  }
  constexpr basic_string& insert(size_type pos, const charT* s, size_type n) {
    ycxx::detail::precondition(s != nullptr || n == 0, "std::basic_string::insert: null pointer");
    check_pos(pos, "std::basic_string::insert: pos > size()");
    return replace_impl(pos, 0, s, n);
  }
  constexpr basic_string& insert(size_type pos, const charT* s) {
    ycxx::detail::precondition(s != nullptr, "std::basic_string::insert: null pointer");
    return insert(pos, s, traits::length(s));
  }
  constexpr basic_string& insert(size_type pos, size_type n, charT c) {
    check_pos(pos, "std::basic_string::insert: pos > size()");
    return replace_fill(pos, 0, n, c);
  }
  constexpr iterator insert(const_iterator p, charT c) {
    const size_type pos = static_cast<size_type>(p - cbegin());
    replace_fill(pos, 0, 1, c);
    return begin() + static_cast<difference_type>(pos);
  }
  constexpr iterator insert(const_iterator p, size_type n, charT c) {
    const size_type pos = static_cast<size_type>(p - cbegin());
    replace_fill(pos, 0, n, c);
    return begin() + static_cast<difference_type>(pos);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator p, InputIterator first, InputIterator last) {
    const size_type pos = static_cast<size_type>(p - cbegin());
    if constexpr (char_ptr_iter<InputIterator>) {
      replace_impl(pos, 0, std::to_address(first), static_cast<size_type>(last - first));
    } else {
      const basic_string t = temp_of(first, last);
      replace_impl(pos, 0, t.ptr_, t.size_);
    }
    return begin() + static_cast<difference_type>(pos);
  }
  template <ycxx::detail::container_compatible_range<charT> R>
  constexpr iterator insert_range(const_iterator p, R&& rg) {
    const size_type pos = static_cast<size_type>(p - cbegin());
    if constexpr (char_contiguous_range<R>) {
      replace_impl(pos, 0, ranges::data(rg), static_cast<size_type>(ranges::size(rg)));
    } else {
      const basic_string t = temp_of(ranges::begin(rg), ranges::end(rg));
      replace_impl(pos, 0, t.ptr_, t.size_);
    }
    return begin() + static_cast<difference_type>(pos);
  }
  constexpr iterator insert(const_iterator p, initializer_list<charT> il) {
    const size_type pos = static_cast<size_type>(p - cbegin());
    replace_impl(pos, 0, il.begin(), il.size());
    return begin() + static_cast<difference_type>(pos);
  }

  // ---- [string.erase] ----
  constexpr basic_string& erase(size_type pos = 0, size_type n = npos) {
    check_pos(pos, "std::basic_string::erase: pos > size()");
    erase_impl(pos, clamp(pos, n));
    return *this;
  }
  constexpr iterator erase(const_iterator p) noexcept {
    const size_type pos = static_cast<size_type>(p - cbegin());
    ycxx::detail::precondition(pos < size_, "std::basic_string::erase: iterator not dereferenceable");
    erase_impl(pos, 1);
    return begin() + static_cast<difference_type>(pos);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
    const size_type pos = static_cast<size_type>(first - cbegin());
    erase_impl(pos, static_cast<size_type>(last - first));
    return begin() + static_cast<difference_type>(pos);
  }
  constexpr void pop_back() noexcept {
    ycxx::detail::precondition(size_ != 0, "std::basic_string::pop_back: empty string");
    --size_;
    traits::assign(ptr_[size_], charT());
  }

  // ---- [string.replace] ----
  constexpr basic_string& replace(size_type pos1, size_type n1, const basic_string& str) {
    return replace(pos1, n1, str.ptr_, str.size_);
  }
  constexpr basic_string& replace(size_type pos1, size_type n1, const basic_string& str, size_type pos2,
                                  size_type n2 = npos) {
    return replace(pos1, n1, sv_type(str).substr(pos2, n2));
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& replace(size_type pos1, size_type n1, const T& t) {
    const sv_type sv = t;
    return replace(pos1, n1, sv.data(), sv.size());
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& replace(size_type pos1, size_type n1, const T& t, size_type pos2, size_type n2 = npos) {
    const sv_type sv = t;
    return replace(pos1, n1, sv.substr(pos2, n2));
  }
  constexpr basic_string& replace(size_type pos1, size_type n1, const charT* s, size_type n2) {
    ycxx::detail::precondition(s != nullptr || n2 == 0, "std::basic_string::replace: null pointer");
    check_pos(pos1, "std::basic_string::replace: pos > size()");
    return replace_impl(pos1, clamp(pos1, n1), s, n2);
  }
  constexpr basic_string& replace(size_type pos, size_type n1, const charT* s) {
    ycxx::detail::precondition(s != nullptr, "std::basic_string::replace: null pointer");
    return replace(pos, n1, s, traits::length(s));
  }
  constexpr basic_string& replace(size_type pos1, size_type n1, size_type n2, charT c) {
    check_pos(pos1, "std::basic_string::replace: pos > size()");
    return replace_fill(pos1, clamp(pos1, n1), n2, c);
  }
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, const basic_string& str) {
    return replace(i1, i2, sv_type(str));
  }
  template <class T>
    requires sv_like<T>
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, const T& t) {
    const sv_type sv = t;
    return replace_impl(static_cast<size_type>(i1 - cbegin()), static_cast<size_type>(i2 - i1), sv.data(), sv.size());
  }
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, const charT* s, size_type n) {
    return replace(i1, i2, sv_type(s, n));
  }
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, const charT* s) {
    return replace(i1, i2, sv_type(s));
  }
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, size_type n, charT c) {
    return replace_fill(static_cast<size_type>(i1 - cbegin()), static_cast<size_type>(i2 - i1), n, c);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, InputIterator j1, InputIterator j2) {
    const size_type pos = static_cast<size_type>(i1 - cbegin());
    const size_type n1 = static_cast<size_type>(i2 - i1);
    if constexpr (char_ptr_iter<InputIterator>) {
      return replace_impl(pos, n1, std::to_address(j1), static_cast<size_type>(j2 - j1));
    } else {
      const basic_string t = temp_of(j1, j2);
      return replace_impl(pos, n1, t.ptr_, t.size_);
    }
  }
  template <ycxx::detail::container_compatible_range<charT> R>
  constexpr basic_string& replace_with_range(const_iterator i1, const_iterator i2, R&& rg) {
    const size_type pos = static_cast<size_type>(i1 - cbegin());
    const size_type n1 = static_cast<size_type>(i2 - i1);
    if constexpr (char_contiguous_range<R>) {
      return replace_impl(pos, n1, ranges::data(rg), static_cast<size_type>(ranges::size(rg)));
    } else {
      const basic_string t = temp_of(ranges::begin(rg), ranges::end(rg));
      return replace_impl(pos, n1, t.ptr_, t.size_);
    }
  }
  constexpr basic_string& replace(const_iterator i1, const_iterator i2, initializer_list<charT> il) {
    return replace(i1, i2, il.begin(), il.size());
  }

  // ---- [string.copy], [string.swap] ----
  constexpr size_type copy(charT* s, size_type n, size_type pos = 0) const {
    return static_cast<size_type>(sv_type(*this).copy(s, n, pos));
  }
  constexpr void swap(basic_string& s) noexcept(pocs || always_equal) {
    if (this == __builtin_addressof(s))
      return;
    if constexpr (pocs)
      ::ycxx::detail::swap_adl::do_swap(alloc_, s.alloc_);
    else
      ycxx::detail::precondition(always_equal || alloc_ == s.alloc_,
                                 "std::basic_string::swap: unequal allocators that do not propagate");
    const bool l1 = is_long(), l2 = s.is_long();
    if (l1 && l2) {
      charT* const p = ptr_;
      const size_type c = cap_;
      set_long(s.ptr_, s.cap_);
      s.set_long(p, c);
    } else if (!l1 && !l2) {
      charT tmp[buf_len];
      traits::copy(tmp, buf_, size_ + 1);
      traits::copy(buf_, s.buf_, s.size_ + 1);
      traits::copy(s.buf_, tmp, size_ + 1);
    } else {
      basic_string& lng = l1 ? *this : s;
      basic_string& shrt = l1 ? s : *this;
      charT tmp[buf_len];
      traits::copy(tmp, shrt.buf_, shrt.size_ + 1);
      shrt.set_long(lng.ptr_, lng.cap_);
      lng.activate_buf();
      traits::copy(lng.buf_, tmp, shrt.size_ + 1);
    }
    const size_type n = size_;
    size_ = s.size_;
    s.size_ = n;
  }

  // ---- [string.accessors] ----
  constexpr const charT* c_str() const noexcept { return ptr_; }
  constexpr const charT* data() const noexcept { return ptr_; }
  constexpr charT* data() noexcept { return ptr_; }
  constexpr operator basic_string_view<charT, traits>() const noexcept { return sv_type(ptr_, size_); }
  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  // ---- [string.find] ----
private:
  static constexpr size_type to_npos(size_t r) noexcept { return r == sv_type::npos ? npos : static_cast<size_type>(r); }

public:
  template <class T>
    requires sv_like<T>
  constexpr size_type find(const T& t, size_type pos = 0) const noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).find(sv, pos));
  }
  constexpr size_type find(const basic_string& str, size_type pos = 0) const noexcept {
    return to_npos(sv_type(*this).find(sv_type(str), pos));
  }
  constexpr size_type find(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).find(sv_type(s, n), pos));
  }
  constexpr size_type find(const charT* s, size_type pos = 0) const {
    return to_npos(sv_type(*this).find(sv_type(s), pos));
  }
  constexpr size_type find(charT c, size_type pos = 0) const noexcept { return to_npos(sv_type(*this).find(c, pos)); }

  template <class T>
    requires sv_like<T>
  constexpr size_type rfind(const T& t, size_type pos = npos) const noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).rfind(sv, pos));
  }
  constexpr size_type rfind(const basic_string& str, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).rfind(sv_type(str), pos));
  }
  constexpr size_type rfind(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).rfind(sv_type(s, n), pos));
  }
  constexpr size_type rfind(const charT* s, size_type pos = npos) const {
    return to_npos(sv_type(*this).rfind(sv_type(s), pos));
  }
  constexpr size_type rfind(charT c, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).rfind(c, pos));
  }

  template <class T>
    requires sv_like<T>
  constexpr size_type find_first_of(const T& t, size_type pos = 0) const
      noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).find_first_of(sv, pos));
  }
  constexpr size_type find_first_of(const basic_string& str, size_type pos = 0) const noexcept {
    return to_npos(sv_type(*this).find_first_of(sv_type(str), pos));
  }
  constexpr size_type find_first_of(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).find_first_of(sv_type(s, n), pos));
  }
  constexpr size_type find_first_of(const charT* s, size_type pos = 0) const {
    return to_npos(sv_type(*this).find_first_of(sv_type(s), pos));
  }
  constexpr size_type find_first_of(charT c, size_type pos = 0) const noexcept {
    return to_npos(sv_type(*this).find_first_of(c, pos));
  }

  template <class T>
    requires sv_like<T>
  constexpr size_type find_last_of(const T& t, size_type pos = npos) const
      noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).find_last_of(sv, pos));
  }
  constexpr size_type find_last_of(const basic_string& str, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).find_last_of(sv_type(str), pos));
  }
  constexpr size_type find_last_of(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).find_last_of(sv_type(s, n), pos));
  }
  constexpr size_type find_last_of(const charT* s, size_type pos = npos) const {
    return to_npos(sv_type(*this).find_last_of(sv_type(s), pos));
  }
  constexpr size_type find_last_of(charT c, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).find_last_of(c, pos));
  }

  template <class T>
    requires sv_like<T>
  constexpr size_type find_first_not_of(const T& t, size_type pos = 0) const
      noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).find_first_not_of(sv, pos));
  }
  constexpr size_type find_first_not_of(const basic_string& str, size_type pos = 0) const noexcept {
    return to_npos(sv_type(*this).find_first_not_of(sv_type(str), pos));
  }
  constexpr size_type find_first_not_of(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).find_first_not_of(sv_type(s, n), pos));
  }
  constexpr size_type find_first_not_of(const charT* s, size_type pos = 0) const {
    return to_npos(sv_type(*this).find_first_not_of(sv_type(s), pos));
  }
  constexpr size_type find_first_not_of(charT c, size_type pos = 0) const noexcept {
    return to_npos(sv_type(*this).find_first_not_of(c, pos));
  }

  template <class T>
    requires sv_like<T>
  constexpr size_type find_last_not_of(const T& t, size_type pos = npos) const
      noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    const sv_type sv = t;
    return to_npos(sv_type(*this).find_last_not_of(sv, pos));
  }
  constexpr size_type find_last_not_of(const basic_string& str, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).find_last_not_of(sv_type(str), pos));
  }
  constexpr size_type find_last_not_of(const charT* s, size_type pos, size_type n) const {
    return to_npos(sv_type(*this).find_last_not_of(sv_type(s, n), pos));
  }
  constexpr size_type find_last_not_of(const charT* s, size_type pos = npos) const {
    return to_npos(sv_type(*this).find_last_not_of(sv_type(s), pos));
  }
  constexpr size_type find_last_not_of(charT c, size_type pos = npos) const noexcept {
    return to_npos(sv_type(*this).find_last_not_of(c, pos));
  }

  // ---- [string.substr] ----
  constexpr basic_string substr(size_type pos = 0, size_type n = npos) const& { return basic_string(*this, pos, n); }
  constexpr basic_string substr(size_type pos = 0, size_type n = npos) && {
    return basic_string(static_cast<basic_string&&>(*this), pos, n);
  }
  constexpr basic_string_view<charT, traits> subview(size_type pos = 0, size_type n = npos) const {
    return sv_type(*this).subview(pos, n);
  }

  // ---- [string.compare] ----
  template <class T>
    requires sv_like<T>
  constexpr int compare(const T& t) const noexcept(is_nothrow_convertible_v<const T&, sv_type>) {
    return sv_type(*this).compare(t);
  }
  template <class T>
    requires sv_like<T>
  constexpr int compare(size_type pos1, size_type n1, const T& t) const {
    return sv_type(*this).substr(pos1, n1).compare(t);
  }
  template <class T>
    requires sv_like<T>
  constexpr int compare(size_type pos1, size_type n1, const T& t, size_type pos2, size_type n2 = npos) const {
    const sv_type s = *this, sv = t;
    return s.substr(pos1, n1).compare(sv.substr(pos2, n2));
  }
  constexpr int compare(const basic_string& str) const noexcept { return sv_type(*this).compare(sv_type(str)); }
  constexpr int compare(size_type pos1, size_type n1, const basic_string& str) const {
    return sv_type(*this).substr(pos1, n1).compare(sv_type(str));
  }
  constexpr int compare(size_type pos1, size_type n1, const basic_string& str, size_type pos2,
                        size_type n2 = npos) const {
    return sv_type(*this).substr(pos1, n1).compare(sv_type(str).substr(pos2, n2));
  }
  constexpr int compare(const charT* s) const { return sv_type(*this).compare(sv_type(s)); }
  constexpr int compare(size_type pos1, size_type n1, const charT* s) const {
    return sv_type(*this).substr(pos1, n1).compare(sv_type(s));
  }
  constexpr int compare(size_type pos1, size_type n1, const charT* s, size_type n2) const {
    return sv_type(*this).substr(pos1, n1).compare(sv_type(s, n2));
  }

  // ---- [string.starts.with], [string.ends.with], [string.contains] ----
  constexpr bool starts_with(basic_string_view<charT, traits> x) const noexcept { return sv_type(*this).starts_with(x); }
  constexpr bool starts_with(charT x) const noexcept { return sv_type(*this).starts_with(x); }
  constexpr bool starts_with(const charT* x) const { return sv_type(*this).starts_with(x); }
  constexpr bool ends_with(basic_string_view<charT, traits> x) const noexcept { return sv_type(*this).ends_with(x); }
  constexpr bool ends_with(charT x) const noexcept { return sv_type(*this).ends_with(x); }
  constexpr bool ends_with(const charT* x) const { return sv_type(*this).ends_with(x); }
  constexpr bool contains(basic_string_view<charT, traits> x) const noexcept { return sv_type(*this).contains(x); }
  constexpr bool contains(charT x) const noexcept { return sv_type(*this).contains(x); }
  constexpr bool contains(const charT* x) const { return sv_type(*this).contains(x); }
};

// ---- deduction guides ([string.cons]) ----
template <class InputIterator, class Allocator = allocator<typename iterator_traits<InputIterator>::value_type>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
basic_string(InputIterator, InputIterator, Allocator = Allocator())
    -> basic_string<typename iterator_traits<InputIterator>::value_type,
                    char_traits<typename iterator_traits<InputIterator>::value_type>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
basic_string(from_range_t, R&&, Allocator = Allocator())
    -> basic_string<ranges::range_value_t<R>, char_traits<ranges::range_value_t<R>>, Allocator>;
template <class charT, class traits, class Allocator = allocator<charT>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
explicit basic_string(basic_string_view<charT, traits>, const Allocator& = Allocator())
    -> basic_string<charT, traits, Allocator>;
template <class charT, class traits, class Allocator = allocator<charT>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
basic_string(basic_string_view<charT, traits>, typename basic_string<charT, traits, Allocator>::size_type,
             typename basic_string<charT, traits, Allocator>::size_type, const Allocator& = Allocator())
    -> basic_string<charT, traits, Allocator>;

} // namespace std

namespace ycxx::detail {

// lhs + rhs as a new string with allocator a ([string.op.plus]: a copy of one operand, then an
// append or insert), sized once.
template <class S>
constexpr S string_concat(const typename S::allocator_type& a, const typename S::value_type* l,
                          typename S::size_type nl, const typename S::value_type* r, typename S::size_type nr) {
  S s(a);
  if (nr > s.max_size() || nl > s.max_size() - nr)
    ::ycxx::detail::throw_length_error("std::operator+: length exceeds max_size()");
  s.reserve(nl + nr);
  s.append(l, nl);
  s.append(r, nr);
  return s;
}
template <class S>
constexpr typename S::allocator_type copy_alloc(const S& s) {
  return std::allocator_traits<typename S::allocator_type>::select_on_container_copy_construction(s.get_allocator());
}

} // namespace ycxx::detail

namespace std {

// ---- [string.op.plus] ----
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const basic_string<charT, traits, Allocator>& lhs,
                                                           const basic_string<charT, traits, Allocator>& rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(lhs), lhs.data(), lhs.size(), rhs.data(), rhs.size());
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(basic_string<charT, traits, Allocator>&& lhs,
                                                           const basic_string<charT, traits, Allocator>& rhs) {
  lhs.append(rhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(lhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const basic_string<charT, traits, Allocator>& lhs,
                                                           basic_string<charT, traits, Allocator>&& rhs) {
  rhs.insert(0, lhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(basic_string<charT, traits, Allocator>&& lhs,
                                                           basic_string<charT, traits, Allocator>&& rhs) {
  // Note 1: with equal allocators either operand's storage may be reused; take rhs's when only
  // it has room for the result.
  if (lhs.capacity() - lhs.size() < rhs.size() && rhs.capacity() - rhs.size() >= lhs.size() &&
      lhs.get_allocator() == rhs.get_allocator()) {
    rhs.insert(0, lhs);
    return static_cast<basic_string<charT, traits, Allocator>&&>(rhs);
  }
  lhs.append(rhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(lhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const charT* lhs,
                                                           const basic_string<charT, traits, Allocator>& rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(rhs), lhs,
                                        static_cast<typename S::size_type>(traits::length(lhs)), rhs.data(), rhs.size());
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const charT* lhs, basic_string<charT, traits, Allocator>&& rhs) {
  rhs.insert(0, lhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(charT lhs, const basic_string<charT, traits, Allocator>& rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(rhs), __builtin_addressof(lhs), 1, rhs.data(), rhs.size());
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(charT lhs, basic_string<charT, traits, Allocator>&& rhs) {
  rhs.insert(rhs.begin(), lhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const basic_string<charT, traits, Allocator>& lhs,
                                                           const charT* rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(lhs), lhs.data(), lhs.size(), rhs,
                                        static_cast<typename S::size_type>(traits::length(rhs)));
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(basic_string<charT, traits, Allocator>&& lhs, const charT* rhs) {
  lhs.append(rhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(lhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const basic_string<charT, traits, Allocator>& lhs, charT rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(lhs), lhs.data(), lhs.size(), __builtin_addressof(rhs), 1);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(basic_string<charT, traits, Allocator>&& lhs, charT rhs) {
  lhs.push_back(rhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(lhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(const basic_string<charT, traits, Allocator>& lhs,
                                                           type_identity_t<basic_string_view<charT, traits>> rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(lhs), lhs.data(), lhs.size(), rhs.data(),
                                        static_cast<typename S::size_type>(rhs.size()));
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(basic_string<charT, traits, Allocator>&& lhs,
                                                           type_identity_t<basic_string_view<charT, traits>> rhs) {
  lhs.append(rhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(lhs);
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(type_identity_t<basic_string_view<charT, traits>> lhs,
                                                           const basic_string<charT, traits, Allocator>& rhs) {
  using S = basic_string<charT, traits, Allocator>;
  return ycxx::detail::string_concat<S>(ycxx::detail::copy_alloc(rhs), lhs.data(),
                                        static_cast<typename S::size_type>(lhs.size()), rhs.data(), rhs.size());
}
template <class charT, class traits, class Allocator>
constexpr basic_string<charT, traits, Allocator> operator+(type_identity_t<basic_string_view<charT, traits>> lhs,
                                                           basic_string<charT, traits, Allocator>&& rhs) {
  rhs.insert(0, lhs);
  return static_cast<basic_string<charT, traits, Allocator>&&>(rhs);
}

// ---- [string.cmp] ----
template <class charT, class traits, class Allocator>
constexpr bool operator==(const basic_string<charT, traits, Allocator>& lhs,
                          const basic_string<charT, traits, Allocator>& rhs) noexcept {
  return basic_string_view<charT, traits>(lhs) == basic_string_view<charT, traits>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr bool operator==(const basic_string<charT, traits, Allocator>& lhs, const charT* rhs) {
  return basic_string_view<charT, traits>(lhs) == basic_string_view<charT, traits>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr auto operator<=>(const basic_string<charT, traits, Allocator>& lhs,
                           const basic_string<charT, traits, Allocator>& rhs) noexcept
    -> decltype(basic_string_view<charT, traits>(lhs) <=> basic_string_view<charT, traits>(rhs)) {
  return basic_string_view<charT, traits>(lhs) <=> basic_string_view<charT, traits>(rhs);
}
template <class charT, class traits, class Allocator>
constexpr auto operator<=>(const basic_string<charT, traits, Allocator>& lhs, const charT* rhs)
    -> decltype(basic_string_view<charT, traits>(lhs) <=> basic_string_view<charT, traits>(rhs)) {
  return basic_string_view<charT, traits>(lhs) <=> basic_string_view<charT, traits>(rhs);
}

// ---- [string.special] ----
template <class charT, class traits, class Allocator>
constexpr void swap(basic_string<charT, traits, Allocator>& lhs,
                    basic_string<charT, traits, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

// ---- [string.erasure] ----
template <class charT, class traits, class Allocator, class Predicate>
constexpr typename basic_string<charT, traits, Allocator>::size_type erase_if(basic_string<charT, traits, Allocator>& c,
                                                                              Predicate pred) {
  // remove_if, then erase the tail.
  auto first = c.begin();
  const auto last = c.end();
  while (first != last && !bool(pred(*first)))
    ++first;
  auto out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!bool(pred(*first)))
        *out++ = static_cast<charT&&>(*first);
  }
  const auto r = static_cast<typename basic_string<charT, traits, Allocator>::size_type>(last - out);
  c.erase(out, last);
  return r;
}
template <class charT, class traits, class Allocator, class U = charT>
constexpr typename basic_string<charT, traits, Allocator>::size_type erase(basic_string<charT, traits, Allocator>& c,
                                                                           const U& value) {
  return std::erase_if(c, [&value](const charT& e) { return bool(e == value); });
}

// ---- typedef-names ----
using string = basic_string<char>;
using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
using wstring = basic_string<wchar_t>;

namespace pmr {
template <class charT, class traits = char_traits<charT>>
using basic_string = std::basic_string<charT, traits, polymorphic_allocator<charT>>;
using string = basic_string<char>;
using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
using wstring = basic_string<wchar_t>;
} // namespace pmr

} // namespace std

// ---- [basic.string.hash] ----
namespace ycxx::adl_free {
// hash<S>()(s) == hash<SV>()(SV(s)) for the five standard string types.
template <class S>
struct string_hash {
  [[nodiscard]] std::size_t operator()(const S& s) const noexcept {
    return std::hash<std::basic_string_view<typename S::value_type>>()(
        std::basic_string_view<typename S::value_type>(s.data(), s.size()));
  }
};
} // namespace ycxx::adl_free

namespace std {

template <class A>
struct hash<basic_string<char, char_traits<char>, A>>
    : ycxx::adl_free::string_hash<basic_string<char, char_traits<char>, A>> {};
template <class A>
struct hash<basic_string<char8_t, char_traits<char8_t>, A>>
    : ycxx::adl_free::string_hash<basic_string<char8_t, char_traits<char8_t>, A>> {};
template <class A>
struct hash<basic_string<char16_t, char_traits<char16_t>, A>>
    : ycxx::adl_free::string_hash<basic_string<char16_t, char_traits<char16_t>, A>> {};
template <class A>
struct hash<basic_string<char32_t, char_traits<char32_t>, A>>
    : ycxx::adl_free::string_hash<basic_string<char32_t, char_traits<char32_t>, A>> {};
template <class A>
struct hash<basic_string<wchar_t, char_traits<wchar_t>, A>>
    : ycxx::adl_free::string_hash<basic_string<wchar_t, char_traits<wchar_t>, A>> {};

// ---- [basic.string.literals] ----
inline namespace literals {
inline namespace string_literals {
constexpr string operator""s(const char* str, size_t len) { return string(str, len); }
constexpr u8string operator""s(const char8_t* str, size_t len) { return u8string(str, len); }
constexpr u16string operator""s(const char16_t* str, size_t len) { return u16string(str, len); }
constexpr u32string operator""s(const char32_t* str, size_t len) { return u32string(str, len); }
constexpr wstring operator""s(const wchar_t* str, size_t len) { return wstring(str, len); }
} // namespace string_literals
} // namespace literals

} // namespace std

// ---- [string.conversions] ----
namespace ycxx::detail {

// format("{}", v) for an integer: decimal digits, a leading '-' for negative values.
template <class charT, class T>
constexpr std::basic_string<charT> integer_to_string(T v) {
  using U = std::make_unsigned_t<T>;
  U u = static_cast<U>(v);
  bool neg = false;
  if constexpr (std::is_signed_v<T>) {
    if (v < 0) {
      neg = true;
      u = static_cast<U>(U(0) - u);
    }
  }
  charT buf[std::numeric_limits<U>::digits10 + 2];
  charT* const end = buf + sizeof(buf) / sizeof(charT);
  charT* p = end;
  do {
    *--p = static_cast<charT>('0' + static_cast<int>(u % 10));
    u /= 10;
  } while (u != 0);
  if (neg)
    *--p = static_cast<charT>('-');
  return std::basic_string<charT>(p, static_cast<std::size_t>(end - p));
}

} // namespace ycxx::detail

namespace std {

// Defined in the hosted runtime (src/hosted/string.cpp): they call the C library.
int stoi(const string& str, size_t* idx = nullptr, int base = 10);
long stol(const string& str, size_t* idx = nullptr, int base = 10);
unsigned long stoul(const string& str, size_t* idx = nullptr, int base = 10);
long long stoll(const string& str, size_t* idx = nullptr, int base = 10);
unsigned long long stoull(const string& str, size_t* idx = nullptr, int base = 10);
float stof(const string& str, size_t* idx = nullptr);
double stod(const string& str, size_t* idx = nullptr);
long double stold(const string& str, size_t* idx = nullptr);
int stoi(const wstring& str, size_t* idx = nullptr, int base = 10);
long stol(const wstring& str, size_t* idx = nullptr, int base = 10);
unsigned long stoul(const wstring& str, size_t* idx = nullptr, int base = 10);
long long stoll(const wstring& str, size_t* idx = nullptr, int base = 10);
unsigned long long stoull(const wstring& str, size_t* idx = nullptr, int base = 10);
float stof(const wstring& str, size_t* idx = nullptr);
double stod(const wstring& str, size_t* idx = nullptr);
long double stold(const wstring& str, size_t* idx = nullptr);
string to_string(float val);
string to_string(double val);
string to_string(long double val);
wstring to_wstring(float val);
wstring to_wstring(double val);
wstring to_wstring(long double val);

constexpr string to_string(int val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr string to_string(unsigned val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr string to_string(long val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr string to_string(unsigned long val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr string to_string(long long val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr string to_string(unsigned long long val) { return ycxx::detail::integer_to_string<char>(val); }
constexpr wstring to_wstring(int val) { return ycxx::detail::integer_to_string<wchar_t>(val); }
constexpr wstring to_wstring(unsigned val) { return ycxx::detail::integer_to_string<wchar_t>(val); }
constexpr wstring to_wstring(long val) { return ycxx::detail::integer_to_string<wchar_t>(val); }
constexpr wstring to_wstring(unsigned long val) { return ycxx::detail::integer_to_string<wchar_t>(val); }
constexpr wstring to_wstring(long long val) { return ycxx::detail::integer_to_string<wchar_t>(val); }
constexpr wstring to_wstring(unsigned long long val) { return ycxx::detail::integer_to_string<wchar_t>(val); }

// [string.io]: declared against the iostreams' forward declarations; defined with the streams
// (ycxx/hosted/istream.hpp, ycxx/hosted/ostream.hpp), so <string> does not include them.
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str);
template <class charT, class traits, class Allocator>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                         const basic_string<charT, traits, Allocator>& str);
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str,
                                      charT delim);
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>&& is, basic_string<charT, traits, Allocator>& str,
                                      charT delim);
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>& is, basic_string<charT, traits, Allocator>& str);
template <class charT, class traits, class Allocator>
basic_istream<charT, traits>& getline(basic_istream<charT, traits>&& is, basic_string<charT, traits, Allocator>& str);

} // namespace std

// The <stdexcept> constructors taking `const string&`, now that string is complete.
#include <ycxx/core/stdexcept_string.hpp>
