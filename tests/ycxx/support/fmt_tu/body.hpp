// The text each translation unit with <format> produces (internal linkage). Include after
// <format>, <vector>, <stack>, <queue> and flag.hpp, in any order.
#pragma once
#include <string>
#include "flag.hpp"

static std::string format_all() {
  std::vector<int> v{1, 2, 3};
  std::vector<bool> vb{true, false, true};
  std::stack<int> s;
  for (int i : {3, 1, 2}) s.push(i);
  std::queue<int> q;
  q.push(7);
  q.push(8);
  std::priority_queue<int> pq;
  for (int i : {5, 1, 9}) pq.push(i);
  std::vector<char> vc{'a', 'b'};
  std::vector<std::string> vs{"x", "y"};
  std::vector<std::vector<int>> vv{{1}, {2, 3}};
  std::stack<bool, std::vector<bool>> sb;
  sb.push(false);
  sb.push(true);
  return std::format("{}|{:n}|{::02}|{}|{:d}|{:#x}|{:*^7}|{}|{}|{}|{:s}|{::?}|{}|{}|{}|{}|{}|{}|{:e}|{}|{}|{:?}",
                     v, v, v, vb, vb[0], vb[2], Flag{vb[1]}, s, q, pq.top(), vc, vc, vs, vv, sb,
                     Flag{vb[0]}, 42, -7LL, 3.5, "str", std::string("s2"), 'c');
}

static std::wstring wformat_all() {
  std::vector<int> v{1, 2, 3};
  std::vector<bool> vb{true, false};
  std::queue<int> q;
  q.push(4);
  return std::format(L"{}|{}|{:d}|{}|{}|{:>4}", v, vb, vb[1], q, L"w", 12);
}
