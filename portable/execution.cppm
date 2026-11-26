// Keep the provider's linkage consistent with the same declarations included by its headers.
module;
#include <ycxx/execution/detail/standard_headers.hpp>

export module ycxx.execution.portable;

export extern "C++" {
#include <ycxx/execution/execution.hpp>
#include <ycxx/execution/stack_thread.hpp>
}
