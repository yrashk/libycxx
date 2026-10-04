// libycxx core: <mdspan> part 1, extents ([mdspan.extents]) and the integer helpers that the
// layouts, mdspan and submdspan share.
//
// An extents object stores only its dynamic extents (an array of index_type, nothing for a
// fully static extents). Static extents and the dynamic-index tables are static constexpr
// arrays. Preconditions on index values are checked during constant evaluation and when
// hardened (DECISIONS §1.7); the checks that loop are skipped otherwise (md_checking()).
#pragma once

#include <ycxx/core/move.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/utility_base.hpp>

namespace std {
template <class IndexType, size_t... Extents>
class extents;
} // namespace std

namespace ycxx::detail {

// True when a precondition that costs a loop is worth evaluating.
[[gnu::always_inline]] constexpr bool md_checking() noexcept {
  if consteval {
    return true;
  } else {
    return cfg::hardened;
  }
}

// [mdspan.extents.overview]/1: "a signed or unsigned integer type".
template <class T>
concept md_index_type = is_signed_or_unsigned_integer<T> && std::is_same_v<T, std::remove_cv_t<T>>;

// An integral value as a signed or unsigned integer type (the character types map to the
// integer type of the same signedness and width), so that std::cmp_* accept it.
template <class T>
constexpr auto md_as_int(T v) noexcept {
  if constexpr (is_signed_or_unsigned_integer<T>)
    return v;
  else if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>)
    return static_cast<unsigned char>(v);
  else if constexpr (std::is_signed_v<T>)
    return static_cast<std::make_signed_t<T>>(v);
  else
    return static_cast<std::make_unsigned_t<T>>(v);
}

template <class T>
concept md_plain_integral = std::is_integral_v<std::remove_cvref_t<T>> && !std::is_same_v<std::remove_cvref_t<T>, bool>;

// index-cast ([mdspan.extents.expo]/9).
template <class IndexType, class O>
constexpr auto md_index_cast(O&& i) noexcept {
  if constexpr (md_plain_integral<O>)
    return i;
  else
    return static_cast<IndexType>(static_cast<O&&>(i));
}

// 0 <= i < n for an integral i of any type and a nonnegative n.
template <class I, class N>
constexpr bool md_in_interval(I i, N n) noexcept {
  auto v = ::ycxx::detail::md_as_int(i);
  return std::cmp_greater_equal(v, 0) && std::cmp_less(v, ::ycxx::detail::md_as_int(n));
}

// "v is representable as a nonnegative value of type IndexType" for an integral v.
template <class IndexType, class T>
constexpr bool md_nonneg_representable(T v) noexcept {
  auto x = ::ycxx::detail::md_as_int(v);
  return std::cmp_greater_equal(x, 0) && std::in_range<IndexType>(x);
}

// Converts one index argument to IndexType and folds "it lies in [0, n)" into ok. An integral
// argument is checked before the conversion (index-cast), any other after it.
template <class IndexType, class O>
constexpr IndexType md_cast_index(O&& v, IndexType n, bool& ok) noexcept {
  if constexpr (md_plain_integral<O>) {
    ok = ok && ::ycxx::detail::md_in_interval(v, n);
    return static_cast<IndexType>(v);
  } else {
    IndexType x = static_cast<IndexType>(static_cast<O&&>(v));
    ok = ok && ::ycxx::detail::md_in_interval(x, n);
    return x;
  }
}

// a * b and a + b, reporting overflow of T (the operands are nonnegative).
template <class T>
constexpr bool md_mul(T a, T b, T& r) noexcept {
  return !__builtin_mul_overflow(a, b, &r);
}
template <class T>
constexpr bool md_add(T a, T b, T& r) noexcept {
  return !__builtin_add_overflow(a, b, &r);
}

// LEAST-MULTIPLE-AT-LEAST(x, y) ([mdspan.layout.general]/2.4) in T; false on overflow.
template <class T>
constexpr bool md_least_multiple(T x, T y, T& r) noexcept {
  if (x == 0) {
    r = y;
    return true;
  }
  T q = y / x + (y % x != 0 ? 1 : 0);
  return ::ycxx::detail::md_mul(q, x, r);
}

