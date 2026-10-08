// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// FLAGS: -fcontracts -fcontract-evaluation-semantic=observe
// REQUIRES: gcc
// Contracts (P2900): a violation calls ::handle_contract_violation(const
// std::contracts::contract_violation&), which the program may replace. Compiled with the
// compiler's contracts flag (run-known.sh); a compiler without contracts skips it.
#include <contracts>
#include <cstdio>
#include <cstdlib>
void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::puts(v.semantic() == std::contracts::evaluation_semantic::observe ? "ok" : "FAIL");
  std::exit(0);
}
int f(int x) pre(x > 0) { return x; }
int main(int argc, char**) { return f(-argc) == 0 ? 0 : 1; }
