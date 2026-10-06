// libycxx hosted runtime: <stacktrace> -- capturing the frames and symbolizing an address.
//
// Capture: _Unwind_Backtrace of the toolchain's unwinder (libgcc_s), the same one exception
// handling uses. Symbolization: the PAL names the loaded object containing the address and its
// load bias; the object file is mapped (through the PAL) and read as ELF:
//   - the function: the innermost subprogram or inlined subroutine of .debug_info containing
//     the address (so code inlined from another function is described by that function), else
//     the symbol of .symtab (else .dynsym; else what the dynamic linker knows) containing it;
//     demangled (demangle.cpp);
//   - the file and line: the DWARF line-number program of .debug_line (DWARF 2-5, 32- and
//     64-bit formats) whose row ranges contain the address.
// Not supported: compressed debug sections (SHF_COMPRESSED), separate debug files
// (.gnu_debuglink, build-id directories), split DWARF, and big-endian or non-ELF objects: the
// queries then return empty strings and 0. Mapped objects are cached (eight, least recently
// used first out); the cache is guarded by a mutex.
#include <stacktrace>
#include <mutex>
#include <ostream>

#include <cstring>
#include <vector>
#include <unwind.h>
#include <ycxx/pal.h>

#include "demangle.hpp"

namespace {

// ---- capture ---------------------------------------------------------------------------------------

struct capture_state {
  std::uintptr_t __ra;     // the return address of basic_stacktrace::current
  bool __started;          // the frame with that return address has been seen
  std::size_t __skip;      // frames still to skip after that
  std::uintptr_t* __buf;
  std::size_t n, count;
};

_Unwind_Reason_Code capture_frame(_Unwind_Context* __ctx, void* arg) {
  capture_state& __st = *static_cast<capture_state*>(arg);
  int before = 0;
  const std::uintptr_t __ip = _Unwind_GetIPInfo(__ctx, &before);
  if (__ip == 0)
    return _URC_END_OF_STACK;
  if (!__st.__started) {
    if (__ip != __st.__ra)
      return _URC_NO_REASON;
    __st.__started = true;
  }
  if (__st.__skip > 0) {
    --__st.__skip;
    return _URC_NO_REASON;
  }
  if (__st.count == __st.n)
    return _URC_END_OF_STACK;
  // A return address points after the call: the call instruction itself is one byte earlier.
  // A frame interrupted by a signal records the address of the next instruction to execute.
  if (__st.__buf != nullptr)
    __st.__buf[__st.count] = before ? __ip : __ip - 1;
  ++__st.count;
  return __st.count == __st.n ? _URC_END_OF_STACK : _URC_NO_REASON;
}

// ---- reading ELF and DWARF ----------------------------------------------------------------------

struct reader {
  const unsigned char* p;
  const unsigned char* end;
  bool ok = true;