// Precondition on one extent given to an extents constructor: an integral value is
// representable as a nonnegative index_type; any other is nonnegative after conversion.
template <class IndexType, class O>
constexpr bool md_extent_ok(const O& v, IndexType converted) noexcept {
  if constexpr (md_plain_integral<O>)
    return ::ycxx::detail::md_nonneg_representable<IndexType>(v);
  else
    return std::cmp_greater_equal(converted, 0);
}

// The static extents of extents<I, E...>, dynamic-index(i) for i in [0, rank()] and
// dynamic-index-inv(i) for i in [0, rank_dynamic()).
template <std::size_t Rank, std::size_t RankDynamic>
struct md_ext_index_tables {
  std::size_t index[Rank + 1];
  std::size_t inv[RankDynamic + 1];
};
template <std::size_t RankDynamic, std::size_t... E>
consteval md_ext_index_tables<sizeof...(E), RankDynamic> md_make_ext_tables() {
  md_ext_index_tables<sizeof...(E), RankDynamic> t{};
  const std::size_t statics[] = {E..., 0};
  std::size_t n = 0;
  for (std::size_t r = 0; r < sizeof...(E); ++r) {
    t.index[r] = n;
    if (statics[r] == std::dynamic_extent)
      t.inv[n++] = r;
  }
  t.index[sizeof...(E)] = n;
  return t;
}
template <std::size_t... E>
struct md_ext_tables {
  static constexpr std::size_t rank_dynamic = ((E == std::dynamic_extent ? 1 : 0) + ... + 0);
  static constexpr std::size_t statics[sizeof...(E) + 1] = {E..., 0};
  static constexpr md_ext_index_tables<sizeof...(E), rank_dynamic> tables =
      ::ycxx::detail::md_make_ext_tables<rank_dynamic, E...>();
};

template <class T>
inline constexpr bool md_is_extents = false;
template <class IndexType, std::size_t... E>
inline constexpr bool md_is_extents<std::extents<IndexType, E...>> = true;

// The dynamic extents: an array of index_type, nothing when every extent is static.
template <class I, std::size_t N>
struct md_dyn_store {
  I v[N];
};
template <class I>
struct md_dyn_store<I, 0> {};

} // namespace ycxx::detail

namespace std {

template <class IndexType, size_t... Extents>
class extents {
  static_assert(ycxx::detail::md_index_type<IndexType>,
                "std::extents: IndexType must be a signed or unsigned integer type");
  static_assert(((Extents == dynamic_extent || in_range<IndexType>(Extents)) && ...),
                "std::extents: every static extent must be representable as a value of IndexType");

public:
  using index_type = IndexType;
  using size_type = make_unsigned_t<index_type>;
  using rank_type = size_t;

private:
  using tables_t = ycxx::detail::md_ext_tables<Extents...>;
  static constexpr const size_t* static_exts_ = tables_t::statics;
  static constexpr rank_type rank_dyn_ = tables_t::rank_dynamic;

  static constexpr rank_type dynamic_index(rank_type i) noexcept { return tables_t::tables.index[i]; }
  static constexpr rank_type dynamic_index_inv(rank_type i) noexcept { return tables_t::tables.inv[i]; }

  // Constraints and explicitness of the converting constructor ([mdspan.extents.cons]/1, /4).
  template <size_t... Other>
  static consteval bool compatible() {
    if constexpr (sizeof...(Other) != sizeof...(Extents))
      return false;
    else
      return ((Other == dynamic_extent || Extents == dynamic_extent || Other == Extents) && ...);
  }
  template <class OtherIndexType, size_t... Other>
  static consteval bool explicit_from() {
    if constexpr (sizeof...(Other) != sizeof...(Extents))
      return false;
    else
      return (((Extents != dynamic_extent) && (Other == dynamic_extent)) || ...) ||
             cmp_less(numeric_limits<index_type>::max(), numeric_limits<OtherIndexType>::max());
  }

