// libycxx core: inplace_vector ([inplace.vector]) and its erasure functions.
//
// Representation: the elements live in an anonymous union `{ T d_[N]; ... }` (so no element
// is constructed until it is inserted), followed by the size in the smallest unsigned type
// that can hold N. For N == 0 the storage is an empty class. All of it lives in the base
// ycxx::adl_free::iv_storage, whose special members are defaulted (trivial) exactly when
// [inplace.vector.overview]/5 asks for a trivial special member of inplace_vector, which then
// declares none of its own.
//
// Constant evaluation: beginning the lifetime of one element of an array member of a union
// (P3074, C++26) works on GCC 16 but not on Clang 23 (probed in-language below). So:
//  - for a trivially destructible, default-constructible T the whole array is
//    value-initialized when an inplace_vector is created during constant evaluation; the
//    elements are then reconstructed in place. This also lets a constant-initialized
//    inplace_vector of such a type hold elements.
//  - on a compiler without P3074, an inplace_vector of a non-trivially-destructible T keeps
//    its elements in storage from std::allocator during constant evaluation (a second union
//    member holds the pointer); they never outlive that evaluation.
//  - other element types (trivially destructible but not default-constructible) cannot be
//    used in constant evaluation on such a compiler.
#pragma once

#include <initializer_list>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace ycxx::detail {

// Probe: can construct_at begin the lifetime of one element of an array that is a union
// member with no active member, during constant evaluation (P3074)?
namespace iv_probe {
struct elem {
  int v;
  constexpr elem(int x) : v(x) {}
  constexpr ~elem() {}
};
template <class T>
union holder {
  T d[2];
  constexpr holder() {}
  constexpr ~holder() {}
};
template <class T>
constexpr bool run() {
  holder<T> h;
  std::construct_at(h.d + 0, 1);
  const bool r = h.d[0].v == 1;
  std::destroy_at(h.d + 0);
  return r;
}
template <class T>
concept works = requires { typename std::bool_constant<run<T>()>; };
} // namespace iv_probe
inline constexpr bool union_array_lifetime = iv_probe::works<iv_probe::elem>;

template <std::size_t N>
using iv_size_t = std::conditional_t<
    (N <= 0xffu), unsigned char,
    std::conditional_t<(N <= 0xffffu), unsigned short, std::conditional_t<(N <= 0xffffffffu), unsigned, std::size_t>>>;

} // namespace ycxx::detail

namespace ycxx::adl_free {

template <class T, std::size_t N, bool = (N == 0)>
struct iv_storage;

// N == 0: empty and trivial ([inplace.vector.overview]/5).
template <class T, std::size_t N>
struct iv_storage<T, N, true> {
  static constexpr T* iv_data() noexcept { return nullptr; }
  static constexpr std::size_t iv_size() noexcept { return 0; }
  static constexpr void iv_set_size(std::size_t) noexcept {}
  static constexpr void iv_prepare() noexcept {}
};

template <class T, std::size_t N>
struct iv_storage<T, N, false> {
  using size_type = ycxx::detail::iv_size_t<N>;
  // See the header comment.
  static constexpr bool prefill = std::is_trivially_destructible_v<T> && std::is_default_constructible_v<T>;
  static constexpr bool heap = !ycxx::detail::union_array_lifetime && !std::is_trivially_destructible_v<T>;

  union {
    T d_[N];
    std::conditional_t<heap, T*, unsigned char> cx_;
  };
  size_type n_ = 0;

  constexpr T* iv_data() noexcept {
    if constexpr (heap) {
      if consteval {
        return cx_;
      }
    }
    return d_;
  }
  constexpr const T* iv_data() const noexcept {
    if constexpr (heap) {
      if consteval {
        return cx_;
      }
    }
    return d_;
  }
  constexpr std::size_t iv_size() const noexcept { return n_; }
  constexpr void iv_set_size(std::size_t n) noexcept { n_ = static_cast<size_type>(n); }
  // Called before elements are constructed.
  constexpr void iv_prepare() {
    if constexpr (heap) {
      if consteval {
        if (!cx_)
          cx_ = std::allocator<T>().allocate(N);
      }
    }
  }
  constexpr void iv_destroy_from(std::size_t k) noexcept {
    T* const p = iv_data();
    for (std::size_t i = k; i != n_; ++i)
      std::destroy_at(p + i);
    n_ = static_cast<size_type>(k);
  }
  template <class Src>
  constexpr void iv_construct_from(Src* q, std::size_t n) {
    iv_prepare();
    T* const p = iv_data();
    for (std::size_t i = n_; i != n; ++i) {
      if constexpr (std::is_const_v<Src>)
        std::construct_at(p + i, q[i]);
      else
        std::construct_at(p + i, static_cast<T&&>(q[i]));
      ++n_;
    }
  }

