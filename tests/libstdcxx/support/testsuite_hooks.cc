// Test-harness support (not part of libycxx): the out-of-line definitions that libstdc++'s
// testsuite_hooks.h declares. DejaGnu links every test with libtestc++.a, built from the
// testsuite's util/*.cc; tests/libstdcxx/lit.cfg.py builds the equivalent archive
// (build/lit-libstdcxx-<compiler>/libtestc++.a) from this file and util/testsuite_allocator.cc.
// The testsuite's own util/testsuite_hooks.cc is not used because it includes <cxxabi.h>
// (libsupc++'s ABI header) for verify_demangle, which only the abi/demangle tests call and
// lit.cfg.py excludes abi/. Everything else is written from the declarations and comments in
// testsuite_hooks.h:
//   set_memory_limits / set_file_limit   no limits (the header sets MEMLIMIT_MB to 0 when
//                                        _GLIBCXX_RES_LIMITS is undefined, as here)
//   run_tests_wrapped_locale / _env      run the callbacks with the global locale (and the
//                                        environment variable) set to the given name (left
//                                        set afterwards)
//   the counters' static data members    zero
//   semaphore                            a System V semaphore shared across fork()
//   test_tm                              a std::tm with the given fields
#include <testsuite_hooks.h>

#include <clocale>
#include <cstdlib>
#include <locale>
#include <stdexcept>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>

namespace __gnu_test {
void set_memory_limits(float) {}

void set_file_limit(unsigned long) {}

void run_tests_wrapped_locale(const char* name, const func_callback& l) {
  std::locale loc_name(name);
  std::locale::global(loc_name);
  const char* res = std::setlocale(LC_ALL, name);
  if (!res)
    std::__throw_runtime_error((std::string("LC_ALL for ") + name).c_str());
  std::string pre = res;
  for (int i = 0; i < l.size(); ++i)
    l.tests()[i]();
  std::string post = std::setlocale(LC_ALL, nullptr);
  VERIFY(pre == post);
}

void run_tests_wrapped_env(const char* name, const char* env, const func_callback& l) {
  std::locale loc_name(name);
  std::locale::global(loc_name);
  const char* old = std::getenv(env);
  std::string saved = old ? old : "";
  if (::setenv(env, name, 1) != 0)
    std::__throw_runtime_error((std::string(env) + " to " + name).c_str());
  for (int i = 0; i < l.size(); ++i)
    l.tests()[i]();
  ::setenv(env, saved.c_str(), 1);
}

object_counter::size_type object_counter::count = 0;
unsigned int copy_constructor::count_ = 0;
unsigned int copy_constructor::throw_on_ = 0;
unsigned int assignment_operator::count_ = 0;
unsigned int assignment_operator::throw_on_ = 0;
unsigned int destructor::_M_count = 0;
int copy_tracker::next_id_ = 0;

namespace {
// semctl's fourth argument; the program must declare it (POSIX semctl).
union semun {
  int val;
  struct semid_ds* buf;
  unsigned short* array;
};
} // namespace

semaphore::semaphore() : pid_(::getpid()) {
  // One semaphore, readable and alterable by the owner; destroyed by the creating process only.
  sem_set_ = ::semget(IPC_PRIVATE, 1, 0600);
  if (sem_set_ == -1)
    std::__throw_runtime_error("could not obtain semaphore set");
  semun val;
  val.val = 0;
  if (::semctl(sem_set_, 0, SETVAL, val) == -1)
    std::__throw_runtime_error("could not initialize semaphore");
}

semaphore::~semaphore() {
  if (pid_ == ::getpid()) {
    semun val;
    val.val = 0;
    ::semctl(sem_set_, 0, IPC_RMID, val);
  }
}

void semaphore::signal() {
  sembuf op = {0, 1, 0};
  if (::semop(sem_set_, &op, 1) == -1)
    std::__throw_runtime_error("could not signal semaphore");
}

void semaphore::wait() {
  sembuf op = {0, -1, SEM_UNDO};
  if (::semop(sem_set_, &op, 1) == -1)
    std::__throw_runtime_error("could not wait for semaphore");
}

std::tm test_tm(int sec, int min, int hour, int mday, int mon, int year, int wday, int yday, int isdst) {
  static std::tm tmp;
  tmp.tm_sec = sec;
  tmp.tm_min = min;
  tmp.tm_hour = hour;
  tmp.tm_mday = mday;
  tmp.tm_mon = mon;
  tmp.tm_year = year;
  tmp.tm_wday = wday;
  tmp.tm_yday = yday;
  tmp.tm_isdst = isdst;
  return tmp;
}
} // namespace __gnu_test
