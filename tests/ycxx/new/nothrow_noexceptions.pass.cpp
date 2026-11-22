// REQUIRES: !exceptions
// UNSUPPORTED-SANITIZER: asan ASan replaces the allocation functions and their handler behavior.
// [new.delete.single], [new.delete.array], [new.handler]
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

namespace {
constexpr std::size_t extended_alignment = 64;
constexpr auto aligned = std::align_val_t{extended_alignment};
constexpr auto default_alignment = alignof(std::max_align_t);

struct Form {
    void* (*allocate)(std::size_t) noexcept;
    void (*release)(void*) noexcept;
    std::size_t alignment;
};

constexpr Form forms[] = {
    {[](std::size_t n) noexcept { return ::operator new(n, std::nothrow); },
     [](void* p) noexcept { ::operator delete(p); }, default_alignment},
    {[](std::size_t n) noexcept { return ::operator new[](n, std::nothrow); },
     [](void* p) noexcept { ::operator delete[](p); }, default_alignment},
    {[](std::size_t n) noexcept { return ::operator new(n, aligned, std::nothrow); },
     [](void* p) noexcept { ::operator delete(p, aligned); }, extended_alignment},
    {[](std::size_t n) noexcept { return ::operator new[](n, aligned, std::nothrow); },
     [](void* p) noexcept { ::operator delete[](p, aligned); }, extended_alignment},
};

// Exceed the hosted address space without overflowing alignment rounding.
constexpr auto too_large = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) + 1;
static_assert(too_large % extended_alignment == 0);
volatile std::size_t huge_request = too_large;
void* reserve;
int handler_calls;

void release_reserve() noexcept {
    ++handler_calls;
    ::operator delete(reserve);
    reserve = nullptr;
    std::set_new_handler(nullptr);
}

bool aligned_pointer(void* p, std::size_t alignment) noexcept {
    return p != nullptr && reinterpret_cast<std::uintptr_t>(p) % alignment == 0;
}

int check(const Form& form) {
    void* ordinary = form.allocate(65);
    if (!aligned_pointer(ordinary, form.alignment))
        return 1;
    form.release(ordinary);

    void* zero = form.allocate(0);
    void* another_zero = form.allocate(0);
    if (!aligned_pointer(zero, form.alignment) || !aligned_pointer(another_zero, form.alignment) || zero == another_zero)
        return 2;
    form.release(zero);
    form.release(another_zero);

    auto size = huge_request;
    if (void* unexpected = form.allocate(size)) {
        form.release(unexpected);
        return 3;
    }

    reserve = ::operator new(64, std::nothrow);
    if (!reserve)
        return 4;
    handler_calls = 0;
    std::set_new_handler(release_reserve);
    void* failed = form.allocate(size);
    bool handled = !failed && handler_calls == 1 && !reserve && std::get_new_handler() == nullptr;
    std::set_new_handler(nullptr);
    if (reserve) {
        ::operator delete(reserve);
        reserve = nullptr;
    }
    if (failed)
        form.release(failed);
    return handled ? 0 : 5;
}
}

int main() {
    const auto previous = std::set_new_handler(nullptr);
    int status = 0;
    int index = 1;
    for (const auto& form : forms) {
        if (const int error = check(form)) {
            status = 10 * index + error;
            break;
        }
        ++index;
    }
    std::set_new_handler(previous);
    return status;
}
