# Callable storage and allocation guarantees

libycxx's owning standard function wrappers use a small inline buffer to avoid allocating
the closure itself. `<ycxx/functional.hpp>` adds two **libycxx extensions**: a storage-policy
query and a configurable fixed-capacity wrapper. Both are in namespace `ycxx`, require an
explicit header include, and are **not exported by `import std;`**.

The [generated API reference](https://libycxx.org/api/headers/ycxx/functional.hpp.html) lists their
declarations, constraints and member documentation.

## Standard-wrapper SBO

`std::function`, `std::move_only_function` and `std::copyable_function` share this policy:

- The decayed target size is at most `4 * sizeof(void*)`.
- Its alignment is at most `alignof(std::max_align_t)`.
- It is nothrow move constructible. Trivial copying is not required.

On typical 64-bit targets the buffer has 32 bytes and 16-byte alignment, and the wrapper
occupies 48 bytes including metadata and padding. These are libycxx implementation choices,
not portable standard-library guarantees. Rebuild all C++ dependencies when the ABI changes.

| Capture contents | Typical closure size | Target storage |
| --- | ---: | --- |
| Four ordinary pointers | 32 bytes | Inline |
| Two `std::shared_ptr`s | 32 bytes | Inline |
| One libycxx `std::string` | 32 bytes | Inline |
| One string and an ordinary pointer | 40 bytes | Heap |
| A 16-byte target with 16-byte alignment | 16 bytes | Inline |
| A target requiring 32-byte alignment | At least 32 bytes | Heap |

These assume an otherwise valid, nothrow-movable target. Padding and target-specific type sizes
matter; capture count alone does not determine whether a lambda fits.

**Inline closure storage does not make all target operations allocation-free.** A copied string
can allocate its characters; `make_shared` can allocate an object and control block; a callable
can allocate when invoked. SBO avoids the wrapper's separate allocation for the closure.

## Querying inline storage

```cpp
#include <ycxx/functional.hpp>
#include <memory>

int main() {
    auto p = std::make_shared<int>(1);
    auto q = std::make_shared<int>(2);
    auto task = [p, q] { return *p + *q; };
    using callback = std::function<int()>;
    static_assert(ycxx::uses_inline_storage_v<callback, decltype(task)>);
    callback f = task; // no separate closure allocation
}
```

`ycxx::uses_inline_storage<Wrapper, Target>` derives from `std::bool_constant`; its `_v`
variable template exposes the value. Supported wrappers are the three standard owning wrappers
above and `ycxx::inplace_function`. Use an unqualified wrapper type. The target is decayed;
unknown wrappers, `void`, and incomplete target types report false.

This checks **storage properties only**, not signature compatibility, copyability or
construction from a particular argument. Use `std::is_constructible_v<Wrapper, Argument>`
as a separate check where appropriate. The trait cannot inspect an already erased wrapper's
runtime target, and does not describe conversions that adopt another standard wrapper's target.
Policy values can change with future ABI revisions. Assert against your actual callable types
instead of hard-coding a universal threshold.

## Selecting a fixed capacity

```cpp
#include <ycxx/functional.hpp>
#include <array>
#include <utility>

int main() {
    std::array<int, 12> values{};
    auto task = [values] { return values[0]; };
    using callback = ycxx::inplace_function<int(), 64>;
    static_assert(ycxx::uses_inline_storage_v<callback, decltype(task)>);
    callback f = task;
    auto copy = f;             // independent target copy
    auto moved = std::move(f); // f becomes empty
    copy.swap(moved);          // no wrapper allocation
}
```

The template is:

```cpp
template<class Signature,
         std::size_t Capacity = 4 * sizeof(void*),
         std::size_t Alignment = alignof(std::max_align_t)>
class inplace_function;
```

`Signature` must be `R(Args...)`. `Capacity` is a positive maximum target size in bytes;
`Alignment` must be a positive power of two supported by the compiler. Constants `capacity`
and `alignment` report the settings. Capacity excludes metadata and padding, so it is not
`sizeof(callback)`.

A target must fit the size and alignment, be copy constructible and nothrow move constructible,
be constructible from the argument, and be callable as a mutable lvalue with the specified
arguments and result. Conversions producing dangling reference results are rejected.

Unsuitable targets fail at compile time: **there is no heap fallback**. For example,
`inplace_function<void(), 16>` rejects a 32-byte closure. Move-only targets are also rejected;
use `std::move_only_function` when move-only ownership is needed.

For more alignment, specify it explicitly:

```cpp
#include <ycxx/functional.hpp>

struct alignas(64) Work {
    int value = 3;
    int operator()() const { return value; }
};
int main() {
    ycxx::inplace_function<int(), sizeof(Work), alignof(Work)> f = Work{};
}
```

## Operations and errors

| Operation | Behavior |
| --- | --- |
| Default construction or construction from `nullptr` | Empty; nonthrowing |
| Construction from a null function or member pointer | Empty |
| Construction from a callable | Owns a target; propagates construction exceptions |
| Copy construction | Independent target copy; can throw if copying the target throws |
| Copy or callable assignment | Builds the replacement first; destination unchanged on construction failure |
| Move construction or assignment | Nonthrowing; source becomes empty; self-move does nothing |
| `swap` | Nonthrowing; self-swap does nothing |
| `operator bool`, comparison with `nullptr` | Queries whether a target exists |
| Calling an empty wrapper | Throws `std::bad_function_call` |
| Calling a nonempty wrapper | Invokes the target and propagates its exceptions |

When exceptions are disabled, an empty call follows libycxx's `ycxx_error_handler` policy for
`bad_function_call`. Callable construction and assignment are conditionally nonthrowing when
construction of the target from the argument is. No operation allocates wrapper target storage;
the target's own operations can still allocate internally.

A const wrapper can invoke a mutable target, as with `std::function`; this supplies no
synchronization for concurrent calls. References and reference captures retain their lifetime
requirements: their referents must outlive their use.

This extension has no `target()`/`target_type()` introspection, allocator argument, or
cv/ref/noexcept-qualified signatures. Different capacity specializations are different types.
Passing one to another treats the entire source wrapper as a callable, which must itself fit;
it does not transfer an erased target into a smaller buffer.

## Choosing a wrapper

Use `std::function` for a copyable callback with automatic heap fallback. Use
`inplace_function` when exceeding a known buffer must fail at compile time. Use
`std::function_ref` for borrowing when the callable's lifetime is guaranteed, or a template
parameter when type erasure is unnecessary.

Larger buffers enlarge every wrapper, including empty ones. Measure callback arrays,
construction, copying and movement as well as invocation. `bench/function_storage.cpp` covers
three-/four-/six-pointer boundaries, alignment, smart pointers, strings and collections:

```sh
bench/run --cc clang --only function_storage --libs ycxx --runs 3 --md
```

The [recorded SBO comparison](../bench/RESULTS.md#function-wrapper-sbo-2026-10-15) includes both
newly admitted targets and already-small callback collections, along with the method and limits.
