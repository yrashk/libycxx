#include <optional>
int x;
std::optional<int&> get() { return x; }
int& f() { return *get(); }
int main() { return &f() == &x ? 0 : 1; }
