// [exec.task.scheduler]/3-5, /10 and /13 exercise allocator-aware construction,
// copying and the schedule sender's backend. The downstream probe exercises
// libycxx's allocator guarantee for backend operation storage (#24).
// FLAGS: -DTASK_SCHEDULER_PROBE_GLOBAL_NEW -DTASK_SCHEDULER_REQUIRE_DOWNSTREAM_ROUTING
// REQUIRES: exceptions

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <execution>
#include <limits>
#include <memory_resource>
#include <new>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", message);
        std::abort();
    }
}

thread_local std::size_t* active_new_count = nullptr;

struct count_new {
    std::size_t* previous;

    explicit count_new(std::size_t* count) noexcept
        : previous(std::exchange(active_new_count, count)) {}

    ~count_new() { active_new_count = previous; }
};

struct tracking_resource : std::pmr::memory_resource {
    std::size_t allocations = 0;
    std::size_t deallocations = 0;

    std::size_t live() const noexcept { return allocations - deallocations; }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        count_new pause{nullptr};
        void* storage = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++allocations;
        return storage;
    }

    void do_deallocate(void* storage, std::size_t bytes, std::size_t alignment) override {
        require(live() != 0, "deallocation through a resource with no live allocation");
        count_new pause{nullptr};
        std::pmr::new_delete_resource()->deallocate(storage, bytes, alignment);
        ++deallocations;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct work {
    std::pmr::memory_resource* resource = nullptr;
    unsigned char expected_marker = 0;
    unsigned starts = 0;
    unsigned char marker = 0;
};

template<std::size_t Bytes>
struct scheduler_storage {
    work* state;
    std::array<unsigned char, Bytes> data{};

    explicit scheduler_storage(work& state) noexcept : state(&state) {
        data.back() = state.expected_marker;
    }

    unsigned char marker() const noexcept { return data.back(); }
};

template<>
struct scheduler_storage<0> {
    work* state;

    explicit scheduler_storage(work& state) noexcept : state(&state) {}

    unsigned char marker() const noexcept { return state->expected_marker; }
};

template<std::size_t SchedulerBytes, std::size_t OperationBytes>
struct scheduler {
    using scheduler_concept = ex::scheduler_tag;

    scheduler_storage<SchedulerBytes> storage;

    explicit scheduler(work& state) noexcept : storage(state) {}

    bool operator==(const scheduler& other) const noexcept {
        return storage.state == other.storage.state && storage.marker() == other.storage.marker();
    }

    auto query(ex::get_forward_progress_guarantee_t) const noexcept {
        return ex::forward_progress_guarantee::weakly_parallel;
    }

    scheduler query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept {
        return *this;
    }

    struct sender {
        using sender_concept = ex::sender_tag;

        work* state;
        unsigned char marker;

        struct attributes {
            work* state;

            scheduler query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept {
                return scheduler{*state};
            }
        };

        attributes get_env() const noexcept { return {state}; }

        template<class, class...>
        static consteval auto get_completion_signatures() {
            return ex::completion_signatures<ex::set_value_t()>{};
        }

        template<class Receiver>
        struct operation {
            using operation_state_concept = ex::operation_state_tag;

            Receiver receiver;
            work* state;
            std::array<unsigned char, OperationBytes> data{};
            void* user_storage = nullptr;

            template<class R>
            operation(work* state, unsigned char marker, R&& receiver)
                : receiver(std::forward<R>(receiver)), state(state) {
                data.back() = marker;
                if (state->resource)
                    user_storage = state->resource->allocate(1);
            }

            operation(operation&&) = delete;

            ~operation() {
                if (user_storage)
                    state->resource->deallocate(user_storage, 1);
            }

            void start() & noexcept {
                ++state->starts;
                state->marker = data.back();
                ex::set_value(std::move(receiver));
            }
        };

        template<ex::receiver R>
        auto connect(R&& receiver) const {
            return operation<std::decay_t<R>>{state, marker, std::forward<R>(receiver)};
        }
    };

    sender schedule() const noexcept { return {storage.state, storage.marker()}; }
};

struct receiver {
    using receiver_concept = ex::receiver_tag;

    unsigned* completions;

    ex::env<> get_env() const noexcept { return {}; }
    void set_value() && noexcept { ++*completions; }
    void set_error(std::exception_ptr) && noexcept { require(false, "unexpected error completion"); }
    void set_stopped() && noexcept { require(false, "unexpected stopped completion"); }
};

using allocator = std::pmr::polymorphic_allocator<std::byte>;

template<class Scheduler>
void run(const char* label, bool allocate_in_user_scheduler = false) {
    tracking_resource original, replaced, user;
    work source{.resource = allocate_in_user_scheduler ? &user : nullptr, .expected_marker = 37};
    work other{.expected_marker = 19};
    unsigned completions = 0;
    std::size_t direct_new = 0;
    std::size_t downstream_new = 0;
    std::size_t construction_allocations = 0;
    std::size_t schedule_allocations = 0;
    std::size_t operation_allocations = 0;
    std::size_t operation_allocations_from_replaced = 0;
    {
        count_new direct{&direct_new};
        ex::task_scheduler surviving{Scheduler{other}, allocator{&replaced}};
        {
            ex::task_scheduler first{Scheduler{source}, allocator{&original}};
            auto previous_allocations = original.allocations;
            auto previous_live = original.live();
            auto replaced_allocations = replaced.allocations;
            ex::task_scheduler copy{first};
            surviving = copy;
            require(copy == first && surviving == first, "copies compare equal");
            require(original.allocations == previous_allocations && original.live() == previous_live,
                    "copy or assignment changed the copied backend allocation");
            require(replaced.allocations == replaced_allocations && replaced.live() == 0,
                    "assignment releases the previous backend through its allocator");
        }
        construction_allocations = original.allocations;
        auto replaced_allocations = replaced.allocations;
        auto sender = ex::schedule(surviving);
        require(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender)) == surviving,
                "schedule sender reports the surviving scheduler");
        require(replaced.allocations == replaced_allocations,
                "schedule on an assigned scheduler uses the current allocator");
        auto after_schedule = original.allocations;
        schedule_allocations = after_schedule - construction_allocations;
        {
            count_new downstream{&downstream_new};
            auto operation = ex::connect(std::move(sender), receiver{&completions});
            require(completions == 0, "no completion before start");
            ex::start(operation);
            require(completions == 1 && source.starts == 1 && source.marker == source.expected_marker,
                    "the surviving scheduler starts and completes the original operation");
        }
        operation_allocations = original.allocations - after_schedule;
        operation_allocations_from_replaced = replaced.allocations - replaced_allocations;
    }
    require(original.live() == 0 && replaced.live() == 0 && user.live() == 0,
            "all allocation resources balance after destruction");
    require(user.allocations == (allocate_in_user_scheduler ? 1u : 0u),
            "the user scheduler's own allocation stays with its resource");
    std::printf("%s: construction=%zu schedule=%zu connect/start=%zu replaced=%zu user=%zu\n",
                label, construction_allocations, schedule_allocations, operation_allocations,
                operation_allocations_from_replaced, user.allocations);