  // Stores the dynamic extents from n values; get(r) yields the value for rank index r of the
  // full list (n == rank()) or for the r-th dynamic extent (n == rank_dynamic()).
  template <class Get>
  constexpr void store(size_t n, Get get) noexcept {
    if constexpr (rank_dyn_ != 0) {
      for (size_t d = 0; d < rank_dyn_; ++d)
        dyn_.v[d] = get(n == rank_dyn_ ? d : dynamic_index_inv(d));
    }
  }
  // Precondition: the given value for each static extent equals it.
  template <class Get>
  static constexpr bool statics_match(size_t n, Get get) noexcept {
    if (n == rank_dyn_)
      return true;
    for (size_t r = 0; r < sizeof...(Extents); ++r)
      if (static_exts_[r] != dynamic_extent && !cmp_equal(get(r), static_exts_[r]))
        return false;
    return true;
  }

public:
  // [mdspan.extents.obs]
  static constexpr rank_type rank() noexcept { return sizeof...(Extents); }
  static constexpr rank_type rank_dynamic() noexcept { return rank_dyn_; }
  static constexpr size_t static_extent(rank_type i) noexcept {
    ycxx::detail::precondition(i < rank(), "std::extents::static_extent: index out of range");
    return static_exts_[i];
  }
  constexpr index_type extent(rank_type i) const noexcept {
    ycxx::detail::precondition(i < rank(), "std::extents::extent: index out of range");
    if constexpr (rank_dyn_ == 0) {
      return static_cast<index_type>(static_exts_[i]);
    } else {
      if (static_exts_[i] == dynamic_extent)
        return dyn_.v[dynamic_index(i)];
      return static_cast<index_type>(static_exts_[i]);
    }
  }

  // [mdspan.extents.cons]
  constexpr extents() noexcept = default;

  template <class OtherIndexType, size_t... OtherExtents>
    requires(compatible<OtherExtents...>())
  constexpr explicit(explicit_from<OtherIndexType, OtherExtents...>())
      extents(const extents<OtherIndexType, OtherExtents...>& other) noexcept {
    if (ycxx::detail::md_checking()) {
      for (size_t r = 0; r < rank(); ++r) {
        ycxx::detail::precondition(static_exts_[r] == dynamic_extent || cmp_equal(other.extent(r), static_exts_[r]),
                                   "std::extents: a static extent differs from the source extent");
        ycxx::detail::precondition(in_range<index_type>(other.extent(r)),
                                   "std::extents: source extent not representable as index_type");
      }
    }
    store(rank(), [&](size_t r) { return static_cast<index_type>(other.extent(r)); });
  }

  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) &&
             (sizeof...(OtherIndexTypes) == rank_dyn_ || sizeof...(OtherIndexTypes) == rank()))
  constexpr explicit extents(OtherIndexTypes... exts) noexcept {
    constexpr size_t n = sizeof...(OtherIndexTypes);
    array<index_type, n> arr{static_cast<index_type>(std::move(exts))...};
    if (ycxx::detail::md_checking()) {
      if constexpr (n != 0) {
        size_t r = 0;
        bool ok = true;
        ((ok = ok && ycxx::detail::md_extent_ok<index_type>(exts, arr[r]), ++r), ...);
        ycxx::detail::precondition(ok, "std::extents: an extent is negative or not representable as index_type");
      }
      ycxx::detail::precondition(statics_match(n, [&](size_t r) { return arr[r]; }),
                                 "std::extents: a given extent differs from the static extent");
    }
    store(n, [&](size_t r) { return arr[r]; });
  }

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&> && (N == rank_dyn_ || N == rank()))
  constexpr explicit(N != rank_dyn_) extents(span<OtherIndexType, N> exts) noexcept {
    init_from_array<N>(exts);
  }
  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<const OtherIndexType&, index_type> &&
             is_nothrow_constructible_v<index_type, const OtherIndexType&> && (N == rank_dyn_ || N == rank()))
  constexpr explicit(N != rank_dyn_) extents(const array<OtherIndexType, N>& exts) noexcept {
    init_from_array<N>(exts);
  }

  // [mdspan.extents.cmp]
  template <class OtherIndexType, size_t... OtherExtents>
  friend constexpr bool operator==(const extents& lhs, const extents<OtherIndexType, OtherExtents...>& rhs) noexcept {
    if constexpr (sizeof...(OtherExtents) != sizeof...(Extents)) {
      return false;
    } else {
      for (size_t r = 0; r < rank(); ++r)
        if (!cmp_equal(lhs.extent(r), rhs.extent(r)))
          return false;
      return true;
    }
  }

