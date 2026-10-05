// A program-defined formatter written with only <vector> available, delegating to
// formatter<vector<bool>::reference> ([vector.bool.fmt]); included before or after <format>.
#pragma once
#include <vector>

struct Flag {
  std::vector<bool>::reference r;
};

template <>
struct std::formatter<Flag> {
  std::formatter<std::vector<bool>::reference> f;
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& pc) {
    return f.parse(pc);
  }
  template <class FormatContext>
  typename FormatContext::iterator format(const Flag& x, FormatContext& ctx) const {
    return f.format(x.r, ctx);
  }
};

// Formatter objects made in a translation unit without <format> (tu_e.cpp), used by one with it.
std::formatter<int> tu_e_int_formatter();
std::formatter<std::vector<bool>::reference> tu_e_ref_formatter();