  bool __need(std::size_t n) {
    if (!ok || static_cast<std::size_t>(end - p) < n)
      ok = false;
    return ok;
  }
  std::uint64_t __u(std::size_t n) { // little-endian
    if (!__need(n))
      return 0;
    std::uint64_t __v = 0;
    for (std::size_t i = 0; i < n; ++i)
      __v |= static_cast<std::uint64_t>(p[i]) << (8 * i);
    p += n;
    return __v;
  }
  std::uint64_t uleb() {
    std::uint64_t __v = 0;
    unsigned shift = 0;
    for (;;) {
      if (!__need(1))
        return 0;
      const unsigned char b = *p++;
      if (shift < 64)
        __v |= static_cast<std::uint64_t>(b & 0x7f) << shift;
      shift += 7;
      if (!(b & 0x80))
        return __v;
    }
  }
  std::int64_t sleb() {
    std::int64_t __v = 0;
    unsigned shift = 0;
    unsigned char b;
    do {
      if (!__need(1))
        return 0;
      b = *p++;
      if (shift < 64)
        __v |= static_cast<std::int64_t>(static_cast<std::uint64_t>(b & 0x7f) << shift);
      shift += 7;
    } while (b & 0x80);
    if (shift < 64 && (b & 0x40))
      __v |= -(static_cast<std::int64_t>(1) << shift);
    return __v;
  }
  const char* str() {
    const unsigned char* s = p;
    while (p != end && *p)
      ++p;
    if (p == end) {
      ok = false;
      return "";
    }
    ++p;
    return reinterpret_cast<const char*>(s);
  }
  void __skip(std::size_t n) {
    if (__need(n))
      p += n;
  }
};

struct section {
  const unsigned char* data = nullptr;
  std::size_t size = 0;
  std::uint64_t __addr = 0;
  std::uint32_t link = 0;
  std::uint64_t entsize = 0;
  bool usable() const { return data != nullptr && size != 0; }
};

// One mapped object file and the sections the queries use.
struct object_file {
  char path[4096] = {};
  const void* map = nullptr;
  __ycxx_pal_size map_size = 0;
  bool is64 = false;
  section symtab, symstr, dynsym, dynstr, line, line_str, str;
  section info, abbrev, ranges, rnglists, __addr, str_offsets;
  unsigned long long last_use = 0;
};

constexpr std::uint32_t shf_compressed = 0x800;
constexpr std::uint32_t sht_nobits = 8;

bool parse_elf(object_file& __f) {
  const auto* d = static_cast<const unsigned char*>(__f.map);
  const std::size_t size = __f.map_size;
  if (size < 64 || std::memcmp(d, "\177ELF", 4) != 0 || d[5] != 1 /* little-endian */)
    return false;
  __f.is64 = d[4] == 2;
  reader h{d, d + size};
  std::uint64_t shoff;
  std::uint32_t shentsize, shnum, shstrndx;
  if (__f.is64) {
    h.__skip(0x28);
    shoff = h.__u(8);
    h.__skip(0x3a - 0x30);
  } else {
    h.__skip(0x20);
    shoff = h.__u(4);
    h.__skip(0x2e - 0x24);
  }
  shentsize = static_cast<std::uint32_t>(h.__u(2));
  shnum = static_cast<std::uint32_t>(h.__u(2));
  shstrndx = static_cast<std::uint32_t>(h.__u(2));
  if (!h.ok || shoff == 0 || shoff >= size || shnum == 0 || shstrndx >= shnum ||
      shentsize < (__f.is64 ? 64u : 40u) || (size - shoff) / shentsize < shnum)
    return false;
  struct __raw {
    std::uint32_t name, type, link;
    std::uint64_t flags, __addr, offset, size, entsize;
  };
  const auto read_shdr = [&](std::uint32_t i) {
    reader r{d + shoff + std::uint64_t(i) * shentsize, d + size};
    __raw s{};
    s.name = static_cast<std::uint32_t>(r.__u(4));
    s.type = static_cast<std::uint32_t>(r.__u(4));
    const std::size_t __w = __f.is64 ? 8 : 4;
    s.flags = r.__u(__w);
    s.__addr = r.__u(__w);
    s.offset = r.__u(__w);
    s.size = r.__u(__w);
    s.link = static_cast<std::uint32_t>(r.__u(4));
    r.__u(4); // info
    r.__u(__w); // addralign
    s.entsize = r.__u(__w);
    return s;
  };
  const __raw __names = read_shdr(shstrndx);
  if (__names.offset >= size || __names.size > size - __names.offset)
    return false;
  const auto to_section = [&](const __raw& s) {
    section out;
    if (s.type == sht_nobits || (s.flags & shf_compressed) || s.offset >= size || s.size > size - s.offset)
      return out;
    out.data = d + s.offset;
    out.size = s.size;
    out.__addr = s.__addr;
    out.link = s.link;
    out.entsize = s.entsize;
    return out;
  };
  __raw all_symtab{}, all_dynsym{};
  bool have_symtab = false, have_dynsym = false;
  for (std::uint32_t i = 0; i < shnum; ++i) {
    const __raw s = read_shdr(i);
    if (s.name >= __names.size)
      continue;
    const char* __nm = reinterpret_cast<const char*>(d + __names.offset + s.name);
    const std::size_t max = __names.size - s.name;
    const auto is = [&](const char* __want) { return std::strncmp(__nm, __want, max) == 0 && std::strlen(__want) < max; };
    if (s.type == 2 /* SHT_SYMTAB */) {
      all_symtab = s;
      have_symtab = true;
    } else if (s.type == 11 /* SHT_DYNSYM */) {
      all_dynsym = s;
      have_dynsym = true;
    } else if (is(".debug_line")) {
      __f.line = to_section(s);
    } else if (is(".debug_line_str")) {
      __f.line_str = to_section(s);
    } else if (is(".debug_str")) {
      __f.str = to_section(s);
    } else if (is(".debug_info")) {
      __f.info = to_section(s);
    } else if (is(".debug_abbrev")) {
      __f.abbrev = to_section(s);
    } else if (is(".debug_ranges")) {
      __f.ranges = to_section(s);
    } else if (is(".debug_rnglists")) {
      __f.rnglists = to_section(s);
    } else if (is(".debug_addr")) {
      __f.__addr = to_section(s);
    } else if (is(".debug_str_offsets")) {
      __f.str_offsets = to_section(s);
    }
  }
  const auto with_strings = [&](const __raw& tab, section& syms, section& strs) {
    syms = to_section(tab);
    if (tab.link < shnum)
      strs = to_section(read_shdr(tab.link));
  };
  if (have_symtab)
    with_strings(all_symtab, __f.symtab, __f.symstr);
  if (have_dynsym)
    with_strings(all_dynsym, __f.dynsym, __f.dynstr);
  return true;
}

// The function symbol of syms containing the link-time address addr.
const char* find_symbol(const object_file& __f, const section& syms, const section& strs, std::uint64_t __addr) {
  if (!syms.usable() || !strs.usable())
    return nullptr;
  const std::size_t entsize = __f.is64 ? 24 : 16;
  const char* __best = nullptr;
  std::uint64_t __best_value = 0;
  bool best_sized = false;
  for (std::size_t __off = 0; __off + entsize <= syms.size; __off += entsize) {
    reader r{syms.data + __off, syms.data + syms.size};
    std::uint32_t name;
    unsigned char info;
    std::uint16_t shndx;
    std::uint64_t value, __sz;
    if (__f.is64) {
      name = static_cast<std::uint32_t>(r.__u(4));
      info = static_cast<unsigned char>(r.__u(1));
      r.__u(1);
      shndx = static_cast<std::uint16_t>(r.__u(2));
      value = r.__u(8);
      __sz = r.__u(8);
    } else {
      name = static_cast<std::uint32_t>(r.__u(4));
      value = r.__u(4);
      __sz = r.__u(4);
      info = static_cast<unsigned char>(r.__u(1));
      r.__u(1);
      shndx = static_cast<std::uint16_t>(r.__u(2));
    }
    const unsigned type = info & 0xf;
    if (shndx == 0 || (type != 2 /* STT_FUNC */ && type != 10 /* STT_GNU_IFUNC */) || name >= strs.size)
      continue;
    if (__addr < value)
      continue;
    const bool sized = __sz != 0 && __addr - value < __sz;
    if (__sz != 0 && !sized)
      continue;
    // Prefer a symbol whose size covers the address; else the nearest unsized one below it.
    if (__best == nullptr || (sized && !best_sized) || (sized == best_sized && value > __best_value)) {
      const char* __nm = reinterpret_cast<const char*>(strs.data + name);
      if (std::memchr(__nm, 0, strs.size - name) == nullptr || *__nm == '\0')
        continue;
      __best = __nm;
      __best_value = value;
      best_sized = sized;
    }
  }
  return __best;
}

// ---- DWARF line tables -----------------------------------------------------------------------------

struct line_result {
  std::string __file;
  std::uint32_t line = 0;
  bool found = false;
};

// A string of an attribute form in a DWARF 5 line-table header.
bool read_form_string(reader& r, std::uint64_t form, bool dwarf64, const object_file& __f, const char*& out) {
  const auto in = [&](const section& s, std::uint64_t __off) {
    if (!s.usable() || __off >= s.size || std::memchr(s.data + __off, 0, s.size - __off) == nullptr)
      return false;
    out = reinterpret_cast<const char*>(s.data + __off);
    return true;
  };
  switch (form) {
  case 0x08: // DW_FORM_string
    out = r.str();
    return r.ok;
  case 0x1f: // DW_FORM_line_strp
    return in(__f.line_str, r.__u(dwarf64 ? 8 : 4));
  case 0x0e: // DW_FORM_strp
    return in(__f.str, r.__u(dwarf64 ? 8 : 4));
  default:
    return false;
  }
}

// Skips a value of the attribute form; false for a form a line-table header cannot hold.
bool skip_form(reader& r, std::uint64_t form, bool dwarf64) {
  switch (form) {
  case 0x0b: r.__skip(1); return r.ok;                     // data1
  case 0x05: r.__skip(2); return r.ok;                     // data2
  case 0x06: r.__skip(4); return r.ok;                     // data4
  case 0x07: r.__skip(8); return r.ok;                     // data8
  case 0x1e: r.__skip(16); return r.ok;                    // data16
  case 0x0f: r.uleb(); return r.ok;                      // udata
  case 0x0d: r.sleb(); return r.ok;                      // sdata
  case 0x09: r.__skip(static_cast<std::size_t>(r.uleb())); return r.ok; // block
  case 0x08: r.str(); return r.ok;                       // string
  case 0x1f: case 0x0e: r.__skip(dwarf64 ? 8 : 4); return r.ok; // line_strp, strp
  default: return false;
  }
}

std::uint64_t read_form_udata(reader& r, std::uint64_t form) {
  switch (form) {
  case 0x0b: return r.__u(1);
  case 0x05: return r.__u(2);
  case 0x06: return r.__u(4);
  case 0x07: return r.__u(8);
  case 0x0f: return r.uleb();
  default: r.ok = false; return 0;
  }
}

std::string join_path(const char* __dir, const char* name) {
  if (name[0] == '/' || __dir == nullptr || __dir[0] == '\0')
    return name;
  std::string s = __dir;
  if (s.back() != '/')
    s += '/';
  s += name;
  return s;
}

// Runs one line-number program; true if it covers addr (then res holds the answer).
bool run_line_program(const object_file& __f, reader& __unit, bool dwarf64, std::uint64_t __addr, line_result& __res) {
  const std::uint16_t version = static_cast<std::uint16_t>(__unit.__u(2));
  if (version < 2 || version > 5)
    return false;
  std::size_t address_size = __f.is64 ? 8 : 4;
  if (version >= 5) {
    address_size = static_cast<std::size_t>(__unit.__u(1));
    __unit.__u(1); // segment selector size
  }
  const std::uint64_t header_length = __unit.__u(dwarf64 ? 8 : 4);
  if (!__unit.ok || header_length > static_cast<std::uint64_t>(__unit.end - __unit.p))
    return false;
  const unsigned char* __program = __unit.p + header_length;
  const unsigned min_inst = static_cast<unsigned>(__unit.__u(1));
  if (version >= 4)
    __unit.__u(1); // maximum operations per instruction (VLIW only)
  const bool default_is_stmt = __unit.__u(1) != 0;
  (void)default_is_stmt;
  const int line_base = static_cast<signed char>(__unit.__u(1));
  const unsigned line_range = static_cast<unsigned>(__unit.__u(1));
  const unsigned opcode_base = static_cast<unsigned>(__unit.__u(1));
  if (!__unit.ok || line_range == 0 || opcode_base == 0)
    return false;
  unsigned char std_lengths[256] = {};
  for (unsigned i = 1; i < opcode_base; ++i)
    std_lengths[i] = static_cast<unsigned char>(__unit.__u(1));

  // Directories and files.
  std::vector<const char*> dirs;
  struct file_entry {
    const char* name;
    std::uint64_t __dir;
  };
  std::vector<file_entry> files;
  if (version >= 5) {
    for (int table = 0; table < 2; ++table) {
      const unsigned nformats = static_cast<unsigned>(__unit.__u(1));
      std::uint64_t formats[32][2];
      if (nformats > 32)
        return false;
      for (unsigned i = 0; i < nformats; ++i) {
        formats[i][0] = __unit.uleb();
        formats[i][1] = __unit.uleb();
      }
      const std::uint64_t count = __unit.uleb();
      if (!__unit.ok || count > 1000000)
        return false;
      for (std::uint64_t e = 0; e < count; ++e) {
        const char* path = "";
        std::uint64_t __dir = 0;
        for (unsigned i = 0; i < nformats; ++i) {
          if (formats[i][0] == 1) { // DW_LNCT_path
            if (!read_form_string(__unit, formats[i][1], dwarf64, __f, path))
              return false;
          } else if (formats[i][0] == 2) { // DW_LNCT_directory_index
            __dir = read_form_udata(__unit, formats[i][1]);
          } else if (!skip_form(__unit, formats[i][1], dwarf64)) {
            return false;
          }
        }
        if (!__unit.ok)
          return false;
        if (table == 0)
          dirs.push_back(path);
        else
          files.push_back({path, __dir});
      }
    }
  } else {
    dirs.push_back(""); // directory 0: the compilation directory, not recorded here
    for (;;) {
      const char* d = __unit.str();
      if (!__unit.ok)
        return false;
      if (*d == '\0')
        break;
      dirs.push_back(d);
    }
    files.push_back({"", 0}); // file indices are 1-based before DWARF 5
    for (;;) {
      const char* name = __unit.str();
      if (!__unit.ok)
        return false;
      if (*name == '\0')
        break;
      const std::uint64_t __dir = __unit.uleb();
      __unit.uleb(); // modification time
      __unit.uleb(); // length
      files.push_back({name, __dir});
    }
  }
  if (!__unit.ok)
    return false;

  // The state machine ([DWARF5] 6.2.2). A row (address, file, line) covers the addresses up to
  // the next row of the same sequence.
  reader r{__program, __unit.end};
  std::uint64_t address = 0, __file = 1, line = 1;
  bool have_prev = false;
  std::uint64_t prev_address = 0, prev_file = 0, prev_line = 0;
  const auto emit = [&]() -> bool {
    if (have_prev && prev_address <= __addr && __addr < address) {
      if (prev_file < files.size()) {
        const file_entry& fe = files[prev_file];
        __res.__file = join_path(fe.__dir < dirs.size() ? dirs[fe.__dir] : nullptr, fe.name);
      }
      __res.line = static_cast<std::uint32_t>(prev_line);
      __res.found = true;
      return true;
    }
    have_prev = true;
    prev_address = address;
    prev_file = __file;
    prev_line = line;
    return false;
  };
  while (r.ok && r.p < r.end) {
    const unsigned op = static_cast<unsigned>(r.__u(1));
    if (op >= opcode_base) {
      const unsigned adj = op - opcode_base;
      address += std::uint64_t(adj / line_range) * min_inst;
      line += static_cast<std::uint64_t>(line_base + static_cast<int>(adj % line_range));
      if (emit())
        return true;
      continue;
    }
    switch (op) {
    case 0: { // extended opcode
      const std::uint64_t __len = r.uleb();
      if (!r.ok || __len == 0 || __len > static_cast<std::uint64_t>(r.end - r.p))
        return false;
      const unsigned char* next = r.p + __len;
      const unsigned __sub = static_cast<unsigned>(r.__u(1));
      if (__sub == 1) { // DW_LNE_end_sequence
        if (emit())
          return true;
        address = 0;
        __file = 1;
        line = 1;
        have_prev = false;
      } else if (__sub == 2) { // DW_LNE_set_address
        address = r.__u(static_cast<std::size_t>(__len - 1 <= 8 ? __len - 1 : address_size));
      }
      r.p = next;
      break;
    }
    case 1: // DW_LNS_copy
      if (emit())
        return true;
      break;
    case 2: address += r.uleb() * min_inst; break;           // DW_LNS_advance_pc
    case 3: line += static_cast<std::uint64_t>(r.sleb()); break; // DW_LNS_advance_line
    case 4: __file = r.uleb(); break;                            // DW_LNS_set_file
    case 5: r.uleb(); break;                                   // DW_LNS_set_column
    case 6: case 7: case 10: case 11: break;                   // flags
    case 8: address += std::uint64_t((255 - opcode_base) / line_range) * min_inst; break; // const_add_pc
    case 9: address += r.__u(2); break;                          // DW_LNS_fixed_advance_pc
    case 12: r.uleb(); break;                                  // DW_LNS_set_isa
    default:
      for (unsigned i = 0; i < std_lengths[op]; ++i)
        r.uleb();
      break;
    }
  }
  return false;
}

line_result find_line(const object_file& __f, std::uint64_t __addr) {
  line_result __res;
  if (!__f.line.usable())
    return __res;
  reader all{__f.line.data, __f.line.data + __f.line.size};
  while (all.ok && all.p < all.end) {
    std::uint64_t __len = all.__u(4);
    bool dwarf64 = false;
    if (__len == 0xffffffffu) {
      dwarf64 = true;
      __len = all.__u(8);
    }
    if (!all.ok || __len > static_cast<std::uint64_t>(all.end - all.p))
      break;
    reader __unit{all.p, all.p + __len};
    all.p += __len;
    if (run_line_program(__f, __unit, dwarf64, __addr, __res))
      return __res;
  }
  return __res;
}

// ---- DWARF debugging information entries: the function (inlined or not) at an address -------------
//
// The innermost DW_TAG_subprogram or DW_TAG_inlined_subroutine whose address ranges contain the
// address names the function: an inlined call's code is described by the function that was
// inlined. Its name is the DW_AT_linkage_name (demangled later) of the entry or of the entries
// its DW_AT_abstract_origin / DW_AT_specification refer to, else their DW_AT_name. DWARF 2-5,
// 32- and 64-bit formats, including DWARF 5's indexed forms (strx, addrx, rnglistx).

struct dwarf_unit {
  const unsigned char* begin = nullptr; // the unit header
  const unsigned char* dies = nullptr;  // the first entry
  const unsigned char* end = nullptr;
  bool dwarf64 = false;
  unsigned version = 0;
  unsigned addr_size = 8;
  std::uint64_t abbrev_offset = 0;
  std::uint64_t str_offsets_base = 0, addr_base = 0, rnglists_base = 0, base_address = 0;
  bool has_str_offsets_base = false, has_rnglists_base = false;
};

struct abbrev_entry {
  std::uint64_t tag = 0;
  bool children = false;
  const unsigned char* __attrs = nullptr; // (attribute, form [, implicit const]) pairs
};

// The unit header at p (in .debug_info); false if it is not one this reader handles.
bool read_unit_header(const object_file& __f, const unsigned char* p, dwarf_unit& __u) {
  reader r{p, __f.info.data + __f.info.size};
  std::uint64_t __len = r.__u(4);
  __u.dwarf64 = __len == 0xffffffffu;
  if (__u.dwarf64)
    __len = r.__u(8);
  if (!r.ok || __len > static_cast<std::uint64_t>(r.end - r.p))
    return false;
  __u.begin = p;
  __u.end = r.p + __len;
  r.end = __u.end;
  __u.version = static_cast<unsigned>(r.__u(2));
  if (__u.version < 2 || __u.version > 5)
    return false;
  if (__u.version >= 5) {
    const unsigned type = static_cast<unsigned>(r.__u(1));
    __u.addr_size = static_cast<unsigned>(r.__u(1));
    __u.abbrev_offset = r.__u(__u.dwarf64 ? 8 : 4);
    if (type == 4 || type == 5) // skeleton, split compile: dwo id
      r.__u(8);
    else if (type == 2 || type == 6) // type units
      return false;
  } else {
    __u.abbrev_offset = r.__u(__u.dwarf64 ? 8 : 4);
    __u.addr_size = static_cast<unsigned>(r.__u(1));
  }
  __u.dies = r.p;
  return r.ok && (__u.addr_size == 4 || __u.addr_size == 8);
}

// The abbreviation table of a unit, indexed by code.
bool read_abbrevs(const object_file& __f, std::uint64_t offset, std::vector<abbrev_entry>& out) {
  if (!__f.abbrev.usable() || offset >= __f.abbrev.size)
    return false;
  reader r{__f.abbrev.data + offset, __f.abbrev.data + __f.abbrev.size};
  out.clear();
  for (;;) {
    const std::uint64_t code = r.uleb();
    if (!r.ok)
      return false;
    if (code == 0)
      return true;
    if (code > 100000)
      return false;
    abbrev_entry e;
    e.tag = r.uleb();
    e.children = r.__u(1) != 0;
    e.__attrs = r.p;
    for (;;) {
      const std::uint64_t at = r.uleb(), form = r.uleb();
      if (!r.ok)
        return false;
      if (form == 0x21) // DW_FORM_implicit_const
        r.sleb();
      if (at == 0 && form == 0)
        break;
    }
    if (out.size() <= code)
      out.resize(code + 1);
    out[code] = e;
  }
}

struct attr_value {
  std::uint64_t form = 0;
  std::uint64_t __u = 0;
  const char* str = nullptr;
};

// Reads one attribute value of the given form; false for an unknown form.
bool read_attr(reader& r, std::uint64_t form, std::int64_t implicit, const dwarf_unit& __u, attr_value& __v) {
  __v.form = form;
  __v.str = nullptr;
  const std::size_t __off = __u.dwarf64 ? 8 : 4;
  switch (form) {
  case 0x01: __v.__u = r.__u(__u.addr_size); break;                 // addr
  case 0x03: r.__skip(static_cast<std::size_t>(r.__u(2))); break; // block2
  case 0x04: r.__skip(static_cast<std::size_t>(r.__u(4))); break; // block4
  case 0x05: __v.__u = r.__u(2); break;                            // data2
  case 0x06: __v.__u = r.__u(4); break;                            // data4
  case 0x07: __v.__u = r.__u(8); break;                            // data8
  case 0x08: __v.str = r.str(); break;                         // string
  case 0x09: case 0x18: r.__skip(static_cast<std::size_t>(r.uleb())); break; // block, exprloc
  case 0x0a: r.__skip(static_cast<std::size_t>(r.__u(1))); break; // block1
  case 0x0b: __v.__u = r.__u(1); break;                            // data1
  case 0x0c: __v.__u = r.__u(1); break;                            // flag
  case 0x0d: __v.__u = static_cast<std::uint64_t>(r.sleb()); break; // sdata
  case 0x0e: case 0x1f: case 0x17: __v.__u = r.__u(__off); break;    // strp, line_strp, sec_offset
  case 0x0f: __v.__u = r.uleb(); break;                          // udata
  case 0x10: __v.__u = r.__u(__u.version <= 2 ? __u.addr_size : __off); break; // ref_addr
  case 0x11: __v.__u = r.__u(1); break;                            // ref1
  case 0x12: __v.__u = r.__u(2); break;                            // ref2
  case 0x13: __v.__u = r.__u(4); break;                            // ref4
  case 0x14: __v.__u = r.__u(8); break;                            // ref8
  case 0x15: __v.__u = r.uleb(); break;                          // ref_udata
  case 0x16: {                                               // indirect
    const std::uint64_t real = r.uleb();
    return real != 0x16 && read_attr(r, real, implicit, __u, __v);
  }
  case 0x19: __v.__u = 1; break;                                 // flag_present
  case 0x1a: case 0x1b: case 0x22: case 0x23: __v.__u = r.uleb(); break; // strx, addrx, loclistx, rnglistx
  case 0x1c: __v.__u = r.__u(4); break;                            // ref_sup4
  case 0x1d: __v.__u = r.__u(__off); break;                          // strp_sup
  case 0x1e: r.__skip(16); break;                              // data16
  case 0x20: case 0x24: __v.__u = r.__u(8); break;                 // ref_sig8, ref_sup8
  case 0x21: __v.__u = static_cast<std::uint64_t>(implicit); break; // implicit_const
  case 0x25: case 0x29: __v.__u = r.__u(1); break;                 // strx1, addrx1
  case 0x26: case 0x2a: __v.__u = r.__u(2); break;                 // strx2, addrx2
  case 0x27: case 0x2b: __v.__u = r.__u(3); break;                 // strx3, addrx3
  case 0x28: case 0x2c: __v.__u = r.__u(4); break;                 // strx4, addrx4
  case 0x1f01: case 0x1f02: __v.__u = r.uleb(); break;           // GNU_addr_index, GNU_str_index
  case 0x1f20: case 0x1f21: __v.__u = r.__u(__off); break;           // GNU_ref_alt, GNU_strp_alt
  default: return false;
  }
  return r.ok;
}

bool is_strx(std::uint64_t form) { return form == 0x1a || (form >= 0x25 && form <= 0x28) || form == 0x1f02; }
bool is_addrx(std::uint64_t form) { return form == 0x1b || (form >= 0x29 && form <= 0x2c) || form == 0x1f01; }

const char* section_string(const section& s, std::uint64_t __off) {
  if (!s.usable() || __off >= s.size || std::memchr(s.data + __off, 0, s.size - __off) == nullptr)
    return nullptr;
  return reinterpret_cast<const char*>(s.data + __off);
}

// The string an attribute value denotes, if any.
const char* attr_string(const object_file& __f, const dwarf_unit& __u, const attr_value& __v) {
  if (__v.str)
    return __v.str;
  if (__v.form == 0x0e)
    return section_string(__f.str, __v.__u);
  if (__v.form == 0x1f)
    return section_string(__f.line_str, __v.__u);
  if (is_strx(__v.form) && __u.has_str_offsets_base) {
    const std::size_t __w = __u.dwarf64 ? 8 : 4;
    const std::uint64_t at = __u.str_offsets_base + __v.__u * __w;
    if (!__f.str_offsets.usable() || at + __w > __f.str_offsets.size)
      return nullptr;
    reader r{__f.str_offsets.data + at, __f.str_offsets.data + __f.str_offsets.size};
    return section_string(__f.str, r.__u(__w));
  }
  return nullptr;
}

// The address an attribute value denotes (addr or addrx forms).
bool attr_address(const object_file& __f, const dwarf_unit& __u, const attr_value& __v, std::uint64_t& out) {
  if (__v.form == 0x01) {
    out = __v.__u;
    return true;
  }
  if (is_addrx(__v.form)) {
    const std::uint64_t at = __u.addr_base + __v.__u * __u.addr_size;
    if (!__f.__addr.usable() || at + __u.addr_size > __f.__addr.size)
      return false;
    reader r{__f.__addr.data + at, __f.__addr.data + __f.__addr.size};
    out = r.__u(__u.addr_size);
    return r.ok;
  }
  return false;
}

// The attributes of an entry this reader uses.
struct die_info {
  std::uint64_t tag = 0;
  bool children = false;
  attr_value __low, __high, ranges, name, linkage, origin, __spec;
  bool has_low = false, has_high = false, has_ranges = false, has_name = false, has_linkage = false,
       has_origin = false, has_spec = false;
  attr_value str_offsets_base, addr_base, rnglists_base;
  bool has_str_offsets_base = false, has_addr_base = false, has_rnglists_base = false;
};

// Reads the entry at r (its code already known to be abbrev a); false on malformed data.
bool read_die(reader& r, const abbrev_entry& a, const dwarf_unit& __u, die_info& d) {
  d = die_info();
  d.tag = a.tag;
  d.children = a.children;
  reader __spec{a.__attrs, r.end};
  for (;;) {
    const std::uint64_t at = __spec.uleb(), form = __spec.uleb();
    std::int64_t implicit = 0;
    if (form == 0x21)
      implicit = __spec.sleb();
    if (!__spec.ok)
      return false;
    if (at == 0 && form == 0)
      return true;
    attr_value __v;
    if (!read_attr(r, form, implicit, __u, __v))
      return false;
    switch (at) {
    case 0x11: d.__low = __v; d.has_low = true; break;
    case 0x12: d.__high = __v; d.has_high = true; break;
    case 0x55: d.ranges = __v; d.has_ranges = true; break;
    case 0x03: d.name = __v; d.has_name = true; break;
    case 0x6e: case 0x2007: d.linkage = __v; d.has_linkage = true; break;
    case 0x31: d.origin = __v; d.has_origin = true; break;
    case 0x47: d.__spec = __v; d.has_spec = true; break;
    case 0x72: d.str_offsets_base = __v; d.has_str_offsets_base = true; break;
    case 0x73: d.addr_base = __v; d.has_addr_base = true; break;
    case 0x74: d.rnglists_base = __v; d.has_rnglists_base = true; break;
    default: break;
    }
  }
}

// Whether the address ranges of an entry (low_pc/high_pc or DW_AT_ranges) contain addr.
bool die_contains(const object_file& __f, const dwarf_unit& __u, const die_info& d, std::uint64_t __addr) {
  if (d.has_low && d.has_high) {
    std::uint64_t __low;
    if (!attr_address(__f, __u, d.__low, __low))
      return false;
    std::uint64_t __high;
    if (d.__high.form == 0x01 || is_addrx(d.__high.form)) {
      if (!attr_address(__f, __u, d.__high, __high))
        return false;
    } else {
      __high = __low + d.__high.__u;
    }
    return __low <= __addr && __addr < __high;
  }
  if (!d.has_ranges)
    return false;
  if (__u.version < 5) { // .debug_ranges: (begin, end) pairs relative to the base address
    if (!__f.ranges.usable() || d.ranges.__u >= __f.ranges.size)
      return false;
    reader r{__f.ranges.data + d.ranges.__u, __f.ranges.data + __f.ranges.size};
    const std::uint64_t all_ones = __u.addr_size == 8 ? ~std::uint64_t(0) : 0xffffffffu;
    std::uint64_t base = __u.base_address;
    while (r.ok) {
      const std::uint64_t b = r.__u(__u.addr_size), e = r.__u(__u.addr_size);
      if (!r.ok || (b == 0 && e == 0))
        return false;
      if (b == all_ones) {
        base = e;
        continue;
      }
      if (base + b <= __addr && __addr < base + e)
        return true;
    }
    return false;
  }
  // .debug_rnglists
  std::uint64_t __off = d.ranges.__u;
  if (d.ranges.form == 0x23) { // rnglistx: an index into the offsets after the header
    if (!__u.has_rnglists_base)
      return false;
    const std::size_t __w = __u.dwarf64 ? 8 : 4;
    const std::uint64_t at = __u.rnglists_base + __off * __w;
    if (!__f.rnglists.usable() || at + __w > __f.rnglists.size)
      return false;
    reader r{__f.rnglists.data + at, __f.rnglists.data + __f.rnglists.size};
    __off = __u.rnglists_base + r.__u(__w);
  }
  if (!__f.rnglists.usable() || __off >= __f.rnglists.size)
    return false;
  reader r{__f.rnglists.data + __off, __f.rnglists.data + __f.rnglists.size};
  std::uint64_t base = __u.base_address;
  const auto addrx = [&](std::uint64_t __idx, std::uint64_t& out) {
    attr_value __v;
    __v.form = 0x1b;
    __v.__u = __idx;
    return attr_address(__f, __u, __v, out);
  };
  while (r.ok) {
    const unsigned kind = static_cast<unsigned>(r.__u(1));
    std::uint64_t b = 0, e = 0;
    switch (kind) {
    case 0: return false; // end_of_list
    case 1:               // base_addressx
      if (!addrx(r.uleb(), base))
        return false;
      continue;
    case 2: // startx_endx
      if (!addrx(r.uleb(), b) || !addrx(r.uleb(), e))
        return false;
      break;
    case 3: // startx_length
      if (!addrx(r.uleb(), b))
        return false;
      e = b + r.uleb();
      break;
    case 4: // offset_pair
      b = base + r.uleb();
      e = base + r.uleb();
      break;
    case 5: base = r.__u(__u.addr_size); continue; // base_address
    case 6: // start_end
      b = r.__u(__u.addr_size);
      e = r.__u(__u.addr_size);
      break;
    case 7: // start_length
      b = r.__u(__u.addr_size);
      e = b + r.uleb();
      break;
    default: return false;
    }
    if (r.ok && b <= __addr && __addr < e)
      return true;
  }
  return false;
}

// Sets the unit's bases from its unit entry.
void apply_unit_die(const object_file& __f, dwarf_unit& __u, const die_info& d) {
  if (d.has_str_offsets_base) {
    __u.str_offsets_base = d.str_offsets_base.__u;
    __u.has_str_offsets_base = true;
  } else if (__u.version >= 5 && __f.str_offsets.usable()) { // the first contribution's header
    __u.str_offsets_base = __u.dwarf64 ? 16 : 8;
    __u.has_str_offsets_base = true;
  }
  if (d.has_addr_base)
    __u.addr_base = d.addr_base.__u;
  if (d.has_rnglists_base) {
    __u.rnglists_base = d.rnglists_base.__u;
    __u.has_rnglists_base = true;
  }
  if (d.has_low)
    attr_address(__f, __u, d.__low, __u.base_address);
}

// The unit containing the .debug_info offset off, its bases set; false if none.
bool unit_at(const object_file& __f, std::uint64_t __off, dwarf_unit& __u, std::vector<abbrev_entry>& abbrevs) {
  const unsigned char* p = __f.info.data;
  const unsigned char* end = __f.info.data + __f.info.size;
  while (p < end) {
    if (!read_unit_header(__f, p, __u)) {
      // Skip a unit of a kind this reader does not handle.
      reader r{p, end};
      std::uint64_t __len = r.__u(4);
      if (__len == 0xffffffffu)
        __len = r.__u(8);
      if (!r.ok || __len > static_cast<std::uint64_t>(end - r.p))
        return false;
      p = r.p + __len;
      continue;
    }
    if (__f.info.data + __off < __u.end) {
      if (__f.info.data + __off < __u.dies || !read_abbrevs(__f, __u.abbrev_offset, abbrevs))
        return false;
      reader r{__u.dies, __u.end};
      const std::uint64_t code = r.uleb();
      die_info d;
      if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].__attrs == nullptr ||
          !read_die(r, abbrevs[code], __u, d))
        return false;
      apply_unit_die(__f, __u, d);
      return true;
    }
    p = __u.end;
  }
  return false;
}

