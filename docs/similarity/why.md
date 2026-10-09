## Why this exists

- libycxx was written by AI agents (Claude, directed by the author) from the C++ working draft,
  without access to other standard libraries' sources during development.
- The models behind those agents were trained on large amounts of public code that very likely
  includes libstdc++, libc++ and the MSVC STL. Not having the sources open does not by itself show
  that the result is independent: memorised code could resurface without anyone looking at a
  source file. That is why libycxx is not called a clean room.
- Licensing makes this concrete. libstdc++ is under the GPL v3 with the GCC Runtime Library
  Exception; libc++ and the MSVC STL are under the Apache License 2.0 with LLVM Exceptions. Code
  derived from them would carry their obligations, so adopters need evidence, not assurances.
- This analysis is that evidence. It measures libycxx's similarity to each of those libraries.
  As the yardstick, it also measures the libraries' similarity to each other and to a positive
  control of known derived code. It is regenerated from pinned sources on every build, and its
  judgments are committed in the repository, so anyone can check them and re-run it.
- It cannot prove a negative. It shows how libycxx compares with independently written libraries
  and with derived code, and it lists every match it could not explain.
