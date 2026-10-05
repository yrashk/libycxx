// Library functions that cannot report an allocation failure, run while operator new fails at
// its k-th call, for every k until the call makes no allocation that fails. A noexcept function
// whose allocation failure escapes calls std::terminate ([except.spec]/5), which ends this test
// with a crash; a "Throws: Nothing" function must not throw ([structure.specifications]/3.4).
// Either the function needs no allocation, or it recovers from the failure and still meets its
// specification; nothing it allocates is leaked.
//   [fs.path.compare] path::compare noexcept; [fs.path.nonmember] operator==, operator<=> and
//     hash_value noexcept ("hash_value(p): if p1 == p2 then hash_value(p1) == hash_value(p2)").
//   [syserr.syserr.members] system_error::what() noexcept (a string that incorporates the
//     what_arg and code().message(): [syserr.syserr.members]/? "returns an NTBS that
//     incorporates the arguments supplied in the constructor"); [fs.filesystem.error.members]
//     filesystem_error::what() noexcept; [ios.failure] (derived from system_error).
//   [propagation]/12 make_exception_ptr noexcept: "Creates an exception_ptr object that refers
//     to a copy of e" (or, per [propagation]/9 when copying fails, to the bad_alloc or a
//     bad_exception); current_exception noexcept; [except.nested]: nested_exception()
//     noexcept captures current_exception().
//   [stopcallback.cons]: stop_callback(const stop_token&, C&&) is
//     noexcept(is_nothrow_constructible_v<CallbackFn, Initializer>); [stopsource.mem]
//     request_stop noexcept calls the registered callbacks.
//   [func.wrap.func.con]/? function(function&&) noexcept; [func.wrap.move.ctor]
//     move_only_function(move_only_function&&) noexcept; [any.cons] any(any&&) noexcept;
//     [locale.cons] locale(const locale&) noexcept.
//   [mem.res.private] do_deallocate "Throws: Nothing" for every resource of [mem.res.global],
//     [mem.res.pool.mem] and [mem.res.monotonic.buffer.mem].
#include <any>
#include <exception>
#include <filesystem>
#include <functional>
#include <ios>
#include <locale>
#include <memory_resource>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <system_error>
#include "exc_new.hpp"

using namespace exh;
namespace fs = std::filesystem;

template <class F>
void nothrow_sweep(const char* name, F op) {
  sweep_new(name, [&] {
    bool threw = attempt(op);
    EXH_EXPECT(!threw, "an allocation failure escaped from a function that cannot throw");
    return st.fired;
  }, options{true, 4000});
}

struct BigCallable {
  char data[256] = {};
  std::string s = std::string(100, 'x');
  int operator()() const { return int(s.size()); }
};