// The scopes enclosing the entry at .debug_info offset off in the unit u ("ns::C::"): the
// named namespaces, classes, structures, unions and functions among its ancestors.
std::string die_scope(const object_file& __f, const dwarf_unit& __u, const std::vector<abbrev_entry>& abbrevs,
                      std::uint64_t __off) {
  struct __level {
    std::uint64_t tag;
    const char* name;
  };
  std::vector<__level> stack;
  const unsigned char* target = __f.info.data + __off;
  reader r{__u.dies, __u.end};
  die_info d;
  while (r.ok && r.p < target) {
    const std::uint64_t code = r.uleb();
    if (!r.ok)
      break;
    if (code == 0) {
      if (!stack.empty())
        stack.pop_back();
      continue;
    }
    if (code >= abbrevs.size() || abbrevs[code].__attrs == nullptr || !read_die(r, abbrevs[code], __u, d))
      break;
    if (d.children)
      stack.push_back({d.tag, d.has_name ? attr_string(__f, __u, d.name) : nullptr});
  }
  std::string scope;
  for (const __level& __l : stack) {
    const bool named_scope = __l.tag == 0x39 || __l.tag == 0x02 || __l.tag == 0x13 || __l.tag == 0x17 || __l.tag == 0x2e;
    if (!named_scope)
      continue;
    if (__l.name != nullptr)
      scope += __l.name;
    else if (__l.tag == 0x39)
      scope += "(anonymous namespace)";
    else
      continue;
    scope += "::";
  }
  return scope;
}