  constexpr iv_storage() noexcept {
    if consteval {
      if constexpr (heap)
        cx_ = nullptr;
      else if constexpr (prefill)
        std::construct_at(__builtin_addressof(d_));
    }
  }

  constexpr iv_storage(const iv_storage&)
    requires std::is_trivially_copy_constructible_v<T>
  = default;
  // (The copy operations' noexcept is a strengthening; the draft specifies none.)
  constexpr iv_storage(const iv_storage& o) noexcept(std::is_nothrow_copy_constructible_v<T>) : iv_storage() {
    iv_construct_from(o.iv_data(), o.n_);
  }

  constexpr iv_storage(iv_storage&&)
    requires std::is_trivially_move_constructible_v<T>
  = default;
  constexpr iv_storage(iv_storage&& o) noexcept(std::is_nothrow_move_constructible_v<T>) : iv_storage() {
    iv_construct_from(o.iv_data(), o.n_);
  }

  constexpr iv_storage& operator=(const iv_storage&)
    requires std::is_trivially_destructible_v<T> && std::is_trivially_copy_constructible_v<T> &&
                 std::is_trivially_copy_assignable_v<T>
  = default;
  constexpr iv_storage& operator=(const iv_storage& o) noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                                                std::is_nothrow_copy_assignable_v<T>) {
    if (this != __builtin_addressof(o)) {
      const T* const q = o.iv_data();
      T* const p = iv_data();
      const std::size_t common = n_ < o.n_ ? n_ : o.n_;
      for (std::size_t i = 0; i != common; ++i)
        p[i] = q[i];
      if (o.n_ > n_)
        iv_construct_from(q, o.n_);
      else
        iv_destroy_from(o.n_);
    }
    return *this;
  }

  constexpr iv_storage& operator=(iv_storage&&)
    requires std::is_trivially_destructible_v<T> && std::is_trivially_move_constructible_v<T> &&
                 std::is_trivially_move_assignable_v<T>
  = default;
  constexpr iv_storage& operator=(iv_storage&& o) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                           std::is_nothrow_move_constructible_v<T>) {
    if (this != __builtin_addressof(o)) {
      T* const q = o.iv_data();
      T* const p = iv_data();
      const std::size_t common = n_ < o.n_ ? n_ : o.n_;
      for (std::size_t i = 0; i != common; ++i)
        p[i] = static_cast<T&&>(q[i]);
      if (o.n_ > n_)
        iv_construct_from(q, o.n_);
      else
        iv_destroy_from(o.n_);
    }
    return *this;
  }

  constexpr ~iv_storage()
    requires std::is_trivially_destructible_v<T>
  = default;
  constexpr ~iv_storage() {
    iv_destroy_from(0);
    if constexpr (heap) {
      if consteval {
        if (cx_)
          std::allocator<T>().deallocate(cx_, N);
      }
    }
  }
};

} // namespace ycxx::adl_free

namespace std {

template <class T, size_t N>
class inplace_vector : ycxx::adl_free::iv_storage<T, N> {
  using base = ycxx::adl_free::iv_storage<T, N>;

public:
  // ---- types ----
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using iterator = ycxx::adl_free::contiguous_iter<T, inplace_vector, difference_type>;
  using const_iterator = ycxx::adl_free::contiguous_iter<const T, inplace_vector, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr void check_room(size_type have, size_type add) {
    if (add > N - have)
      ycxx::detail::throw_bad_alloc();
  }
  // Appends one element; size() < N.
  template <class... Args>
  constexpr T& construct_back(Args&&... args) {
    if constexpr (N == 0) {
      ycxx::detail::throw_bad_alloc(); // unreachable: callers check the room first
    } else {
      this->iv_prepare();
      T* const p = this->iv_data() + this->iv_size();
      std::construct_at(p, static_cast<Args&&>(args)...);
      this->iv_set_size(this->iv_size() + 1);
      return *p;
    }
  }
  // Removes the elements appended since `old` unless released (an exception unwinds past it).
  struct append_guard {
    inplace_vector& v;
    size_type old;
    constexpr ~append_guard() {
      if (old != size_type(-1))
        v.truncate(old);
    }
  };
  constexpr void truncate(size_type n) noexcept {
    if constexpr (N != 0)
      this->iv_destroy_from(n);
  }

