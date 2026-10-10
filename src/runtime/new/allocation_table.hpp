// libycxx runtime: the allocation table, which joins the default allocation functions of every
// image of a process that links libycxx (DECISIONS §2, "The allocation table").
//
// libycxx's default allocation functions are hidden (hidden.hpp), so they are never bound to, nor
// bind to, another C++ runtime's (Apple's libc++abi, libstdc++). The images that link libycxx
// still share one set: each holds `__ycxx_allocation_functions`, a weak, exported table under a name
// no other runtime defines, and the dynamic linker makes every image use the first image's table
// (the program's, when it links libycxx). Its entries call that image's `::operator new` ...
// `::operator delete[]` (thunks, below): the program's replacements where it has them, else its
// defaults. Each default allocation function first looks up its entry: if the entry is not this
// image's own thunk, it forwards there; otherwise it is the process's default and does the work.
// So a replacement in the program serves every libycxx image ([replacement.functions]/2), and an
// object allocated in one image is freed by the same functions in another.
//
// The thunks have plain `size_t`/`void*` signatures: a function taking std::align_val_t or
// std::nothrow_t would take those (hidden) types' visibility, and so would a table typed with
// them. Their addresses are constant expressions, so the table is constant-initialized and usable
// from the first static constructor.
#pragma once

#include <cstddef>

// The table also carries the ABI version of the libycxx that built its image (__abi, first, and
// at that offset in every version): each image checks, from an initializer, that the process's
// table has its own version, and aborts naming both otherwise (allocation_table.cpp; DECISIONS
// §20.6). Images of different libycxx versions disagree on layouts, so they must not share objects.
// News take (size, alignment); the alignment of the forms without one is ignored. Deletes take
// (pointer, size, alignment); the size or alignment of the forms without them is ignored.
struct __ycxx_allocation_functions_t {
  const char* __abi;
  void* (*__new_)(std::size_t, std::size_t);
  void* (*__new_align)(std::size_t, std::size_t);
  void* (*__new_nothrow)(std::size_t, std::size_t) noexcept;
  void* (*__new_align_nothrow)(std::size_t, std::size_t) noexcept;
  void* (*__new_array)(std::size_t, std::size_t);
  void* (*__new_array_align)(std::size_t, std::size_t);
  void* (*__new_array_nothrow)(std::size_t, std::size_t) noexcept;
  void* (*__new_array_align_nothrow)(std::size_t, std::size_t) noexcept;
  void (*__delete_)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_sized)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_align)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_sized_align)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_nothrow)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_align_nothrow)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array_sized)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array_align)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array_sized_align)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array_nothrow)(void*, std::size_t, std::size_t) noexcept;
  void (*__delete_array_align_nothrow)(void*, std::size_t, std::size_t) noexcept;
};

// The process's table (allocation_table.cpp; the first image's, see above). Declared with default
// visibility like its definition, explicitly: an ELF linker gives a symbol the most restrictive
// visibility of all its references, so one hidden declaration would keep the table inside each
// image.
extern "C" [[__gnu__::__visibility__("default")]] const __ycxx_allocation_functions_t __ycxx_allocation_functions;
// What the link options name as undefined to pull the table's member into a program
// (allocation_table.cpp, cmake/ycxx-link.cmake).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_table_anchor;

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// This image's own table, the one its `__ycxx_allocation_functions` definition holds; an entry of
// the process's table equal to this image's entry means this image provides that function.
extern const __ycxx_allocation_functions_t __own_allocation_functions;
}} // namespace __ycxx::__detail