// The name of the function the entry at .debug_info offset off describes.
bool die_name(const object_file& __f, std::uint64_t __off, int depth, std::string& name) {
  if (depth > 8)
    return false;
  dwarf_unit __u;
  std::vector<abbrev_entry> abbrevs;
  if (!unit_at(__f, __off, __u, abbrevs))
    return false;
  reader r{__f.info.data + __off, __u.end};
  const std::uint64_t code = r.uleb();
  die_info d;
  if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].__attrs == nullptr || !read_die(r, abbrevs[code], __u, d))
    return false;
  if (d.has_linkage)
    if (const char* s = attr_string(__f, __u, d.linkage)) {
      name = s;
      return true;
    }
  const auto ref_target = [&](const attr_value& __v, std::uint64_t& target) {
    if (__v.form == 0x10) { // ref_addr: an offset in .debug_info
      target = __v.__u;
      return true;
    }
    if (__v.form >= 0x11 && __v.form <= 0x15) { // relative to the unit
      target = static_cast<std::uint64_t>(__u.begin - __f.info.data) + __v.__u;
      return true;
    }
    return false;
  };
  std::uint64_t target;
  if (d.has_origin && ref_target(d.origin, target) && die_name(__f, target, depth + 1, name))
    return true;
  if (d.has_spec && ref_target(d.__spec, target) && die_name(__f, target, depth + 1, name))
    return true;
  if (d.has_name)
    if (const char* s = attr_string(__f, __u, d.name)) {
      name = die_scope(__f, __u, abbrevs, __off) + s; // no linkage name: qualify it by its scopes
      return true;
    }
  return false;
}