int main() {
  const fs::path p1("/a/long/path/with/many/components/that/do/not/fit/in/a/small/buffer/file.txt");
  const fs::path p2("/a/long/path/with/many/components/that/do/not/fit/in/a/small/buffer/./file.txt");
  const fs::path p3("/a/long//path/with/many/components/that/do/not/fit/in/a/small/buffer/file.txt");
  const fs::path rel("relative/path/with/several/components/which/are/long/enough/x.y");
  const std::size_t h1 = fs::hash_value(p1);
  nothrow_sweep("path compare", [&] {
    EXH_EXPECT(p1.compare(p1) == 0 && p1.compare(p2) != 0 && (p1.compare(p2) < 0) == (p1 < p2) && p1.compare(p3) == 0,
               "path::compare");
    EXH_EXPECT(p1 == p3 && !(p1 == p2) && (p1 <=> p3) == 0 && (rel <=> p1) != 0, "path ==, <=>");
  });
  nothrow_sweep("path hash_value", [&] {
    EXH_EXPECT(fs::hash_value(p1) == h1 && fs::hash_value(p3) == h1, "hash_value(p1) == hash_value(p3) for p1 == p3");
  });

  const std::system_error se(std::make_error_code(std::errc::no_such_file_or_directory), "a what_arg string long enough");
  const std::string se_what = se.what();
  const fs::filesystem_error fe("a filesystem what_arg long enough to be allocated", p1, rel,
                                std::make_error_code(std::errc::permission_denied));
  const std::string fe_what = fe.what();
  const std::ios_base::failure iof("an ios_base::failure message long enough");
  const std::string iof_what = iof.what();
  nothrow_sweep("what", [&] {
    EXH_EXPECT(se.what() == se_what, "system_error::what");
    EXH_EXPECT(fe.what() == fe_what, "filesystem_error::what");
    EXH_EXPECT(iof.what() == iof_what, "ios_base::failure::what");
  });
  // what() on objects never asked before (a lazily built message must not throw either).
  nothrow_sweep("what, first call", [&] {
    disarm();
    std::system_error e(std::make_error_code(std::errc::invalid_argument), std::string(80, 'w'));
    fs::filesystem_error f(std::string(80, 'f'), p1, std::make_error_code(std::errc::io_error));
    if (st.kind >= 0) st.left[st.kind] = st.k;
    const char* w1 = e.what();
    const char* w2 = f.what();
    EXH_EXPECT(w1 != nullptr && w2 != nullptr, "what() returned null");
  });

  const std::runtime_error re(std::string(100, 'r'));
  nothrow_sweep("make_exception_ptr", [&] {
    std::exception_ptr p = std::make_exception_ptr(re);  // copying a runtime_error does not throw
    EXH_EXPECT(p != nullptr, "make_exception_ptr returned null");
    disarm();
    try {
      std::rethrow_exception(p);
    } catch (const std::runtime_error& e) {
      EXH_EXPECT(std::string(e.what()) == std::string(100, 'r'), "make_exception_ptr: wrong object");
    } catch (const std::bad_alloc&) {
    } catch (const std::bad_exception&) {
    } catch (...) {
      EXH_EXPECT(false, "make_exception_ptr: unexpected exception type");
    }
  });
  nothrow_sweep("current_exception, nested_exception", [&] {
    disarm();
    try {
      throw std::logic_error("current");
    } catch (...) {
      if (st.kind >= 0) st.left[st.kind] = st.k;
      std::exception_ptr p = std::current_exception();
      std::nested_exception n;
      EXH_EXPECT(p != nullptr && n.nested_ptr() != nullptr, "current_exception");
    }
  });

  nothrow_sweep("stop_callback, request_stop", [&] {
    disarm();
    std::stop_source src;
    int called = 0;
    auto cb = [&called]() noexcept { ++called; };
    if (st.kind >= 0) st.left[st.kind] = st.k;
    {
      std::stop_callback<decltype(cb)> c1(src.get_token(), cb);
      std::stop_callback<decltype(cb)> c2(src.get_token(), cb);
      EXH_EXPECT(src.request_stop(), "request_stop");
      std::stop_callback<decltype(cb)> c3(src.get_token(), cb);  // runs at once
    }
    EXH_EXPECT(called == 3, "stop callbacks");
  });

  nothrow_sweep("noexcept moves and copies", [&] {
    disarm();
    std::function<int()> f = BigCallable{};
    std::move_only_function<int()> m = BigCallable{};
    std::copyable_function<int()> c = BigCallable{};
    std::any a = BigCallable{};
    std::locale l = std::locale::classic();
    if (st.kind >= 0) st.left[st.kind] = st.k;
    std::function<int()> f2(std::move(f));
    std::move_only_function<int()> m2(std::move(m));
    std::copyable_function<int()> c2(std::move(c));
    std::any a2(std::move(a));
    std::locale l2(l);
    l2 = l;
    f2.swap(f);
    m2.swap(m);
    a2.swap(a);
    EXH_EXPECT(f() == 100 && m() == 100 && c2() == 100 && a.has_value() && l2 == l, "moved values");
  });

  nothrow_sweep("memory resource deallocate", [&] {
    disarm();
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::synchronized_pool_resource spool;
    std::pmr::monotonic_buffer_resource mono;
    void* blocks[6][3];
    for (int i = 0; i < 6; ++i) {
      blocks[i][0] = pool.allocate(std::size_t(8) << i);
      blocks[i][1] = spool.allocate(std::size_t(8) << i);
      blocks[i][2] = mono.allocate(std::size_t(8) << i);
    }
    void* big = pool.allocate(1 << 20);
    void* nd = std::pmr::new_delete_resource()->allocate(100, 64);
    if (st.kind >= 0) st.left[st.kind] = st.k;
    for (int i = 0; i < 6; ++i) {
      pool.deallocate(blocks[i][0], std::size_t(8) << i);
      spool.deallocate(blocks[i][1], std::size_t(8) << i);
      mono.deallocate(blocks[i][2], std::size_t(8) << i);
    }
    pool.deallocate(big, 1 << 20);
    std::pmr::new_delete_resource()->deallocate(nd, 100, 64);
    EXH_EXPECT(pool.is_equal(pool) && !pool.is_equal(spool), "is_equal");
  });
  return finish();
}
