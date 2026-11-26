// Include these headers in a global module fragment before exporting the portable declarations.
#pragma once

#include <ycxx/execution/detail/host_features.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <execution>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <version>

#if YCXX_PORTABLE_HAS_HOST_NOSTOPSTATE
#include <stop_token>
#endif
