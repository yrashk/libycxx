// [atomics.types.int]/1 provides specializations for the (cv-unqualified) integer types only;
// atomic<const int> uses the primary template, and [atomics.types.generic.general]/1: "The
// program is ill-formed if any of ... is_copy_assignable_v<T>, ... is_same_v<T, remove_cv_t<T>>,
// is false."
#include <atomic>

std::atomic<const int> a;
