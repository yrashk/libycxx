// REQUIRES: !exceptions
// UNSUPPORTED-SANITIZER: asan ASan replaces allocation functions and their forwarding behavior.
// [new.delete.array] forwards the default ordinary array entries to the scalar entries.
#include "nothrow-replacements/forwarding.hpp"

using nothrow_probe::Entry;

void* operator new(std::size_t size) {
    return nothrow_probe::record_ordinary(Entry::ordinary_scalar, size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return nothrow_probe::record_ordinary(Entry::ordinary_scalar, size, static_cast<std::size_t>(alignment));
}

int main() {
    return nothrow_probe::verify(Entry::ordinary_scalar, Entry::ordinary_scalar);
}