// The name of the innermost function (inlined or not) containing addr, from .debug_info.
bool find_function(const object_file& __f, std::uint64_t __addr, std::string& name) {
  if (!__f.info.usable() || !__f.abbrev.usable())
    return false;
  const unsigned char* p = __f.info.data;
  const unsigned char* end = __f.info.data + __f.info.size;
  std::vector<abbrev_entry> abbrevs;
  while (p < end) {
    dwarf_unit __u;
    if (!read_unit_header(__f, p, __u)) {
      reader r{p, end};
      std::uint64_t __len = r.__u(4);
      if (__len == 0xffffffffu)
        __len = r.__u(8);
      if (!r.ok || __len > static_cast<std::uint64_t>(end - r.p))
        return false;
      p = r.p + __len;
      continue;
    }
    p = __u.end;
    if (!read_abbrevs(__f, __u.abbrev_offset, abbrevs))
      continue;
    reader r{__u.dies, __u.end};
    // The unit entry: its bases, and whether the unit covers addr at all.
    std::uint64_t code = r.uleb();
    die_info d;
    if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].__attrs == nullptr || !read_die(r, abbrevs[code], __u, d))
      continue;
    apply_unit_die(__f, __u, d);
    if ((d.has_low && d.has_high) || d.has_ranges) {
      if (!die_contains(__f, __u, d, __addr))
        continue;
    }
    if (!d.children)
      continue;
    int depth = 1, best_depth = -1;
    const unsigned char* __best = nullptr;
    while (r.ok && r.p < r.end && depth > 0) {
      const unsigned char* at = r.p;
      code = r.uleb();
      if (!r.ok)
        break;
      if (code == 0) {
        --depth;
        continue;
      }
      if (code >= abbrevs.size() || abbrevs[code].__attrs == nullptr || !read_die(r, abbrevs[code], __u, d))
        break;
      if ((d.tag == 0x2e || d.tag == 0x1d) && depth > best_depth && die_contains(__f, __u, d, __addr)) {
        __best = at;
        best_depth = depth;
      }
      if (d.children)
        ++depth;
    }
    if (__best != nullptr)
      return die_name(__f, static_cast<std::uint64_t>(__best - __f.info.data), 0, name);
  }
  return false;
}

