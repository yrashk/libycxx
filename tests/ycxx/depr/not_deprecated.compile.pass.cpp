// The non-deprecated neighbours of the Annex D entities are not diagnosed: the volatile members
// and non-member functions of an always lock-free atomic ([atomics.types.operations],
// [depr.atomics.volatile] covers only types that are not always lock-free), the cv forms of
// tuple_size other than volatile, path's templated and named observers ([fs.path.native.obs]),
// basic_format_arg::visit ([format.arg]), the friend swap of vector<bool>::reference
// ([vector.bool.pspc]), the non-deprecated errc enumerators and codecvt specializations.
// FLAGS: -Werror=deprecated-declarations -Werror=deprecated
#include <atomic>
#include <filesystem>
#include <format>
#include <iterator>
#include <limits>
#include <locale>
#include <ostream>
#include <system_error>
#include <tuple>
#include <vector>

struct Big { char c[64]; };

void f(std::ostream& out) {
  volatile std::atomic<int> a;
  a.store(1);
  (void)a.load();
  a++;
  a += 2;
  a.notify_one();
  std::atomic_store(&a, 1);
  (void)std::atomic_load(&a);
  std::atomic_fetch_add(&a, 1);
  std::atomic_store_add(&a, 1);
  std::atomic_notify_one(&a);
  volatile std::atomic<int*> p;
  p.store(nullptr);
  p++;
  std::atomic_fetch_add(&p, 1);
  (void)std::atomic_is_lock_free(&p);
  std::atomic<Big> b;
  std::atomic_store(&b, Big{});
  (void)std::atomic_load(&b);
  (void)b.load();
  (void)std::atomic_is_lock_free(&b);
  (void)a.load(std::memory_order::acquire);

  (void)std::tuple_size<const std::tuple<int>>::value;
  (void)std::tuple_size_v<std::tuple<int>>;
  (void)std::numeric_limits<double>::digits;
  (void)std::make_error_code(std::errc::io_error);

  std::filesystem::path path("a");
  (void)path.native();
  (void)path.string<char>();
  (void)path.native_encoded_string();
  (void)path.generic_string<char>();
  (void)path.generic_native_encoded_string();
  (void)path.u8string();

  int i = 1;
  auto store = std::make_format_args(i);
  std::format_args args(store);
  args.get(0).visit([](auto) {});

  std::vector<bool> v(2);
  swap(v[0], v[1]);
  v.swap(v);

  int arr[1]{};
  std::move_iterator<int*> m(arr);
  (void)*m;

  using F = std::codecvt<wchar_t, char, std::mbstate_t>;
  F* fp = nullptr;
  (void)fp;
  out << 'a' << "x";
}

int main() {}
