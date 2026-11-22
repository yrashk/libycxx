// REQUIRES: !exceptions
// UNSUPPORTED-SANITIZER: asan ASan replaces allocation functions and their forwarding behavior.
// [new.delete.single], [new.delete.array]
#include "nothrow-replacements/forwarding.hpp"

using nothrow_probe::Entry;

void* operator new[](std::size_t size) {
    return nothrow_probe::record_ordinary(Entry::ordinary_array, size);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return nothrow_probe::record_ordinary(Entry::ordinary_array, size, static_cast<std::size_t>(alignment));
}

void* operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    return nothrow_probe::record(Entry::nothrow_scalar, size, 0, &tag);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t& tag) noexcept {
    return nothrow_probe::record(Entry::nothrow_scalar, size, static_cast<std::size_t>(alignment), &tag);
}

int main() {
    return nothrow_probe::verify(Entry::nothrow_scalar, Entry::ordinary_array);
}