// ---- the object cache -------------------------------------------------------------------------------

constexpr int cache_size = 8;
std::mutex cache_mutex;
object_file __cache[cache_size];
unsigned long long cache_clock = 0;

// Copies a NUL-terminated path into a fixed buffer, truncating it (the buffer ends in NUL).
template <std::size_t _Np>
void copy_path(char (&__dst)[_Np], const char* __src) noexcept {
  std::size_t n = 0;
  for (; n + 1 < _Np && __src[n] != '\0'; ++n)
    __dst[n] = __src[n];
  __dst[n] = '\0';
}

// The cached, mapped object named path (loading it, evicting the least recently used one); null
// if it cannot be read. Called with cache_mutex held.
object_file* object_named(const char* path) {
  object_file* victim = &__cache[0];
  for (object_file& __f : __cache) {
    if (__f.map != nullptr && std::strcmp(__f.path, path) == 0) {
      __f.last_use = ++cache_clock;
      return &__f;
    }
    if (__f.map == nullptr || (victim->map != nullptr && __f.last_use < victim->last_use))
      victim = &__f;
  }
  if (victim->map != nullptr)
    __ycxx_pal_unmap_file(victim->map, victim->map_size);
  *victim = object_file();
  if (__ycxx_pal_map_file(path, &victim->map, &victim->map_size) != 0) {
    victim->map = nullptr;
    return nullptr;
  }
  copy_path(victim->path, path);
  if (!parse_elf(*victim)) {
    // Nothing usable; the mapping stays cached as a negative answer.
    const __ycxx_pal_size size = victim->map_size;
    const void* map = victim->map;
    *victim = object_file();
    victim->map = map;
    victim->map_size = size;
    copy_path(victim->path, path);
  }
  victim->last_use = ++cache_clock;
  return victim;
}