  // Appends the n elements of [first, last), or nothing when they do not fit.
  template <class It, class Sent>
  constexpr void append_counted(It first, Sent last, size_type n) {
    check_room(size(), n);
    (void)last;
    append_guard g{*this, size()};
    for (size_type i = 0; i != n; ++i) {
      construct_back(*first);
      ++first;
    }
    g.old = size_type(-1);
  }
  // Appends the elements of a single-pass sequence; bad_alloc (with no effects) when they do
  // not fit.
  template <class It, class Sent>
  constexpr void append_input(It first, Sent last) {
    append_guard g{*this, size()};
    for (; first != last; ++first) {
      check_room(size(), 1);
      construct_back(*first);
    }
    g.old = size_type(-1);
  }
  // Rotates the elements appended since index `old` to index off.
  constexpr iterator move_into_place(size_type off, size_type old) {
    T* const p = data();
    ycxx::detail::rotate_elements<ycxx::detail::plain_temp<T>>(p + off, p + old, p + size());
    return begin() + static_cast<difference_type>(off);
  }
  template <class It, class Sent>
  constexpr void assign_counted(It first, Sent last, size_type n) {
    check_room(0, n);
    T* const p = data();
    const size_type sz = size();
    const size_type common = n < sz ? n : sz;
    for (size_type i = 0; i != common; ++i) {
      p[i] = *first;
      ++first;
    }
    if (n <= sz)
      truncate(n);
    else
      append_counted(static_cast<It&&>(first), static_cast<Sent&&>(last), n - sz);
  }
  template <class It, class Sent>
  constexpr void assign_input(It first, Sent last) {
    T* const p = data();
    size_type i = 0;
    for (; i != size() && first != last; ++first) {
      p[i] = *first;
      ++i;
    }
    if (i != size())
      truncate(i);
    else
      append_input(static_cast<It&&>(first), static_cast<Sent&&>(last));
  }
  template <class R>
  static constexpr bool counted_range = ranges::forward_range<R> || ranges::sized_range<R>;
  // [inplace.vector.cons]/9: Mandates: ranges::size(rg) <= N when it is a constant expression.
  template <class R>
  static constexpr void check_constant_size(R& rg) {
    if constexpr (requires { typename integral_constant<size_t, static_cast<size_t>(ranges::size(rg))>; })
      static_assert(static_cast<size_t>(ranges::size(rg)) <= N,
                    "std::inplace_vector(from_range_t, R&&): the range has more than N elements");
  }
  template <class R>
  constexpr void append_range_impl(R& rg) {
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      append_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      append_input(ranges::begin(rg), ranges::end(rg));
    }
  }

public:
  // ---- [inplace.vector.cons] ----
  constexpr inplace_vector() noexcept = default;
  constexpr explicit inplace_vector(size_type n) {
    check_room(0, n);
    for (size_type i = 0; i != n; ++i)
      construct_back();
  }
  constexpr inplace_vector(size_type n, const T& value) {
    check_room(0, n);
    for (size_type i = 0; i != n; ++i)
      construct_back(value);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr inplace_vector(InputIterator first, InputIterator last) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      append_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      append_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<T> R>
  constexpr inplace_vector(Tag, R&& rg) {
    check_constant_size(rg);
    append_range_impl(rg);
  }
  constexpr inplace_vector(initializer_list<T> il) { append_counted(il.begin(), il.end(), il.size()); }
  // Copy and move construction and assignment and the destructor come from the base.

  constexpr inplace_vector& operator=(initializer_list<T> il) {
    assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void assign(InputIterator first, InputIterator last) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      assign_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      assign_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void assign_range(R&& rg) {
    static_assert(assignable_from<T&, ranges::range_reference_t<R>>,
                  "std::inplace_vector::assign_range: T& must be assignable from the range's reference type");
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      assign_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      assign_input(ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr void assign(size_type n, const T& u) {
    check_room(0, n);
    T* const p = data();
    const size_type sz = size();
    const size_type common = n < sz ? n : sz;
    for (size_type i = 0; i != common; ++i)
      p[i] = u;
    if (n <= sz) {
      truncate(n);
    } else {
      // u may be an element; it is not touched by the appends.
      append_guard g{*this, sz};
      for (size_type i = sz; i != n; ++i)
        construct_back(u);
      g.old = size_type(-1);
    }
  }
  constexpr void assign(initializer_list<T> il) { assign_counted(il.begin(), il.end(), il.size()); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(data()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(data()); }
  constexpr iterator end() noexcept { return iterator(data() + size()); }
  constexpr const_iterator end() const noexcept { return const_iterator(data() + size()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [inplace.vector.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return this->iv_size() == 0; }
  constexpr size_type size() const noexcept { return this->iv_size(); }
  static constexpr size_type max_size() noexcept { return N; }
  static constexpr size_type capacity() noexcept { return N; }
  constexpr void resize(size_type sz) {
    const size_type cur = size();
    if (sz <= cur) {
      truncate(sz);
      return;
    }
    check_room(0, sz);
    append_guard g{*this, cur};
    for (size_type i = cur; i != sz; ++i)
      construct_back();
    g.old = size_type(-1);
  }
  constexpr void resize(size_type sz, const T& c) {
    const size_type cur = size();
    if (sz <= cur) {
      truncate(sz);
      return;
    }
    check_room(0, sz);
    append_guard g{*this, cur};
    for (size_type i = cur; i != sz; ++i)
      construct_back(c);
    g.old = size_type(-1);
  }
  static constexpr void reserve(size_type n) {
    if (n > N)
      ycxx::detail::throw_bad_alloc();
  }
  static constexpr void shrink_to_fit() noexcept {}

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    ycxx::detail::precondition(n < size(), "std::inplace_vector::operator[]: index out of range");
    return data()[n];
  }
  constexpr const_reference operator[](size_type n) const {
    ycxx::detail::precondition(n < size(), "std::inplace_vector::operator[]: index out of range");
    return data()[n];
  }
  constexpr reference at(size_type n) {
    if (n >= size())
      ycxx::detail::throw_out_of_range("std::inplace_vector::at: index out of range");
    return data()[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size())
      ycxx::detail::throw_out_of_range("std::inplace_vector::at: index out of range");
    return data()[n];
  }
  constexpr reference front() {
    ycxx::detail::precondition(size() != 0, "std::inplace_vector::front: empty inplace_vector");
    return data()[0];
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size() != 0, "std::inplace_vector::front: empty inplace_vector");
    return data()[0];
  }
  constexpr reference back() {
    ycxx::detail::precondition(size() != 0, "std::inplace_vector::back: empty inplace_vector");
    return data()[size() - 1];
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size() != 0, "std::inplace_vector::back: empty inplace_vector");
    return data()[size() - 1];
  }

  // ---- [inplace.vector.data] ----
  constexpr T* data() noexcept { return this->iv_data(); }
  constexpr const T* data() const noexcept { return this->iv_data(); }

  // ---- [inplace.vector.modifiers] ----
  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    check_room(size(), 1);
    return construct_back(static_cast<Args&&>(args)...);
  }
  constexpr reference push_back(const T& x) { return emplace_back(x); }
  constexpr reference push_back(T&& x) { return emplace_back(static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void append_range(R&& rg) {
    append_range_impl(rg);
  }
  constexpr void pop_back() {
    ycxx::detail::precondition(size() != 0, "std::inplace_vector::pop_back: empty inplace_vector");
    truncate(size() - 1);
  }

  template <class... Args>
  constexpr optional<reference> try_emplace_back(Args&&... args) {
    if (size() == N)
      return nullopt;
    return optional<reference>(in_place, construct_back(static_cast<Args&&>(args)...));
  }
  constexpr optional<reference> try_push_back(const T& x) { return try_emplace_back(x); }
  constexpr optional<reference> try_push_back(T&& x) { return try_emplace_back(static_cast<T&&>(x)); }
  template <class... Args>
  constexpr reference unchecked_emplace_back(Args&&... args) {
    ycxx::detail::precondition(size() < N, "std::inplace_vector::unchecked_emplace_back: inplace_vector is full");
    return construct_back(static_cast<Args&&>(args)...);
  }
  constexpr reference unchecked_push_back(const T& x) { return unchecked_emplace_back(x); }
  constexpr reference unchecked_push_back(T&& x) { return unchecked_emplace_back(static_cast<T&&>(x)); }

  // Insertions construct the new elements at the end (so arguments that refer to elements stay
  // intact and an exception leaves begin() + [0, size()) as it was), then rotate them into place.
  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args) {
    const size_type off = static_cast<size_type>(position - cbegin());
    check_room(size(), 1);
    const size_type old = size();
    construct_back(static_cast<Args&&>(args)...);
    return move_into_place(off, old);
  }
  constexpr iterator insert(const_iterator position, const T& x) { return emplace(position, x); }
  constexpr iterator insert(const_iterator position, T&& x) { return emplace(position, static_cast<T&&>(x)); }
  constexpr iterator insert(const_iterator position, size_type n, const T& x) {
    const size_type off = static_cast<size_type>(position - cbegin());
    check_room(size(), n);
    const size_type old = size();
    append_guard g{*this, old};
    for (size_type i = 0; i != n; ++i)
      construct_back(x);
    g.old = size_type(-1);
    return move_into_place(off, old);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    const size_type off = static_cast<size_type>(position - cbegin());
    const size_type old = size();
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      append_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      append_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
    return move_into_place(off, old);
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr iterator insert_range(const_iterator position, R&& rg) {
    const size_type off = static_cast<size_type>(position - cbegin());
    const size_type old = size();
    append_range_impl(rg);
    return move_into_place(off, old);
  }
  constexpr iterator insert(const_iterator position, initializer_list<T> il) {
    const size_type off = static_cast<size_type>(position - cbegin());
    const size_type old = size();
    append_counted(il.begin(), il.end(), il.size());
    return move_into_place(off, old);
  }

  constexpr iterator erase(const_iterator position) {
    ycxx::detail::precondition(position != cend(), "std::inplace_vector::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type p = static_cast<size_type>(first - cbegin());
    const size_type q = static_cast<size_type>(last - cbegin());
    if (p != q) {
      T* const d = data();
      const size_type sz = size();
      for (size_type i = q; i != sz; ++i)
        d[p + (i - q)] = static_cast<T&&>(d[i]);
      truncate(sz - (q - p));
    }
    return begin() + static_cast<difference_type>(p);
  }
  constexpr void swap(inplace_vector& x) noexcept(N == 0 ||
                                                  (is_nothrow_swappable_v<T> && is_nothrow_move_constructible_v<T>)) {
    if constexpr (N != 0) {
      if (this == __builtin_addressof(x))
        return;
      inplace_vector& lng = size() < x.size() ? x : *this;
      inplace_vector& shrt = size() < x.size() ? *this : x;
      T* const a = shrt.data();
      T* const b = lng.data();
      const size_type m = shrt.size();
      for (size_type i = 0; i != m; ++i)
        ycxx::detail::swap_adl::do_swap(a[i], b[i]);
      for (size_type i = m; i != lng.size(); ++i)
        shrt.construct_back(static_cast<T&&>(b[i]));
      lng.truncate(m);
    }
  }
  constexpr void clear() noexcept { truncate(0); }

  friend constexpr bool operator==(const inplace_vector& x, const inplace_vector& y) {
    const size_type n = x.size();
    if (n != y.size())
      return false;
    const T* const a = x.data();
    const T* const b = y.data();
    for (size_type i = 0; i != n; ++i)
      if (!static_cast<bool>(a[i] == b[i]))
        return false;
    return true;
  }
  friend constexpr auto operator<=>(const inplace_vector& x, const inplace_vector& y)
    requires requires(const T t) { ycxx::detail::synth_three_way(t, t); }
  {
    using R = ycxx::detail::synth_three_way_result<T>;
    const size_type nx = x.size();
    const size_type ny = y.size();
    const size_type n = nx < ny ? nx : ny;
    const T* const a = x.data();
    const T* const b = y.data();
    for (size_type i = 0; i != n; ++i)
      if (auto c = ycxx::detail::synth_three_way(a[i], b[i]); c != 0)
        return static_cast<R>(c);
    return static_cast<R>(nx <=> ny);
  }
  friend constexpr void swap(inplace_vector& x,
                             inplace_vector& y) noexcept(N == 0 || (is_nothrow_swappable_v<T> &&
                                                                    is_nothrow_move_constructible_v<T>)) {
    x.swap(y);
  }
};

// ---- [inplace.vector.erasure] ----
template <class T, size_t N, class Predicate>
constexpr typename inplace_vector<T, N>::size_type erase_if(inplace_vector<T, N>& c, Predicate pred) {
  T* const base = c.data();
  T* first = base;
  T* const last = base + c.size();
  while (first != last && !static_cast<bool>(pred(*first)))
    ++first;
  T* out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!static_cast<bool>(pred(*first))) {
        *out = static_cast<T&&>(*first);
        ++out;
      }
  }
  const auto r = static_cast<typename inplace_vector<T, N>::size_type>(last - out);
  c.erase(c.begin() + (out - base), c.end());
  return r;
}
template <class T, size_t N, class U = T>
constexpr typename inplace_vector<T, N>::size_type erase(inplace_vector<T, N>& c, const U& value) {
  return std::erase_if(c, [&value](const T& e) { return static_cast<bool>(e == value); });
}

} // namespace std
