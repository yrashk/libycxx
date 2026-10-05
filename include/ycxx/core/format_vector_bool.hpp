// libycxx core: formatter<vector<bool>::reference> ([vector.bool.fmt]), which <vector> declares
// ([vector.syn]). Its parse and format forward to formatter<bool> (format_decl.hpp), whose
// member bodies need the formatting library only when they are used.
#pragma once

#include <ycxx/core/format_decl.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Word>
class bit_ref;
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class T>
inline constexpr bool fmt_is_bit_ref = false;
template <class Word>
inline constexpr bool fmt_is_bit_ref<ycxx::adl_free::bit_ref<Word>> = true;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T, class charT>
  requires ycxx::detail::fmt_is_bit_ref<T>
struct formatter<T, charT> {
private:
  formatter<bool, charT> underlying_;

public:
  template <class ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const T& ref, FormatContext& ctx) const {
    return underlying_.format(ref, ctx);
  }
};
// [format.formatter.spec]/3: not specified otherwise.
template <class Word>
inline constexpr bool enable_nonlocking_formatter_optimization<ycxx::adl_free::bit_ref<Word>> = true;

} // namespace std
