// Exception-injection sweep over std::format, format_to, format_to_n, vformat and formatted_size:
// operator new, the allocator of the output container, a user formatter's format() and the
// output iterator's operations throw at their k-th call, for every k until formatting completes.
//   [format.functions]: "Throws: As specified in [format.err.report]" — format_error, or "any
//     exception thrown by ... formatter specializations"; [format.err.report]/1, and
//   [res.on.exception.handling]/1: the exception propagates, every operator new block and
//   every allocator block is freed, and every argument object is destroyed exactly once (the
//   arguments are passed by reference: format must not copy them, [format.arg.store]).
#include <format>
#include <iterator>
#include <string>
#include <vector>
#include "exc_new.hpp"

using namespace exh;

template <>
struct std::formatter<exh::T> : std::formatter<int> {
  auto format(const exh::T& t, std::format_context& ctx) const {
    point(virt);
    return std::formatter<int>::format(t.v, ctx);
  }
};

// An output iterator over a vector<char, alloc<char>> whose operations throw at their k-th call.
struct Out {
  using difference_type = std::ptrdiff_t;
  std::vector<char, alloc<char>>* v;
  Out& operator*() { return *this; }
  Out& operator=(char c) {
    point(iter_deref);
    v->push_back(c);
    return *this;
  }
  Out& operator++() {
    point(iter_inc);
    return *this;
  }
  Out operator++(int) {
    point(iter_inc);
    return *this;
  }
};

static const T args_g[3] = {T(1), T(22), T(333)};
static const std::string long_s(200, 'x');

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  sw("format(user formatter args + long string)", {virt, gnew, copy_ctor}, [] {
    return attempt([] {
      std::string s = std::format("{:>12} {} {:<20} {} {:*^40}", args_g[0], args_g[1], args_g[2], long_s, 3.25);
      CHECK(s.size() > 200);
    });
  });
  sw("vformat with make_format_args", {virt, gnew, copy_ctor}, [] {
    return attempt([] {
      std::string s = std::vformat("{0} {1} {2} {0} {3}", std::make_format_args(args_g[0], args_g[1], args_g[2], long_s));
    });
  });
  sw("format_to(Out over vector<char, alloc>)", {virt, iter_inc, iter_deref, allocation, gnew}, [] {
    std::vector<char, alloc<char>> v;
    return attempt([&] { std::format_to(Out{&v}, "{} {} {} {}", args_g[0], args_g[1], args_g[2], long_s); });
  });
  sw("format_to(back_inserter(vector<char, alloc>))", {virt, allocation, gnew}, [] {
    std::vector<char, alloc<char>> v;
    return attempt([&] { std::format_to(std::back_inserter(v), "{:>30}{}{}", args_g[0], long_s, args_g[2]); });
  });
  sw("format_to_n(Out, 50)", {virt, iter_inc, iter_deref, allocation, gnew}, [] {
    std::vector<char, alloc<char>> v;
    return attempt([&] { std::format_to_n(Out{&v}, 50, "{} {} {}", args_g[0], long_s, args_g[2]); });
  });
  sw("formatted_size", {virt, gnew}, [] {
    return attempt([] { (void)std::formatted_size("{} {} {}", args_g[0], long_s, args_g[2]); });
  });
  sw("format of ranges", {gnew}, [] {
    return attempt([] {
      std::string s = std::format("{}", std::vector<int>{1, 2, 3}) + std::format("{::>4}", std::vector<int>{4, 5});
    });
  });
  return finish();
}