struct symbolized {
  std::string function, __file;
  std::uint32_t line = 0;
};

symbolized symbolize(std::uintptr_t __pc, bool want_function, bool want_line) {
  symbolized out;
  char path[4096];
  __ycxx_pal_handle __bias = 0;
  const char* __raw_name = nullptr;
  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (__ycxx_pal_object_of(__pc, path, sizeof path, &__bias) == 0) {
      if (object_file* __f = object_named(path)) {
        const std::uint64_t __addr = __pc - __bias;
        if (want_function && !find_function(*__f, __addr, out.function)) {
          __raw_name = find_symbol(*__f, __f->symtab, __f->symstr, __addr);
          if (__raw_name == nullptr)
            __raw_name = find_symbol(*__f, __f->dynsym, __f->dynstr, __addr);
          if (__raw_name != nullptr)
            out.function = __raw_name;
        }
        if (want_line) {
          line_result lr = find_line(*__f, __addr);
          if (lr.found) {
            out.__file = static_cast<std::string&&>(lr.__file);
            out.line = lr.line;
          }
        }
      }
    }
  }
  if (want_function && out.function.empty()) {
    __ycxx_pal_handle start;
    if (__ycxx_pal_dynamic_symbol(__pc, &__raw_name, &start) == 0 && __raw_name != nullptr)
      out.function = __raw_name;
  }
  if (!out.function.empty()) {
    std::string d;
    if (__ycxx::__detail::__demangle(out.function.c_str(), d))
      out.function = static_cast<std::string&&>(d);
  }
  return out;
}

} // namespace

