// The library's allocation functions used from many threads at once, while other threads set
// and read the new_handler and the terminate_handler.
//   [new.delete.dataraces]/1: the library versions of operator new and operator delete "shall
//     not introduce a data race"; "Calls to these functions that allocate or deallocate a
//     particular unit of storage shall occur in a single total order, and each such deallocation
//     call shall happen before the next allocation (if any) in this order." So two live blocks
//     never overlap, and a block filled by one thread, freed, and handed to another thread holds
//     nothing that races (checked by filling and verifying every block, and under TSan).
//   [new.delete.single]/2, [new.delete.array]: suitably aligned storage, for the align_val_t
//     forms the requested alignment; the nothrow forms return such storage too.
//   [handler.functions]/4: "Calling the set_* and get_* functions shall not incur a data race. A
//     call to any of the set_* functions synchronizes with subsequent calls to the same set_*
//     function and to the corresponding get_* function": the handler a get_* call returns is
//     one that some set_* call installed (or the initial one).
// FLAGS: -pthread
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <new>
#include <thread>
#include <vector>
#include "check.hpp"

static void nh1() {}
static void nh2() {}
[[noreturn]] static void th1() { std::abort(); }
[[noreturn]] static void th2() { std::abort(); }

struct Block {
  unsigned char* p;
  std::size_t n;
  std::size_t align;  // 0: default
  int form;
  unsigned char tag;
};

static void release(const Block& b) {
  switch (b.form) {
    case 0: ::operator delete(b.p, b.n); break;
    case 1: ::operator delete[](b.p); break;
    case 2: ::operator delete(b.p, std::align_val_t(b.align)); break;
    case 3: ::operator delete[](b.p, b.n, std::align_val_t(b.align)); break;
    default: ::operator delete(b.p, std::nothrow); break;
  }
}

int main() {
  constexpr int Workers = 6, Rounds = 20000;
  std::atomic<bool> stop{false};
  std::atomic<long> bad{0};
  std::vector<std::thread> ts;
  for (int t = 0; t < Workers; ++t)
    ts.emplace_back([&, t] {
      std::vector<Block> live;
      std::uint32_t seed = 12345u + static_cast<std::uint32_t>(t);
      auto rnd = [&] { return seed = seed * 1664525u + 1013904223u; };
      for (int r = 0; r < Rounds; ++r) {
        if (live.size() < 64 && (rnd() >> 8) % 3 != 0) {
          Block b{};
          b.form = static_cast<int>((rnd() >> 8) % 5);
          b.n = 1 + (rnd() >> 8) % (r % 50 == 0 ? 100000 : 300);
          b.align = b.form == 2 || b.form == 3 ? std::size_t(1) << (4 + (rnd() >> 8) % 9) : 0;  // 16 .. 4096
          b.tag = static_cast<unsigned char>(rnd() >> 24);
          switch (b.form) {
            case 0: b.p = static_cast<unsigned char*>(::operator new(b.n)); break;
            case 1: b.p = static_cast<unsigned char*>(::operator new[](b.n)); break;
            case 2: b.p = static_cast<unsigned char*>(::operator new(b.n, std::align_val_t(b.align))); break;
            case 3: b.p = static_cast<unsigned char*>(::operator new[](b.n, std::align_val_t(b.align))); break;
            default: b.p = static_cast<unsigned char*>(::operator new(b.n, std::nothrow)); break;
          }
          if (!b.p) {
            bad.fetch_add(1);
            continue;
          }
          const std::size_t need = b.align ? b.align : alignof(std::max_align_t);
          if (reinterpret_cast<std::uintptr_t>(b.p) % need != 0) bad.fetch_add(1);
          for (std::size_t i = 0; i < b.n; ++i) b.p[i] = static_cast<unsigned char>(b.tag + i);
          live.push_back(b);
        } else if (!live.empty()) {
          const std::size_t k = (rnd() >> 8) % live.size();
          const Block b = live[k];
          for (std::size_t i = 0; i < b.n; ++i)
            if (b.p[i] != static_cast<unsigned char>(b.tag + i)) {
              bad.fetch_add(1);
              break;
            }
          release(b);
          live[k] = live.back();
          live.pop_back();
        }
      }
      for (const Block& b : live) release(b);
    });

  // Handler setters and getters.
  std::atomic<long> handler_bad{0};
  std::thread setter([&] {
    for (int i = 0; !stop.load(); ++i) {
      std::set_new_handler(i % 2 ? nh2 : nh1);
      std::set_terminate(i % 2 ? th2 : th1);
    }
  });
  std::vector<std::thread> getters;
  for (int g = 0; g < 2; ++g)
    getters.emplace_back([&] {
      while (!stop.load()) {
        const std::new_handler h = std::get_new_handler();
        if (h != nullptr && h != nh1 && h != nh2) handler_bad.fetch_add(1);
        (void)std::get_terminate();  // th1, th2 or the initial handler
      }
    });
  for (auto& th : ts) th.join();
  stop.store(true);
  setter.join();
  for (auto& g : getters) g.join();
  CHECK(bad.load() == 0);
  CHECK(handler_bad.load() == 0);
  std::set_new_handler(nullptr);
  return 0;
}
