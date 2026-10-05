// libycxx core: the result types of the ranges algorithms ([algorithms.results]).
#pragma once

#include <ycxx/core/concepts.hpp>

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

template <class I, class F>
struct in_fun_result {
  [[no_unique_address]] I in;
  [[no_unique_address]] F fun;

  template <class I2, class F2>
    requires convertible_to<const I&, I2> && convertible_to<const F&, F2>
  constexpr operator in_fun_result<I2, F2>() const& {
    return {in, fun};
  }
  template <class I2, class F2>
    requires convertible_to<I, I2> && convertible_to<F, F2>
  constexpr operator in_fun_result<I2, F2>() && {
    return {std::move(in), std::move(fun)};
  }
};

template <class I1, class I2>
struct in_in_result {
  [[no_unique_address]] I1 in1;
  [[no_unique_address]] I2 in2;

  template <class II1, class II2>
    requires convertible_to<const I1&, II1> && convertible_to<const I2&, II2>
  constexpr operator in_in_result<II1, II2>() const& {
    return {in1, in2};
  }
  template <class II1, class II2>
    requires convertible_to<I1, II1> && convertible_to<I2, II2>
  constexpr operator in_in_result<II1, II2>() && {
    return {std::move(in1), std::move(in2)};
  }
};

template <class I, class O>
struct in_out_result {
  [[no_unique_address]] I in;
  [[no_unique_address]] O out;

  template <class I2, class O2>
    requires convertible_to<const I&, I2> && convertible_to<const O&, O2>
  constexpr operator in_out_result<I2, O2>() const& {
    return {in, out};
  }
  template <class I2, class O2>
    requires convertible_to<I, I2> && convertible_to<O, O2>
  constexpr operator in_out_result<I2, O2>() && {
    return {std::move(in), std::move(out)};
  }
};

template <class I1, class I2, class O>
struct in_in_out_result {
  [[no_unique_address]] I1 in1;
  [[no_unique_address]] I2 in2;
  [[no_unique_address]] O out;

  template <class II1, class II2, class OO>
    requires convertible_to<const I1&, II1> && convertible_to<const I2&, II2> && convertible_to<const O&, OO>
  constexpr operator in_in_out_result<II1, II2, OO>() const& {
    return {in1, in2, out};
  }
  template <class II1, class II2, class OO>
    requires convertible_to<I1, II1> && convertible_to<I2, II2> && convertible_to<O, OO>
  constexpr operator in_in_out_result<II1, II2, OO>() && {
    return {std::move(in1), std::move(in2), std::move(out)};
  }
};

template <class I, class O1, class O2>
struct in_out_out_result {
  [[no_unique_address]] I in;
  [[no_unique_address]] O1 out1;
  [[no_unique_address]] O2 out2;

  template <class II, class OO1, class OO2>
    requires convertible_to<const I&, II> && convertible_to<const O1&, OO1> && convertible_to<const O2&, OO2>
  constexpr operator in_out_out_result<II, OO1, OO2>() const& {
    return {in, out1, out2};
  }
  template <class II, class OO1, class OO2>
    requires convertible_to<I, II> && convertible_to<O1, OO1> && convertible_to<O2, OO2>
  constexpr operator in_out_out_result<II, OO1, OO2>() && {
    return {std::move(in), std::move(out1), std::move(out2)};
  }
};

template <class T>
struct min_max_result {
  [[no_unique_address]] T min;
  [[no_unique_address]] T max;

  template <class T2>
    requires convertible_to<const T&, T2>
  constexpr operator min_max_result<T2>() const& {
    return {min, max};
  }
  template <class T2>
    requires convertible_to<T, T2>
  constexpr operator min_max_result<T2>() && {
    return {std::move(min), std::move(max)};
  }
};

template <class I>
struct in_found_result {
  [[no_unique_address]] I in;
  bool found;

  template <class I2>
    requires convertible_to<const I&, I2>
  constexpr operator in_found_result<I2>() const& {
    return {in, found};
  }
  template <class I2>
    requires convertible_to<I, I2>
  constexpr operator in_found_result<I2>() && {
    return {std::move(in), found};
  }
};

template <class I, class T>
struct in_value_result {
  [[no_unique_address]] I in;
  [[no_unique_address]] T value;

  template <class I2, class T2>
    requires convertible_to<const I&, I2> && convertible_to<const T&, T2>
  constexpr operator in_value_result<I2, T2>() const& {
    return {in, value};
  }
  template <class I2, class T2>
    requires convertible_to<I, I2> && convertible_to<T, T2>
  constexpr operator in_value_result<I2, T2>() && {
    return {std::move(in), std::move(value)};
  }
};

template <class O, class T>
struct out_value_result {
  [[no_unique_address]] O out;
  [[no_unique_address]] T value;

  template <class O2, class T2>
    requires convertible_to<const O&, O2> && convertible_to<const T&, T2>
  constexpr operator out_value_result<O2, T2>() const& {
    return {out, value};
  }
  template <class O2, class T2>
    requires convertible_to<O, O2> && convertible_to<T, T2>
  constexpr operator out_value_result<O2, T2>() && {
    return {std::move(out), std::move(value)};
  }
};

}} // namespace std::ranges
