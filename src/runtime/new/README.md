# The default replaceable allocation functions

One function per file, in each runtime archive (`src/runtime/new`: the forwarding forms shared by
the hosted and freestanding archives; `src/hosted/new`, `src/freestanding/new`: the forms that
allocate), so that a program can replace any subset ([replacement.functions]): a replaced
function is linked from the program, and the archive member holding the default is not.

They keep default visibility (DECISIONS §2): the compilers' implicit declarations give them that
under `-fvisibility=hidden`, so one replacement serves every image of the process. They are also
weak definitions (`[[gnu::weak]]`): a shared library that links libycxx statically holds its own
copy of the defaults, and on Mach-O a strong definition inside an image is bound at static link
time, so the shared library would keep calling its own copy whatever the program defines; dyld
coalesces a weak definition with the program's definition of the same name. On ELF a
default-visibility definition in a shared object is preemptible either way.
