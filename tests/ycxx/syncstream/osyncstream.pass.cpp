// [syncstream.osyncstream.members]/1: emit() "calls sb.emit()" and on false sets badbit;
// [syncstream.syncbuf.members]/1: emit() "Atomically transfers the associated output of *this
// to the stream buffer *wrapped"; output is held until emit() or destruction
// ([syncstream.syncbuf.cons]: the destructor calls emit()); "wrapped->pubsync() is called if
// and only if a call was made to sync() since the most recent call to emit()" (and with
// emit_on_sync, sync() emits); get_wrapped().
// Example 3: nested osyncstreams on the same wrapped buffer.
#include <syncstream>
#include <sstream>
#include <ostream>
#include <string>
#include "check.hpp"

struct CountingBuf : std::stringbuf {
  int syncs = 0;
  int sync() override { ++syncs; return 0; }
};

int main() {
  std::ostringstream target;
  {
    std::osyncstream out(target);
    CHECK(out.get_wrapped() == target.rdbuf());
    out << "Hello, " << 42;
    CHECK(target.str().empty());  // held
    out.emit();
    CHECK(target.str() == "Hello, 42");
    out << " more";
    CHECK(target.str() == "Hello, 42");
  }  // destructor emits
  CHECK(target.str() == "Hello, 42 more");

  // Example 3
  std::ostringstream t2;
  {
    std::osyncstream bout1(t2);
    bout1 << "Hello, ";
    {
      std::osyncstream(bout1.get_wrapped()) << "Goodbye, " << "Planet!" << '\n';
    }
    bout1 << "World!" << '\n';
  }
  CHECK(t2.str() == "Goodbye, Planet!\nHello, World!\n");

  // flush: pubsync on the wrapped buffer only at the following emit()
  CountingBuf cb;
  {
    std::osyncstream s(&cb);
    s << "a" << std::flush;
    CHECK(cb.syncs == 0 && cb.str().empty());
    s.emit();
    CHECK(cb.syncs == 1 && cb.str() == "a");
    s << "b";
    s.emit();
    CHECK(cb.syncs == 1);  // no sync() since the last emit()
    s << std::emit_on_flush << "c" << std::flush;
    CHECK(cb.str() == "abc");  // emitted by the flush
    s << std::noemit_on_flush << "d" << std::flush_emit;
    CHECK(cb.str() == "abcd");
  }

  return 0;
}
