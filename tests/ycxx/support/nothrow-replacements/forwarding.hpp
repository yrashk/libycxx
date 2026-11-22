#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>

namespace nothrow_probe {
enum class Entry { ordinary_scalar, ordinary_array, nothrow_scalar };

struct Request {
    Entry entry;
    std::size_t size;
    std::size_t alignment;
    const std::nothrow_t* tag;
    void* storage;
};

inline Request last{};
inline int calls;

inline void* record(Entry entry, std::size_t size, std::size_t alignment = 0,
                    const std::nothrow_t* tag = nullptr) noexcept {
    void* storage = nullptr;
    auto bytes = size == 0 ? 1 : size;
    if (alignment == 0) {
        storage = std::malloc(bytes);
    } else {
        const auto extra = (alignment - bytes % alignment) % alignment;
        if (extra <= std::numeric_limits<std::size_t>::max() - bytes)
            storage = std::aligned_alloc(alignment, bytes + extra);
    }
    last = {entry, size, alignment, tag, storage};
    ++calls;
    return storage;
}

inline void* record_ordinary(Entry entry, std::size_t size, std::size_t alignment = 0) {
    if (void* storage = record(entry, size, alignment))
        return storage;
    std::abort();
}

inline bool matches(void* storage, Entry entry, std::size_t size, std::size_t alignment = 0,
                    const std::nothrow_t* tag = nullptr) {
    const bool captured = storage != nullptr && storage == last.storage;
    const bool aligned = alignment == 0 || reinterpret_cast<std::uintptr_t>(storage) % alignment == 0;
    const bool correct = captured && aligned && calls == 1 && last.entry == entry && last.size == size &&
                         last.alignment == alignment && last.tag == tag;
    std::free(last.storage);
    calls = 0;
    last = {};
    return correct;
}

inline int verify(Entry scalar, Entry array) {
    calls = 0;
    last = {};
    const std::nothrow_t tag{};
    const auto* scalar_tag = scalar == Entry::nothrow_scalar ? &tag : nullptr;
    constexpr std::size_t alignment = 64;
    constexpr auto aligned = std::align_val_t{alignment};
    constexpr std::size_t requests[] = {0, 65};
    for (const auto size : requests) {
        if (!matches(::operator new(size, tag), scalar, size, 0, scalar_tag))
            return 1;
        if (!matches(::operator new[](size, tag), array, size))
            return 2;
        if (!matches(::operator new(size, aligned, tag), scalar, size, alignment, scalar_tag))
            return 3;
        if (!matches(::operator new[](size, aligned, tag), array, size, alignment))
            return 4;
    }
    return 0;
}
}
