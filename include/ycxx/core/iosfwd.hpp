// libycxx core: the iostreams class templates that core headers name without needing their
// definitions ([iosfwd.syn]).
//
// Core headers declare the stream operators of their own types against these declarations (the
// draft declares `operator<<` for basic_string, basic_string_view, bitset, unique_ptr, ... in
// those types' headers), so <string> does not include <ostream>. The stream templates are
// declared here WITHOUT default template arguments: <iosfwd> (ycxx/hosted/iosfwd.hpp) supplies
// them, exactly once. The two stream-buffer iterators are the exception: <iterator> declares
// them with their default argument ([iterator.synopsis]), so this file does, and <iosfwd> does
// not repeat it.
#pragma once

#include <ycxx/core/char_traits.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class charT, class traits>
class basic_ios;
template <class charT, class traits>
class basic_streambuf;
template <class charT, class traits>
class basic_istream;
template <class charT, class traits>
class basic_ostream;
template <class charT, class traits>
class basic_iostream;

template <class charT, class traits = char_traits<charT>>
class istreambuf_iterator;
template <class charT, class traits = char_traits<charT>>
class ostreambuf_iterator;

} // namespace std
