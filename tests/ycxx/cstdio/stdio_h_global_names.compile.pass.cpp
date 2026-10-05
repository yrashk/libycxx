// [support.c.headers.other]/1: <stdio.h> places in the global namespace each name <cstdio>
// places in std ([cstdio.syn]: FILE, fpos_t, size_t and the functions remove ... perror).
// Only <stdio.h> is included. (The C23 additions: cstdio/c23_conversions.)
#include <stdio.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::clearerr;
using ::fclose;
using ::feof;
using ::ferror;
using ::fflush;
using ::fgetc;
using ::fgetpos;
using ::fgets;
using ::FILE;
using ::fopen;
using ::fpos_t;
using ::fprintf;
using ::fputc;
using ::fputs;
using ::fread;
using ::freopen;
using ::fscanf;
using ::fseek;
using ::fsetpos;
using ::ftell;
using ::fwrite;
using ::getc;
using ::getchar;
using ::perror;
using ::printf;
using ::putc;
using ::putchar;
using ::puts;
using ::remove;
using ::rename;
using ::rewind;
using ::scanf;
using ::setbuf;
using ::setvbuf;
using ::size_t;
using ::snprintf;
using ::sprintf;
using ::sscanf;
using ::tmpfile;
using ::tmpnam;
using ::ungetc;
using ::vfprintf;
using ::vfscanf;
using ::vprintf;
using ::vscanf;
using ::vsnprintf;
using ::vsprintf;
using ::vsscanf;

static_assert(same<decltype(::ftell(nullptr)), long>);
static_assert(same<decltype(::fread(nullptr, 1, 1, nullptr)), ::size_t>);
static_assert(EOF < 0 && BUFSIZ >= 256 && FOPEN_MAX >= 8 && FILENAME_MAX > 0 && L_tmpnam > 0 &&
              TMP_MAX >= 25);
static_assert(SEEK_SET != SEEK_CUR && SEEK_CUR != SEEK_END && _IOFBF != _IONBF && _IOLBF != _IONBF);

int main() { return 0; }