private:
  template <size_t n, class Exts>
  constexpr void init_from_array(const Exts& exts) noexcept {
    if (ycxx::detail::md_checking()) {
      for (size_t r = 0; r < n; ++r)
        ycxx::detail::precondition(
            ycxx::detail::md_extent_ok<index_type>(as_const(exts[r]), static_cast<index_type>(as_const(exts[r]))),
            "std::extents: an extent is negative or not representable as index_type");
      ycxx::detail::precondition(statics_match(n, [&](size_t r) { return static_cast<index_type>(as_const(exts[r])); }),
                                 "std::extents: a given extent differs from the static extent");
    }
    store(n, [&](size_t r) { return static_cast<index_type>(as_const(exts[r])); });
  }

  [[no_unique_address]] ycxx::detail::md_dyn_store<index_type, rank_dyn_> dyn_{};
};

template <class... Integrals>
  requires(is_convertible_v<Integrals, size_t> && ...)
explicit extents(Integrals...) -> extents<size_t, ycxx::detail::maybe_static_ext<Integrals>...>;

} // namespace std

namespace ycxx::detail {

template <class IndexType, class Seq>
struct md_dextents;
template <class IndexType, std::size_t... I>
struct md_dextents<IndexType, std::index_sequence<I...>> {
  using type = std::extents<IndexType, ((void)I, std::dynamic_extent)...>;
};

} // namespace ycxx::detail

namespace std {

// [mdspan.extents.dextents], [mdspan.extents.dims]
template <class IndexType, size_t Rank>
using dextents = typename ycxx::detail::md_dextents<IndexType, make_index_sequence<Rank>>::type;
template <size_t Rank, class IndexType = size_t>
using dims = dextents<IndexType, Rank>;

} // namespace std

namespace ycxx::detail {

// fwd-prod-of-extents(i) and rev-prod-of-extents(i) ([mdspan.extents.expo]/5-8).
template <class E>
constexpr std::size_t md_fwd_prod(const E& e, std::size_t i) noexcept {
  std::size_t p = 1;
  for (std::size_t k = 0; k < i; ++k)
    p *= static_cast<std::size_t>(e.extent(k));
  return p;
}
template <class E>
constexpr std::size_t md_rev_prod(const E& e, std::size_t i) noexcept {
  std::size_t p = 1;
  for (std::size_t k = i + 1; k < E::rank(); ++k)
    p *= static_cast<std::size_t>(e.extent(k));
  return p;
}

// "The size of the multidimensional index space e is representable as a value of type T."
template <class T, class E>
constexpr bool md_size_fits(const E& e) noexcept {
  for (std::size_t r = 0; r < E::rank(); ++r)
    if (e.extent(r) == 0)
      return true;
  T p = 1;
  for (std::size_t r = 0; r < E::rank(); ++r) {
    if (!std::in_range<T>(e.extent(r)) || !::ycxx::detail::md_mul(p, static_cast<T>(e.extent(r)), p))
      return false;
  }
  return true;
}

// The Mandates of the standard layouts: a fully static index space has a representable size.
template <class E>
consteval bool md_static_size_fits() {
  if constexpr (E::rank_dynamic() != 0)
    return true;
  else
    return ::ycxx::detail::md_size_fits<typename E::index_type>(E());
}

// "I... is a multidimensional index in e" for integral (index-cast) values.
template <class E, class... I>
constexpr bool md_is_index(const E& e, I... i) noexcept {
  std::size_t r = 0;
  bool ok = true;
  ((ok = ok && ::ycxx::detail::md_in_interval(i, e.extent(r)), ++r), ...);
  return ok;
}

} // namespace ycxx::detail
