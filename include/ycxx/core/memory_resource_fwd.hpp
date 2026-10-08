// libycxx core: declarations from <memory_resource> ([mem.res.syn]) that the containers'
// pmr:: aliases name without including the whole of <memory_resource>.
//
// polymorphic_allocator's default template argument (Tp = byte) is given here and only here: a
// default argument may be specified once, and every declaration, the definition in
// ycxx/core/memory_resource.hpp included, comes after this one.
#pragma once

#include <ycxx/core/cstddef.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace pmr {

class memory_resource;
template <class _Tp_ = byte>
class polymorphic_allocator;

}}} // namespace std::pmr
