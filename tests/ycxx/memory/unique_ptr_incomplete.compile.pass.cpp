// [unique.ptr.general]/5: "The template parameter T of unique_ptr may be an incomplete
// type." Only the destructor (and reset) need the default deleter, which needs T complete
// ([unique.ptr.single.dtor] note 1); declaring, default-constructing (in a context where the
// destructor is not used), moving and observing are fine with T incomplete. (The array
// form is not used: [unique.ptr.runtime.general] requires a complete T there.)
#include <memory>
#include <utility>

struct Incomplete;

struct Pimpl {
  std::unique_ptr<Incomplete> impl;
  Pimpl();
  ~Pimpl();
  Pimpl(Pimpl&&) noexcept;
  bool has() const { return impl != nullptr; }
  Incomplete* get() const { return impl.get(); }
};

void swap_them(std::unique_ptr<Incomplete>& a, std::unique_ptr<Incomplete>& b) { a.swap(b); }
Incomplete* take(std::unique_ptr<Incomplete>& a) { return a.release(); }

struct Incomplete {
  int v;
};

Pimpl::Pimpl() = default;
Pimpl::~Pimpl() = default;
Pimpl::Pimpl(Pimpl&&) noexcept = default;

int main() {}
