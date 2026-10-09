// Self-swap, self-copy-assignment and self-move-assignment of library types.
//   [utility.swap]/3: swap(a, b) "Exchanges the values of a and b": with a and b the same
//     object, its value is unchanged. The same holds for the member swap functions
//     ([container.reqmts] a.swap(b): "Exchanges the contents of a and b"; [optional.swap];
//     [variant.swap]; [any.modifiers]; [func.wrap.func.mod]; ...), for ranges::swap
//     ([concept.swappable]: "exchanges the values denoted by E1 and E2") and for the swap found
//     by argument-dependent lookup.
//   [cpp17.copyassignable]: after t = v, "the value of t is equivalent to v" (unchanged for
//     t = t).
//   [lib.types.movedfrom]/2: "An object of a type defined in the C++ standard library may be
//     move-assigned to itself. Unless otherwise specified, such an assignment places the object
//     in a valid but unspecified state": the object can then be assigned and used.
//   Otherwise specified (the value is kept):
//     [any.assign]/4, [func.wrap.move.ctor]/22, [func.wrap.copy.ctor]/26,
//     [util.smartptr.shared.assign]/4, [util.smartptr.weak.assign]/4, [futures.promise]/8,
//     [futures.task.members]/12, [thread.lock.unique.cons]/18, [thread.lock.shared.cons]/17:
//     "Equivalent to: X(std::move(rhs)).swap(*this)": the temporary takes the value and gives
//     it back; [unique.ptr.single.asgn]: "Calls reset(u.release())";
//     [futures.unique.future]/11, [futures.shared.future]/13, [thread.jthread.cons]/13,
//     [fs.path.assign]/3: "If addressof(rhs) == this is true, there are no effects";
//     [re.regex.assign]/2: flags() and mark_count() "return the values that e.flags() and
//     e.mark_count() ... had before assignment".
// Run also under SANITIZER=asan: a self-move that frees the storage it then reads shows there.
// FLAGS: -pthread
#include <any>
#include <array>
#include <bitset>
#include <complex>
#include <deque>
#include <expected>
#include <filesystem>
#include <flat_map>
#include <flat_set>
#include <forward_list>
#include <functional>
#include <future>
#include <inplace_vector>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include "check.hpp"

static std::string long_str(int i) { return "a string long enough to need dynamic storage #" + std::to_string(i); }

// Swaps and copy-assignment keep the value; a self-move leaves an object that can be assigned.
template <class T, class Make, class Eq>
void common(Make make, Eq eq) {
  auto check = [&](T& a, int i) {
    T w = make(i);
    CHECK(eq(a, w));
  };
  {
    T a = make(1);
    std::swap(a, a);
    check(a, 1);
  }
  {
    T a = make(1);
    using std::swap;
    swap(a, a);
    check(a, 1);
  }
  {
    T a = make(1);
    std::ranges::swap(a, a);
    check(a, 1);
  }
  if constexpr (requires(T& x) { x.swap(x); }) {
    T a = make(1);
    a.swap(a);
    check(a, 1);
  }
  if constexpr (std::is_copy_assignable_v<T>) {
    T a = make(1);
    const T& r = a;
    a = r;
    check(a, 1);
  }
  {
    T a = make(1);
    T& r = a;
    a = std::move(r);
    a = make(2);
    check(a, 2);
  }
  {
    T a = make(1);
    T& r = a;
    a = std::move(r);  // then only destroyed
  }
}

// Self-move-assignment keeps the value.
template <class T, class Make, class Eq>
void keeps(Make make, Eq eq) {
  auto check = [&](T& a, int i) {
    T w = make(i);
    CHECK(eq(a, w));
  };
  common<T>(make, eq);
  T a = make(1);
  T& r = a;
  a = std::move(r);
  check(a, 1);
}

template <class T>
void container(auto make) {
  common<T>(make, [](const T& a, const T& b) { return a == b; });
}

