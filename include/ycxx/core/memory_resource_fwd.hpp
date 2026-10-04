// libycxx core: declarations from <memory_resource> ([mem.res.syn]) that the containers'
// pmr:: aliases name before <memory_resource> exists.
//
// polymorphic_allocator is declared without its default template argument (Tp = byte): the
// definition in <memory_resource> must supply it, and a default argument may be given only once.
#pragma once

namespace std::pmr {

class memory_resource;
template <class Tp>
class polymorphic_allocator;

} // namespace std::pmr