#if defined(TASK_SCHEDULER_PROBE_GLOBAL_NEW)
    require(direct_new == 0, "a direct scheduler call used global new outside the supplied resource");
    std::printf("%s: direct untracked new=%zu connect/start untracked new=%zu\n",
                label, direct_new, downstream_new);
#endif
#if defined(TASK_SCHEDULER_REQUIRE_DOWNSTREAM_ROUTING)
#if !defined(TASK_SCHEDULER_PROBE_GLOBAL_NEW)
#error TASK_SCHEDULER_REQUIRE_DOWNSTREAM_ROUTING requires TASK_SCHEDULER_PROBE_GLOBAL_NEW
#endif
    require(downstream_new == 0, "erased operation storage used global new outside the supplied resource");
    require(operation_allocations_from_replaced == 0,
            "erased operation storage used the allocator from before assignment");
#endif
}

} // namespace

#if defined(TASK_SCHEDULER_PROBE_GLOBAL_NEW)

namespace {
void note_new() noexcept {
    if (active_new_count)
        ++*active_new_count;
}
} // namespace

void* operator new(std::size_t bytes) {
    note_new();
    if (void* storage = std::malloc(bytes == 0 ? 1 : bytes))
        return storage;
    throw std::bad_alloc{};
}

void* operator new(std::size_t bytes, std::align_val_t alignment) {
    note_new();
    const auto align = static_cast<std::size_t>(alignment);
    if (bytes == 0)
        bytes = 1;
    if (bytes <= std::numeric_limits<std::size_t>::max() - (align - 1)) {
        const auto rounded = (bytes + align - 1) / align * align;
        if (void* storage = std::aligned_alloc(align, rounded))
            return storage;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void* operator new[](std::size_t bytes, std::align_val_t align) {
    return ::operator new(bytes, align);
}
void operator delete(void* storage) noexcept { std::free(storage); }
void operator delete(void* storage, std::size_t) noexcept { std::free(storage); }
void operator delete(void* storage, std::align_val_t) noexcept { std::free(storage); }
void operator delete(void* storage, std::size_t, std::align_val_t) noexcept { std::free(storage); }
void operator delete[](void* storage) noexcept { std::free(storage); }
void operator delete[](void* storage, std::size_t) noexcept { std::free(storage); }
void operator delete[](void* storage, std::align_val_t) noexcept { std::free(storage); }
void operator delete[](void* storage, std::size_t, std::align_val_t) noexcept { std::free(storage); }

#endif

int main() {
    constexpr std::size_t large = 64 * 1024;
    static_assert(ex::scheduler<scheduler<0, 1>>);
    static_assert(std::is_same_v<
        ex::completion_signatures_of_t<ex::schedule_result_t<scheduler<0, 1>>, ex::env<>>,
        ex::completion_signatures<ex::set_value_t()>>);
    run<scheduler<0, 1>>("small");
    run<scheduler<large, 1>>("large scheduler");
    run<scheduler<0, large>>("large operation");
    run<scheduler<large, large>>("large scheduler and operation");
    run<scheduler<0, large>>("large operation with user allocation", true);
    std::puts("PASS task_scheduler allocator");
}