int main() {
  auto eq = [](const auto& a, const auto& b) { return a == b; };
  common<std::string>([](int i) { return long_str(i); }, eq);
  common<std::wstring>([](int i) { return std::wstring(40, wchar_t(L'a' + i)); }, eq);
  container<std::vector<std::string>>([](int i) { return std::vector<std::string>{long_str(i), long_str(i + 1)}; });
  container<std::vector<bool>>([](int i) { return std::vector<bool>(100 + i, true); });
  container<std::deque<std::string>>([](int i) { return std::deque<std::string>(40, long_str(i)); });
  container<std::list<std::string>>([](int i) { return std::list<std::string>{long_str(i)}; });
  container<std::forward_list<std::string>>([](int i) { return std::forward_list<std::string>{long_str(i)}; });
  container<std::map<int, std::string>>([](int i) { return std::map<int, std::string>{{i, long_str(i)}, {9, "x"}}; });
  container<std::multimap<int, std::string>>([](int i) { return std::multimap<int, std::string>{{i, long_str(i)}, {i, "y"}}; });
  container<std::set<std::string>>([](int i) { return std::set<std::string>{long_str(i), "b"}; });
  container<std::multiset<int>>([](int i) { return std::multiset<int>{i, i, 3}; });
  container<std::unordered_map<std::string, int>>([](int i) { return std::unordered_map<std::string, int>{{long_str(i), i}}; });
  container<std::unordered_set<int>>([](int i) {
    std::unordered_set<int> s;
    for (int k = 0; k < 50; ++k) s.insert(k * i);
    return s;
  });
  container<std::unordered_multimap<int, int>>([](int i) { return std::unordered_multimap<int, int>{{i, 1}, {i, 2}}; });
  container<std::flat_map<int, std::string>>([](int i) { return std::flat_map<int, std::string>{{i, long_str(i)}, {7, "z"}}; });
  container<std::flat_set<std::string>>([](int i) { return std::flat_set<std::string>{long_str(i)}; });
  container<std::flat_multimap<int, int>>([](int i) { return std::flat_multimap<int, int>{{i, i}, {i, 0}}; });
  container<std::inplace_vector<std::string, 4>>([](int i) { return std::inplace_vector<std::string, 4>{long_str(i), "q"}; });
  container<std::array<std::string, 2>>([](int i) { return std::array<std::string, 2>{long_str(i), "w"}; });
  container<std::pair<std::string, std::vector<int>>>([](int i) { return std::pair(long_str(i), std::vector<int>(30, i)); });
  container<std::tuple<std::string, int>>([](int i) { return std::tuple(long_str(i), i); });
  container<std::optional<std::string>>([](int i) { return std::optional<std::string>(long_str(i)); });
  container<std::variant<int, std::string>>([](int i) { return std::variant<int, std::string>(long_str(i)); });
  container<std::expected<std::string, int>>([](int i) { return std::expected<std::string, int>(long_str(i)); });
  container<std::expected<std::string, std::string>>([](int i) {
    return std::expected<std::string, std::string>(std::unexpect, long_str(i));
  });
  container<std::expected<void, std::string>>([](int i) { return std::expected<void, std::string>(std::unexpect, long_str(i)); });
  container<std::bitset<130>>([](int i) { return std::bitset<130>().set(i * 50); });
  container<std::complex<double>>([](int i) { return std::complex<double>(i, -i); });
  container<std::error_code>([](int i) { return std::error_code(i, std::generic_category()); });
  common<std::valarray<double>>([](int i) { return std::valarray<double>(double(i), 20); },
                                [](const auto& a, const auto& b) { return a.size() == b.size() && (a == b).min(); });
  common<std::locale>([](int i) { return i == 1 ? std::locale::classic() : std::locale(); },
                      [](const std::locale& a, const std::locale& b) { return a == b; });
  common<std::exception_ptr>(
      [](int i) { return i == 1 ? std::exception_ptr() : std::make_exception_ptr(i); },
      [](const std::exception_ptr& a, const std::exception_ptr& b) { return (a == nullptr) == (b == nullptr); });
  common<std::stringstream>([](int i) { return std::stringstream(long_str(i)); },
                            [](const std::stringstream& a, const std::stringstream& b) { return a.str() == b.str(); });
  common<std::basic_regex<char>>([](int i) { return std::regex(i == 1 ? "(a)(b)" : "x+", std::regex::icase); },
                                 [](const std::regex& a, const std::regex& b) {
                                   return a.mark_count() == b.mark_count() && a.flags() == b.flags() &&
                                          std::regex_match("AB", a) == std::regex_match("AB", b);
                                 });
  {
    std::regex r("(a)(b)(c)", std::regex::icase | std::regex::nosubs);
    auto f = r.flags();
    unsigned m = r.mark_count();
    std::regex& rr = r;
    r = std::move(rr);
    CHECK(r.flags() == f && r.mark_count() == m);
  }

  keeps<std::any>([](int i) { return std::any(long_str(i)); },
                  [](const std::any& a, const std::any& b) {
                    return a.has_value() && std::any_cast<std::string>(a) == std::any_cast<std::string>(b);
                  });
  auto call_eq = [](auto& a, auto& b) { return a && b && a() == b(); };
  auto make_fn = [](int i) { return [i, s = long_str(i)] { return i + int(s.size()); }; };
  common<std::function<int()>>(make_fn, call_eq);
  keeps<std::move_only_function<int()>>(make_fn, call_eq);
  keeps<std::copyable_function<int()>>(make_fn, call_eq);
  keeps<std::shared_ptr<std::string>>([](int i) { return std::make_shared<std::string>(long_str(i)); },
                                      [](const auto& a, const auto& b) { return a && *a == *b && a.use_count() == 1; });
  {
    auto sp = std::make_shared<int>(5);
    keeps<std::weak_ptr<int>>([&](int i) { return i == 1 ? std::weak_ptr<int>(sp) : std::weak_ptr<int>(); },
                              [](const auto& a, const auto& b) { return a.lock() == b.lock(); });
  }
  keeps<std::unique_ptr<std::string>>([](int i) { return std::make_unique<std::string>(long_str(i)); },
                                      [](const auto& a, const auto& b) { return a && *a == *b; });
  keeps<std::filesystem::path>([](int i) { return std::filesystem::path("dir") / long_str(i); }, eq);
  {
    // future and shared_future: no effects; promise and packaged_task: the swap form.
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::future<int>& fr = f;
    f = std::move(fr);
    CHECK(f.valid());
    std::promise<int>& pr = p;
    p = std::move(pr);
    p.set_value(11);
    CHECK(f.get() == 11);

    std::promise<int> p2;
    std::shared_future<int> sf = p2.get_future().share();
    std::shared_future<int>& sfr = sf;
    sf = std::move(sfr);
    CHECK(sf.valid());
    p2.set_value(12);
    CHECK(sf.get() == 12);

    std::packaged_task<int()> t([] { return 13; });
    std::future<int> tf = t.get_future();
    std::packaged_task<int()>& tr = t;
    t = std::move(tr);
    CHECK(t.valid());
    t();
    CHECK(tf.get() == 13);
    t.swap(t);
    CHECK(t.valid());
  }
  {
    std::mutex m;
    std::unique_lock<std::mutex> l(m);
    std::unique_lock<std::mutex>& lr = l;
    l = std::move(lr);
    CHECK(l.owns_lock() && l.mutex() == &m);
    l.swap(l);
    CHECK(l.owns_lock() && l.mutex() == &m);
    l.unlock();
    m.lock();
    m.unlock();

    std::shared_mutex sm;
    std::shared_lock<std::shared_mutex> s(sm);
    std::shared_lock<std::shared_mutex>& sr = s;
    s = std::move(sr);
    CHECK(s.owns_lock() && s.mutex() == &sm);
    s.unlock();
    sm.lock();
    sm.unlock();
  }
  {
    std::jthread j([](std::stop_token st) {
      while (!st.stop_requested()) std::this_thread::yield();
    });
    auto id = j.get_id();
    std::jthread& jr = j;
    j = std::move(jr);
    CHECK(j.joinable() && j.get_id() == id);
    j.swap(j);
    CHECK(j.joinable() && j.get_id() == id);
    std::thread t;
    std::thread& tr = t;
    t = std::move(tr);  // not joinable: no terminate
    CHECK(!t.joinable());
  }
  return 0;
}
