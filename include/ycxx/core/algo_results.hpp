// libycxx core: the result types of the ranges algorithms ([algorithms.results]).
#pragma once

#include <ycxx/core/concepts.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <class _Ip, class _Fp>
struct in_fun_result {
  [[no_unique_address]] _Ip in;
  [[no_unique_address]] _Fp fun;

  template <class _I2, class _F2>
    requires convertible_to<const _Ip&, _I2> && convertible_to<const _Fp&, _F2>
  constexpr operator in_fun_result<_I2, _F2>() const& {
    return {in, fun};
  }
  template <class _I2, class _F2>
    requires convertible_to<_Ip, _I2> && convertible_to<_Fp, _F2>
  constexpr operator in_fun_result<_I2, _F2>() && {
    return {std::move(in), std::move(fun)};
  }
};

template <class _I1, class _I2>
struct in_in_result {
  [[no_unique_address]] _I1 in1;
  [[no_unique_address]] _I2 in2;

  template <class _II1, class _II2>
    requires convertible_to<const _I1&, _II1> && convertible_to<const _I2&, _II2>
  constexpr operator in_in_result<_II1, _II2>() const& {
    return {in1, in2};
  }
  template <class _II1, class _II2>
    requires convertible_to<_I1, _II1> && convertible_to<_I2, _II2>
  constexpr operator in_in_result<_II1, _II2>() && {
    return {std::move(in1), std::move(in2)};
  }
};

template <class _Ip, class _Op>
struct in_out_result {
  [[no_unique_address]] _Ip in;
  [[no_unique_address]] _Op out;

  template <class _I2, class _O2>
    requires convertible_to<const _Ip&, _I2> && convertible_to<const _Op&, _O2>
  constexpr operator in_out_result<_I2, _O2>() const& {
    return {in, out};
  }
  template <class _I2, class _O2>
    requires convertible_to<_Ip, _I2> && convertible_to<_Op, _O2>
  constexpr operator in_out_result<_I2, _O2>() && {
    return {std::move(in), std::move(out)};
  }
};

template <class _I1, class _I2, class _Op>
struct in_in_out_result {
  [[no_unique_address]] _I1 in1;
  [[no_unique_address]] _I2 in2;
  [[no_unique_address]] _Op out;

  template <class _II1, class _II2, class _OO>
    requires convertible_to<const _I1&, _II1> && convertible_to<const _I2&, _II2> && convertible_to<const _Op&, _OO>
  constexpr operator in_in_out_result<_II1, _II2, _OO>() const& {
    return {in1, in2, out};
  }
  template <class _II1, class _II2, class _OO>
    requires convertible_to<_I1, _II1> && convertible_to<_I2, _II2> && convertible_to<_Op, _OO>
  constexpr operator in_in_out_result<_II1, _II2, _OO>() && {
    return {std::move(in1), std::move(in2), std::move(out)};
  }
};

template <class _Ip, class _O1, class _O2>
struct in_out_out_result {
  [[no_unique_address]] _Ip in;
  [[no_unique_address]] _O1 out1;
  [[no_unique_address]] _O2 out2;

  template <class _II, class _OO1, class _OO2>
    requires convertible_to<const _Ip&, _II> && convertible_to<const _O1&, _OO1> && convertible_to<const _O2&, _OO2>
  constexpr operator in_out_out_result<_II, _OO1, _OO2>() const& {
    return {in, out1, out2};
  }
  template <class _II, class _OO1, class _OO2>
    requires convertible_to<_Ip, _II> && convertible_to<_O1, _OO1> && convertible_to<_O2, _OO2>
  constexpr operator in_out_out_result<_II, _OO1, _OO2>() && {
    return {std::move(in), std::move(out1), std::move(out2)};
  }
};

template <class _Tp>
struct min_max_result {
  [[no_unique_address]] _Tp min;
  [[no_unique_address]] _Tp max;

  template <class _T2>
    requires convertible_to<const _Tp&, _T2>
  constexpr operator min_max_result<_T2>() const& {
    return {min, max};
  }
  template <class _T2>
    requires convertible_to<_Tp, _T2>
  constexpr operator min_max_result<_T2>() && {
    return {std::move(min), std::move(max)};
  }
};

template <class _Ip>
struct in_found_result {
  [[no_unique_address]] _Ip in;
  bool found;

  template <class _I2>
    requires convertible_to<const _Ip&, _I2>
  constexpr operator in_found_result<_I2>() const& {
    return {in, found};
  }
  template <class _I2>
    requires convertible_to<_Ip, _I2>
  constexpr operator in_found_result<_I2>() && {
    return {std::move(in), found};
  }
};

template <class _Ip, class _Tp>
struct in_value_result {
  [[no_unique_address]] _Ip in;
  [[no_unique_address]] _Tp value;

  template <class _I2, class _T2>
    requires convertible_to<const _Ip&, _I2> && convertible_to<const _Tp&, _T2>
  constexpr operator in_value_result<_I2, _T2>() const& {
    return {in, value};
  }
  template <class _I2, class _T2>
    requires convertible_to<_Ip, _I2> && convertible_to<_Tp, _T2>
  constexpr operator in_value_result<_I2, _T2>() && {
    return {std::move(in), std::move(value)};
  }
};

template <class _Op, class _Tp>
struct out_value_result {
  [[no_unique_address]] _Op out;
  [[no_unique_address]] _Tp value;

  template <class _O2, class _T2>
    requires convertible_to<const _Op&, _O2> && convertible_to<const _Tp&, _T2>
  constexpr operator out_value_result<_O2, _T2>() const& {
    return {out, value};
  }
  template <class _O2, class _T2>
    requires convertible_to<_Op, _O2> && convertible_to<_Tp, _T2>
  constexpr operator out_value_result<_O2, _T2>() && {
    return {std::move(out), std::move(value)};
  }
};

}} // namespace std::ranges
