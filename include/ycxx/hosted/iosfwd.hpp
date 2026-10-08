// libycxx hosted: the forward declarations of <iosfwd> ([iosfwd.syn]).
//
// This is the one declaration of each stream template that carries its default template
// arguments ([iosfwd.syn]/1: the defaults also appear in the other headers' synopses, but must be
// declared only once). Core declares basic_ios, basic_streambuf, basic_istream, basic_ostream and
// basic_iostream without defaults (ycxx/core/iosfwd.hpp) and the two stream-buffer iterators with
// theirs; the class definitions repeat none of them.
#pragma once

#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/iosfwd.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
class allocator;

template <class __charT, class __traits = char_traits<__charT>>
class basic_ios;
template <class __charT, class __traits = char_traits<__charT>>
class basic_streambuf;
template <class __charT, class __traits = char_traits<__charT>>
class basic_istream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_ostream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_iostream;

template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_stringbuf;
template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_istringstream;
template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_ostringstream;
template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_stringstream;

template <class __charT, class __traits = char_traits<__charT>>
class basic_spanbuf;
template <class __charT, class __traits = char_traits<__charT>>
class basic_ispanstream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_ospanstream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_spanstream;

template <class __charT, class __traits = char_traits<__charT>>
class basic_filebuf;
template <class __charT, class __traits = char_traits<__charT>>
class basic_ifstream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_ofstream;
template <class __charT, class __traits = char_traits<__charT>>
class basic_fstream;

template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_syncbuf;
template <class __charT, class __traits = char_traits<__charT>, class _Allocator = allocator<__charT>>
class basic_osyncstream;

using ios = basic_ios<char>;
using wios = basic_ios<wchar_t>;

using streambuf = basic_streambuf<char>;
using istream = basic_istream<char>;
using ostream = basic_ostream<char>;
using iostream = basic_iostream<char>;

using stringbuf = basic_stringbuf<char>;
using istringstream = basic_istringstream<char>;
using ostringstream = basic_ostringstream<char>;
using stringstream = basic_stringstream<char>;

using spanbuf = basic_spanbuf<char>;
using ispanstream = basic_ispanstream<char>;
using ospanstream = basic_ospanstream<char>;
using spanstream = basic_spanstream<char>;

using filebuf = basic_filebuf<char>;
using ifstream = basic_ifstream<char>;
using ofstream = basic_ofstream<char>;
using fstream = basic_fstream<char>;

using syncbuf = basic_syncbuf<char>;
using osyncstream = basic_osyncstream<char>;

using wstreambuf = basic_streambuf<wchar_t>;
using wistream = basic_istream<wchar_t>;
using wostream = basic_ostream<wchar_t>;
using wiostream = basic_iostream<wchar_t>;

using wstringbuf = basic_stringbuf<wchar_t>;
using wistringstream = basic_istringstream<wchar_t>;
using wostringstream = basic_ostringstream<wchar_t>;
using wstringstream = basic_stringstream<wchar_t>;

using wspanbuf = basic_spanbuf<wchar_t>;
using wispanstream = basic_ispanstream<wchar_t>;
using wospanstream = basic_ospanstream<wchar_t>;
using wspanstream = basic_spanstream<wchar_t>;

using wfilebuf = basic_filebuf<wchar_t>;
using wifstream = basic_ifstream<wchar_t>;
using wofstream = basic_ofstream<wchar_t>;
using wfstream = basic_fstream<wchar_t>;

using wsyncbuf = basic_syncbuf<wchar_t>;
using wosyncstream = basic_osyncstream<wchar_t>;

// fpos and the streampos aliases are declared with char_traits (core).

}} // namespace std
