// [thread.thread.constr]/3: template<class... Args> explicit thread(Args&&...): "Constraints:
// Args is not an empty pack, and remove_cvref_t<Args...[0]> is not the same type as thread."
// [thread.jthread.cons]/3 likewise for jthread. The class is not copyable.
#include <thread>
#include <type_traits>

static_assert(!std::is_constructible_v<std::thread, std::thread&>);
static_assert(!std::is_constructible_v<std::thread, const std::thread&>);
static_assert(!std::is_constructible_v<std::jthread, std::jthread&>);
static_assert(!std::is_constructible_v<std::jthread, const std::jthread&>);
static_assert(std::is_constructible_v<std::thread, void (*)()>);
static_assert(std::is_constructible_v<std::thread, void (*)(int), int>);
static_assert(std::is_constructible_v<std::jthread, void (*)(std::stop_token)>);
static_assert(std::is_constructible_v<std::jthread, void (*)(std::stop_token, int), long>);
static_assert(std::is_same_v<std::jthread::native_handle_type, std::thread::native_handle_type>);
