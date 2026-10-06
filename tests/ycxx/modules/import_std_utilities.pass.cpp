// [std.modules]/2: `import std;` provides the general utilities, memory management, type traits,
// concepts, comparisons, bit manipulation, numerics, time and random numbers.
// MODULES: std
import std;
#include "module_check.hpp"

struct base {
  virtual ~base() = default;
  virtual int f() const { return 1; }
};
struct derived : base {
  int f() const override { return 2; }
};
struct triple {
  int a, b;
  double c;
};

int main() {
  std::optional<int> o = 3;
  CHECK(o.value_or(0) == 3 && !std::optional<int>(std::nullopt));
  std::variant<int, std::string> var = std::string("x");
  CHECK(std::holds_alternative<std::string>(var) && std::visit([](auto&& x) { return sizeof(x) > 0; }, var));
  std::expected<int, std::string> e = std::unexpected(std::string("bad"));
  CHECK(!e && e.error() == "bad");
  std::any a = 5;
  CHECK(std::any_cast<int>(a) == 5);
  auto t = std::make_tuple(1, 'c', 2.0);
  auto [x, y, z] = t;
  CHECK(x == 1 && y == 'c' && z == 2.0 && std::tuple_size_v<decltype(t)> == 3);
  std::pair<int, int> p{1, 2};
  CHECK(p < std::pair<int, int>(1, 3));
  auto u = std::make_unique<derived>();
  std::shared_ptr<base> sp = std::make_shared<derived>();
  std::weak_ptr<base> wp = sp;
  CHECK(u->f() == 2 && wp.lock()->f() == 2 && std::dynamic_pointer_cast<derived>(sp));
  std::allocator<int> al;
  int* q = std::allocator_traits<std::allocator<int>>::allocate(al, 2);
  std::construct_at(q, 7);
  CHECK(*q == 7);
  std::destroy_at(q);
  al.deallocate(q, 2);
  static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int> && std::integral<long>);
  static_assert(std::is_aggregate_v<triple> && std::same_as<std::common_type_t<int, long>, long>);
  static_assert(std::three_way_comparable<int> && (1 <=> 2) == std::strong_ordering::less);
  CHECK(std::popcount(0xFFu) == 8 && std::bit_ceil(5u) == 8u && std::byteswap(std::uint16_t{0x1234}) == 0x3412);
  CHECK(std::numbers::pi > 3.14 && std::numbers::e_v<float> > 2.7f);
  CHECK(std::numeric_limits<int>::max() == 2147483647 && std::cmp_less(-1, 1u));
  static_assert(std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>::den == 6);
  std::complex<double> c(1, 1);
  CHECK(std::abs(c * std::conj(c) - 2.0) < 1e-12);
  auto dur = std::chrono::milliseconds(1500);
  CHECK(std::chrono::duration_cast<std::chrono::seconds>(dur).count() == 1);
  auto ymd = std::chrono::year(2024) / std::chrono::February / 29;
  CHECK(ymd.ok() && std::chrono::sys_days(ymd).time_since_epoch().count() > 0);
  CHECK(std::chrono::system_clock::now().time_since_epoch().count() > 0);
  std::mt19937 gen(42);
  std::uniform_int_distribution<int> dist(1, 6);
  int roll = dist(gen);
  CHECK(roll >= 1 && roll <= 6);
  CHECK(std::source_location::current().line() > 0);
  CHECK(std::type_index(typeid(int)) == std::type_index(typeid(int)));
  CHECK(std::to_underlying(std::byte{3}) == 3 && std::to_integer<int>(std::byte{2} | std::byte{1}) == 3);
  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  CHECK(ec == std::errc::invalid_argument && ec.category() == std::generic_category());
  std::exchange(x, 9);
  CHECK(x == 9 && std::max({1, 5, 3}) == 5 && std::clamp(10, 0, 4) == 4);
  CHECK(std::filesystem::path("a/b.txt").extension() == ".txt");
  std::stop_source ss;
  CHECK(ss.request_stop() && ss.get_token().stop_requested());
  CHECK(std::hash<std::string>{}("a") == std::hash<std::string_view>{}("a"));
  return 0;
}
