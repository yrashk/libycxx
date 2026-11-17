// [stopsource.inplace.mem]/request_stop and [stopsource.mem]/request_stop: a callback may end the
// stop source itself (after deregistering every callback still registered, as a callback may
// destroy other callbacks, [stoptoken.concepts]/3.3.2): the completion of an operation state that
// owns the source, from inside a stop callback of one of its children, destroys the source
// (when_all, task, the parallel scheduler's receiver proxy). request_stop() must then return true
// without touching the destroyed state: not even to take its list lock again for the next
// callback, as with other registrations already run (every one is taken off the list before it
// runs) or destroyed by this one.
// The storage of the destroyed source is overwritten (destroyed_storage.hpp), so a request_stop
// that went on would spin on the lock it reads as taken: the watchdog fails the test.
#include <optional>
#include <stop_token>
#include "check.hpp"
#include "destroyed_storage.hpp"
#include "watchdog.hpp"

struct fn {
  void (*f)(void*);
  void* p;
  void operator()() noexcept { f(p); }
};

// inplace_stop_source: the one callback destroys the source.
void inplace_last_callback() {
  struct st {
    destroyed_storage<std::inplace_stop_source> src;
    std::optional<std::inplace_stop_callback<fn>> cb;
    int ran = 0;
  } s;
  s.src.emplace();
  s.cb.emplace(s.src->get_token(), fn{[](void* p) {
                 auto& s = *static_cast<st*>(p);
                 ++s.ran;
                 s.cb.reset();
                 s.src.destroy();
               },
                                        &s});
  CHECK(s.src->request_stop());
  CHECK(s.ran == 1 && !s.src.alive());
}

// inplace_stop_source: the callback that destroys the source is not the last registered one; it
// deregisters the one registered before it (still on the list) first.
void inplace_other_still_registered() {
  struct st {
    destroyed_storage<std::inplace_stop_source> src;
    std::optional<std::inplace_stop_callback<fn>> first, second;
    int ran_first = 0, ran_second = 0;
  } s;
  s.src.emplace();
  s.first.emplace(s.src->get_token(), fn{[](void* p) { ++static_cast<st*>(p)->ran_first; }, &s});
  // Registered later: run first.
  s.second.emplace(s.src->get_token(), fn{[](void* p) {
                     auto& s = *static_cast<st*>(p);
                     ++s.ran_second;
                     s.first.reset();
                     s.second.reset();
                     s.src.destroy();
                   },
                                            &s});
  CHECK(s.src->request_stop());
  CHECK(s.ran_second == 1 && s.ran_first == 0 && !s.src.alive());
}

// stop_source: the callback destroys the source and itself, the last owners of the shared state
// (the token it was registered through was a temporary), which is freed.
void shared_state_freed() {
  struct st {
    std::stop_source* src = nullptr;
    std::optional<std::stop_callback<fn>> cb;
    int ran = 0;
  } s;
  s.src = new std::stop_source;
  s.cb.emplace(s.src->get_token(), fn{[](void* p) {
                 auto& s = *static_cast<st*>(p);
                 ++s.ran;
                 s.cb.reset();
                 delete s.src;
                 s.src = nullptr;
               },
                                        &s});
  std::stop_source* src = s.src;
  CHECK(src->request_stop());
  CHECK(s.ran == 1 && s.src == nullptr);
}

int main() {
  watchdog(10);
  inplace_last_callback();
  inplace_other_still_registered();
  shared_state_freed();
  return 0;
}