std::size_t __ycxx::__detail::__stacktrace_capture(const void* __ra, std::size_t __skip, std::uintptr_t* __buf,
                                             std::size_t n) noexcept {
  if (n == 0)
    return 0;
  capture_state __st{reinterpret_cast<std::uintptr_t>(__ra), false, __skip, __buf, n, 0};
  _Unwind_Backtrace(capture_frame, &__st);
  return __st.count;
}

std::string __ycxx::__detail::__stacktrace_describe(std::uintptr_t __pc, __stacktrace_query what) {
  if (what == __stacktrace_query::description)
    return symbolize(__pc, true, false).function;
  return symbolize(__pc, false, true).__file;
}

std::uint_least32_t __ycxx::__detail::__stacktrace_line(std::uintptr_t __pc) { return symbolize(__pc, false, true).line; }

std::string std::to_string(const stacktrace_entry& __f) {
  if (!__f)
    return std::string();
  symbolized s = symbolize(__f.native_handle(), true, true);
  std::string out;
  if (!s.function.empty()) {
    out = static_cast<std::string&&>(s.function);
  } else { // no symbol: the address
    char hex[2 * sizeof(std::uintptr_t)];
    char* p = hex + sizeof hex;
    std::uintptr_t __v = __f.native_handle();
    do {
      *--p = "0123456789abcdef"[__v & 0xf];
      __v >>= 4;
    } while (__v != 0);
    out = "0x";
    out.append(p, hex + sizeof hex);
  }
  if (!s.__file.empty()) {
    out += " at ";
    out += s.__file;
    if (s.line != 0) {
      out += ':';
      out += std::to_string(s.line);
    }
  }
  return out;
}

std::ostream& std::operator<<(std::ostream& __os, const stacktrace_entry& __f) { return __os << std::to_string(__f); }
