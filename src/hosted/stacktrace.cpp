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
  std::uintptr_t ra;     // the return address of basic_stacktrace::current
  bool started;          // the frame with that return address has been seen
  std::size_t skip;      // frames still to skip after that
  std::uintptr_t* buf;
  std::size_t n, count;
};

_Unwind_Reason_Code capture_frame(_Unwind_Context* ctx, void* arg) {
  capture_state& st = *static_cast<capture_state*>(arg);
  int before = 0;
  const std::uintptr_t ip = _Unwind_GetIPInfo(ctx, &before);
  if (ip == 0)
    return _URC_END_OF_STACK;
  if (!st.started) {
    if (ip != st.ra)
      return _URC_NO_REASON;
    st.started = true;
  }
  if (st.skip > 0) {
    --st.skip;
    return _URC_NO_REASON;
  }
  if (st.count == st.n)
    return _URC_END_OF_STACK;
  // A return address points after the call: the call instruction itself is one byte earlier.
  // A frame interrupted by a signal records the address of the next instruction to execute.
  if (st.buf != nullptr)
    st.buf[st.count] = before ? ip : ip - 1;
  ++st.count;
  return st.count == st.n ? _URC_END_OF_STACK : _URC_NO_REASON;
}

// ---- reading ELF and DWARF ----------------------------------------------------------------------

struct reader {
  const unsigned char* p;
  const unsigned char* end;
  bool ok = true;

  bool need(std::size_t n) {
    if (!ok || static_cast<std::size_t>(end - p) < n)
      ok = false;
    return ok;
  }
  std::uint64_t u(std::size_t n) { // little-endian
    if (!need(n))
      return 0;
    std::uint64_t v = 0;
    for (std::size_t i = 0; i < n; ++i)
      v |= static_cast<std::uint64_t>(p[i]) << (8 * i);
    p += n;
    return v;
  }
  std::uint64_t uleb() {
    std::uint64_t v = 0;
    unsigned shift = 0;
    for (;;) {
      if (!need(1))
        return 0;
      const unsigned char b = *p++;
      if (shift < 64)
        v |= static_cast<std::uint64_t>(b & 0x7f) << shift;
      shift += 7;
      if (!(b & 0x80))
        return v;
    }
  }
  std::int64_t sleb() {
    std::int64_t v = 0;
    unsigned shift = 0;
    unsigned char b;
    do {
      if (!need(1))
        return 0;
      b = *p++;
      if (shift < 64)
        v |= static_cast<std::int64_t>(static_cast<std::uint64_t>(b & 0x7f) << shift);
      shift += 7;
    } while (b & 0x80);
    if (shift < 64 && (b & 0x40))
      v |= -(static_cast<std::int64_t>(1) << shift);
    return v;
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
  void skip(std::size_t n) {
    if (need(n))
      p += n;
  }
};

struct section {
  const unsigned char* data = nullptr;
  std::size_t size = 0;
  std::uint64_t addr = 0;
  std::uint32_t link = 0;
  std::uint64_t entsize = 0;
  bool usable() const { return data != nullptr && size != 0; }
};

// One mapped object file and the sections the queries use.
struct object_file {
  char path[4096] = {};
  const void* map = nullptr;
  ycxx_pal_size map_size = 0;
  bool is64 = false;
  section symtab, symstr, dynsym, dynstr, line, line_str, str;
  section info, abbrev, ranges, rnglists, addr, str_offsets;
  unsigned long long last_use = 0;
};

constexpr std::uint32_t shf_compressed = 0x800;
constexpr std::uint32_t sht_nobits = 8;

bool parse_elf(object_file& f) {
  const auto* d = static_cast<const unsigned char*>(f.map);
  const std::size_t size = f.map_size;
  if (size < 64 || std::memcmp(d, "\177ELF", 4) != 0 || d[5] != 1 /* little-endian */)
    return false;
  f.is64 = d[4] == 2;
  reader h{d, d + size};
  std::uint64_t shoff;
  std::uint32_t shentsize, shnum, shstrndx;
  if (f.is64) {
    h.skip(0x28);
    shoff = h.u(8);
    h.skip(0x3a - 0x30);
  } else {
    h.skip(0x20);
    shoff = h.u(4);
    h.skip(0x2e - 0x24);
  }
  shentsize = static_cast<std::uint32_t>(h.u(2));
  shnum = static_cast<std::uint32_t>(h.u(2));
  shstrndx = static_cast<std::uint32_t>(h.u(2));
  if (!h.ok || shoff == 0 || shoff >= size || shnum == 0 || shstrndx >= shnum ||
      shentsize < (f.is64 ? 64u : 40u) || (size - shoff) / shentsize < shnum)
    return false;
  struct raw {
    std::uint32_t name, type, link;
    std::uint64_t flags, addr, offset, size, entsize;
  };
  const auto read_shdr = [&](std::uint32_t i) {
    reader r{d + shoff + std::uint64_t(i) * shentsize, d + size};
    raw s{};
    s.name = static_cast<std::uint32_t>(r.u(4));
    s.type = static_cast<std::uint32_t>(r.u(4));
    const std::size_t w = f.is64 ? 8 : 4;
    s.flags = r.u(w);
    s.addr = r.u(w);
    s.offset = r.u(w);
    s.size = r.u(w);
    s.link = static_cast<std::uint32_t>(r.u(4));
    r.u(4); // info
    r.u(w); // addralign
    s.entsize = r.u(w);
    return s;
  };
  const raw names = read_shdr(shstrndx);
  if (names.offset >= size || names.size > size - names.offset)
    return false;
  const auto to_section = [&](const raw& s) {
    section out;
    if (s.type == sht_nobits || (s.flags & shf_compressed) || s.offset >= size || s.size > size - s.offset)
      return out;
    out.data = d + s.offset;
    out.size = s.size;
    out.addr = s.addr;
    out.link = s.link;
    out.entsize = s.entsize;
    return out;
  };
  raw all_symtab{}, all_dynsym{};
  bool have_symtab = false, have_dynsym = false;
  for (std::uint32_t i = 0; i < shnum; ++i) {
    const raw s = read_shdr(i);
    if (s.name >= names.size)
      continue;
    const char* nm = reinterpret_cast<const char*>(d + names.offset + s.name);
    const std::size_t max = names.size - s.name;
    const auto is = [&](const char* want) { return std::strncmp(nm, want, max) == 0 && std::strlen(want) < max; };
    if (s.type == 2 /* SHT_SYMTAB */) {
      all_symtab = s;
      have_symtab = true;
    } else if (s.type == 11 /* SHT_DYNSYM */) {
      all_dynsym = s;
      have_dynsym = true;
    } else if (is(".debug_line")) {
      f.line = to_section(s);
    } else if (is(".debug_line_str")) {
      f.line_str = to_section(s);
    } else if (is(".debug_str")) {
      f.str = to_section(s);
    } else if (is(".debug_info")) {
      f.info = to_section(s);
    } else if (is(".debug_abbrev")) {
      f.abbrev = to_section(s);
    } else if (is(".debug_ranges")) {
      f.ranges = to_section(s);
    } else if (is(".debug_rnglists")) {
      f.rnglists = to_section(s);
    } else if (is(".debug_addr")) {
      f.addr = to_section(s);
    } else if (is(".debug_str_offsets")) {
      f.str_offsets = to_section(s);
    }
  }
  const auto with_strings = [&](const raw& tab, section& syms, section& strs) {
    syms = to_section(tab);
    if (tab.link < shnum)
      strs = to_section(read_shdr(tab.link));
  };
  if (have_symtab)
    with_strings(all_symtab, f.symtab, f.symstr);
  if (have_dynsym)
    with_strings(all_dynsym, f.dynsym, f.dynstr);
  return true;
}

// The function symbol of syms containing the link-time address addr.
const char* find_symbol(const object_file& f, const section& syms, const section& strs, std::uint64_t addr) {
  if (!syms.usable() || !strs.usable())
    return nullptr;
  const std::size_t entsize = f.is64 ? 24 : 16;
  const char* best = nullptr;
  std::uint64_t best_value = 0;
  bool best_sized = false;
  for (std::size_t off = 0; off + entsize <= syms.size; off += entsize) {
    reader r{syms.data + off, syms.data + syms.size};
    std::uint32_t name;
    unsigned char info;
    std::uint16_t shndx;
    std::uint64_t value, sz;
    if (f.is64) {
      name = static_cast<std::uint32_t>(r.u(4));
      info = static_cast<unsigned char>(r.u(1));
      r.u(1);
      shndx = static_cast<std::uint16_t>(r.u(2));
      value = r.u(8);
      sz = r.u(8);
    } else {
      name = static_cast<std::uint32_t>(r.u(4));
      value = r.u(4);
      sz = r.u(4);
      info = static_cast<unsigned char>(r.u(1));
      r.u(1);
      shndx = static_cast<std::uint16_t>(r.u(2));
    }
    const unsigned type = info & 0xf;
    if (shndx == 0 || (type != 2 /* STT_FUNC */ && type != 10 /* STT_GNU_IFUNC */) || name >= strs.size)
      continue;
    if (addr < value)
      continue;
    const bool sized = sz != 0 && addr - value < sz;
    if (sz != 0 && !sized)
      continue;
    // Prefer a symbol whose size covers the address; else the nearest unsized one below it.
    if (best == nullptr || (sized && !best_sized) || (sized == best_sized && value > best_value)) {
      const char* nm = reinterpret_cast<const char*>(strs.data + name);
      if (std::memchr(nm, 0, strs.size - name) == nullptr || *nm == '\0')
        continue;
      best = nm;
      best_value = value;
      best_sized = sized;
    }
  }
  return best;
}

// ---- DWARF line tables -----------------------------------------------------------------------------

struct line_result {
  std::string file;
  std::uint32_t line = 0;
  bool found = false;
};

// A string of an attribute form in a DWARF 5 line-table header.
bool read_form_string(reader& r, std::uint64_t form, bool dwarf64, const object_file& f, const char*& out) {
  const auto in = [&](const section& s, std::uint64_t off) {
    if (!s.usable() || off >= s.size || std::memchr(s.data + off, 0, s.size - off) == nullptr)
      return false;
    out = reinterpret_cast<const char*>(s.data + off);
    return true;
  };
  switch (form) {
  case 0x08: // DW_FORM_string
    out = r.str();
    return r.ok;
  case 0x1f: // DW_FORM_line_strp
    return in(f.line_str, r.u(dwarf64 ? 8 : 4));
  case 0x0e: // DW_FORM_strp
    return in(f.str, r.u(dwarf64 ? 8 : 4));
  default:
    return false;
  }
}

// Skips a value of the attribute form; false for a form a line-table header cannot hold.
bool skip_form(reader& r, std::uint64_t form, bool dwarf64) {
  switch (form) {
  case 0x0b: r.skip(1); return r.ok;                     // data1
  case 0x05: r.skip(2); return r.ok;                     // data2
  case 0x06: r.skip(4); return r.ok;                     // data4
  case 0x07: r.skip(8); return r.ok;                     // data8
  case 0x1e: r.skip(16); return r.ok;                    // data16
  case 0x0f: r.uleb(); return r.ok;                      // udata
  case 0x0d: r.sleb(); return r.ok;                      // sdata
  case 0x09: r.skip(static_cast<std::size_t>(r.uleb())); return r.ok; // block
  case 0x08: r.str(); return r.ok;                       // string
  case 0x1f: case 0x0e: r.skip(dwarf64 ? 8 : 4); return r.ok; // line_strp, strp
  default: return false;
  }
}

std::uint64_t read_form_udata(reader& r, std::uint64_t form) {
  switch (form) {
  case 0x0b: return r.u(1);
  case 0x05: return r.u(2);
  case 0x06: return r.u(4);
  case 0x07: return r.u(8);
  case 0x0f: return r.uleb();
  default: r.ok = false; return 0;
  }
}

std::string join_path(const char* dir, const char* name) {
  if (name[0] == '/' || dir == nullptr || dir[0] == '\0')
    return name;
  std::string s = dir;
  if (s.back() != '/')
    s += '/';
  s += name;
  return s;
}

// Runs one line-number program; true if it covers addr (then res holds the answer).
bool run_line_program(const object_file& f, reader& unit, bool dwarf64, std::uint64_t addr, line_result& res) {
  const std::uint16_t version = static_cast<std::uint16_t>(unit.u(2));
  if (version < 2 || version > 5)
    return false;
  std::size_t address_size = f.is64 ? 8 : 4;
  if (version >= 5) {
    address_size = static_cast<std::size_t>(unit.u(1));
    unit.u(1); // segment selector size
  }
  const std::uint64_t header_length = unit.u(dwarf64 ? 8 : 4);
  if (!unit.ok || header_length > static_cast<std::uint64_t>(unit.end - unit.p))
    return false;
  const unsigned char* program = unit.p + header_length;
  const unsigned min_inst = static_cast<unsigned>(unit.u(1));
  if (version >= 4)
    unit.u(1); // maximum operations per instruction (VLIW only)
  const bool default_is_stmt = unit.u(1) != 0;
  (void)default_is_stmt;
  const int line_base = static_cast<signed char>(unit.u(1));
  const unsigned line_range = static_cast<unsigned>(unit.u(1));
  const unsigned opcode_base = static_cast<unsigned>(unit.u(1));
  if (!unit.ok || line_range == 0 || opcode_base == 0)
    return false;
  unsigned char std_lengths[256] = {};
  for (unsigned i = 1; i < opcode_base; ++i)
    std_lengths[i] = static_cast<unsigned char>(unit.u(1));

  // Directories and files.
  std::vector<const char*> dirs;
  struct file_entry {
    const char* name;
    std::uint64_t dir;
  };
  std::vector<file_entry> files;
  if (version >= 5) {
    for (int table = 0; table < 2; ++table) {
      const unsigned nformats = static_cast<unsigned>(unit.u(1));
      std::uint64_t formats[32][2];
      if (nformats > 32)
        return false;
      for (unsigned i = 0; i < nformats; ++i) {
        formats[i][0] = unit.uleb();
        formats[i][1] = unit.uleb();
      }
      const std::uint64_t count = unit.uleb();
      if (!unit.ok || count > 1000000)
        return false;
      for (std::uint64_t e = 0; e < count; ++e) {
        const char* path = "";
        std::uint64_t dir = 0;
        for (unsigned i = 0; i < nformats; ++i) {
          if (formats[i][0] == 1) { // DW_LNCT_path
            if (!read_form_string(unit, formats[i][1], dwarf64, f, path))
              return false;
          } else if (formats[i][0] == 2) { // DW_LNCT_directory_index
            dir = read_form_udata(unit, formats[i][1]);
          } else if (!skip_form(unit, formats[i][1], dwarf64)) {
            return false;
          }
        }
        if (!unit.ok)
          return false;
        if (table == 0)
          dirs.push_back(path);
        else
          files.push_back({path, dir});
      }
    }
  } else {
    dirs.push_back(""); // directory 0: the compilation directory, not recorded here
    for (;;) {
      const char* d = unit.str();
      if (!unit.ok)
        return false;
      if (*d == '\0')
        break;
      dirs.push_back(d);
    }
    files.push_back({"", 0}); // file indices are 1-based before DWARF 5
    for (;;) {
      const char* name = unit.str();
      if (!unit.ok)
        return false;
      if (*name == '\0')
        break;
      const std::uint64_t dir = unit.uleb();
      unit.uleb(); // modification time
      unit.uleb(); // length
      files.push_back({name, dir});
    }
  }
  if (!unit.ok)
    return false;

  // The state machine ([DWARF5] 6.2.2). A row (address, file, line) covers the addresses up to
  // the next row of the same sequence.
  reader r{program, unit.end};
  std::uint64_t address = 0, file = 1, line = 1;
  bool have_prev = false;
  std::uint64_t prev_address = 0, prev_file = 0, prev_line = 0;
  const auto emit = [&]() -> bool {
    if (have_prev && prev_address <= addr && addr < address) {
      if (prev_file < files.size()) {
        const file_entry& fe = files[prev_file];
        res.file = join_path(fe.dir < dirs.size() ? dirs[fe.dir] : nullptr, fe.name);
      }
      res.line = static_cast<std::uint32_t>(prev_line);
      res.found = true;
      return true;
    }
    have_prev = true;
    prev_address = address;
    prev_file = file;
    prev_line = line;
    return false;
  };
  while (r.ok && r.p < r.end) {
    const unsigned op = static_cast<unsigned>(r.u(1));
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
      const std::uint64_t len = r.uleb();
      if (!r.ok || len == 0 || len > static_cast<std::uint64_t>(r.end - r.p))
        return false;
      const unsigned char* next = r.p + len;
      const unsigned sub = static_cast<unsigned>(r.u(1));
      if (sub == 1) { // DW_LNE_end_sequence
        if (emit())
          return true;
        address = 0;
        file = 1;
        line = 1;
        have_prev = false;
      } else if (sub == 2) { // DW_LNE_set_address
        address = r.u(static_cast<std::size_t>(len - 1 <= 8 ? len - 1 : address_size));
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
    case 4: file = r.uleb(); break;                            // DW_LNS_set_file
    case 5: r.uleb(); break;                                   // DW_LNS_set_column
    case 6: case 7: case 10: case 11: break;                   // flags
    case 8: address += std::uint64_t((255 - opcode_base) / line_range) * min_inst; break; // const_add_pc
    case 9: address += r.u(2); break;                          // DW_LNS_fixed_advance_pc
    case 12: r.uleb(); break;                                  // DW_LNS_set_isa
    default:
      for (unsigned i = 0; i < std_lengths[op]; ++i)
        r.uleb();
      break;
    }
  }
  return false;
}

line_result find_line(const object_file& f, std::uint64_t addr) {
  line_result res;
  if (!f.line.usable())
    return res;
  reader all{f.line.data, f.line.data + f.line.size};
  while (all.ok && all.p < all.end) {
    std::uint64_t len = all.u(4);
    bool dwarf64 = false;
    if (len == 0xffffffffu) {
      dwarf64 = true;
      len = all.u(8);
    }
    if (!all.ok || len > static_cast<std::uint64_t>(all.end - all.p))
      break;
    reader unit{all.p, all.p + len};
    all.p += len;
    if (run_line_program(f, unit, dwarf64, addr, res))
      return res;
  }
  return res;
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
  const unsigned char* attrs = nullptr; // (attribute, form [, implicit const]) pairs
};

// The unit header at p (in .debug_info); false if it is not one this reader handles.
bool read_unit_header(const object_file& f, const unsigned char* p, dwarf_unit& u) {
  reader r{p, f.info.data + f.info.size};
  std::uint64_t len = r.u(4);
  u.dwarf64 = len == 0xffffffffu;
  if (u.dwarf64)
    len = r.u(8);
  if (!r.ok || len > static_cast<std::uint64_t>(r.end - r.p))
    return false;
  u.begin = p;
  u.end = r.p + len;
  r.end = u.end;
  u.version = static_cast<unsigned>(r.u(2));
  if (u.version < 2 || u.version > 5)
    return false;
  if (u.version >= 5) {
    const unsigned type = static_cast<unsigned>(r.u(1));
    u.addr_size = static_cast<unsigned>(r.u(1));
    u.abbrev_offset = r.u(u.dwarf64 ? 8 : 4);
    if (type == 4 || type == 5) // skeleton, split compile: dwo id
      r.u(8);
    else if (type == 2 || type == 6) // type units
      return false;
  } else {
    u.abbrev_offset = r.u(u.dwarf64 ? 8 : 4);
    u.addr_size = static_cast<unsigned>(r.u(1));
  }
  u.dies = r.p;
  return r.ok && (u.addr_size == 4 || u.addr_size == 8);
}

// The abbreviation table of a unit, indexed by code.
bool read_abbrevs(const object_file& f, std::uint64_t offset, std::vector<abbrev_entry>& out) {
  if (!f.abbrev.usable() || offset >= f.abbrev.size)
    return false;
  reader r{f.abbrev.data + offset, f.abbrev.data + f.abbrev.size};
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
    e.children = r.u(1) != 0;
    e.attrs = r.p;
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
  std::uint64_t u = 0;
  const char* str = nullptr;
};

// Reads one attribute value of the given form; false for an unknown form.
bool read_attr(reader& r, std::uint64_t form, std::int64_t implicit, const dwarf_unit& u, attr_value& v) {
  v.form = form;
  v.str = nullptr;
  const std::size_t off = u.dwarf64 ? 8 : 4;
  switch (form) {
  case 0x01: v.u = r.u(u.addr_size); break;                 // addr
  case 0x03: r.skip(static_cast<std::size_t>(r.u(2))); break; // block2
  case 0x04: r.skip(static_cast<std::size_t>(r.u(4))); break; // block4
  case 0x05: v.u = r.u(2); break;                            // data2
  case 0x06: v.u = r.u(4); break;                            // data4
  case 0x07: v.u = r.u(8); break;                            // data8
  case 0x08: v.str = r.str(); break;                         // string
  case 0x09: case 0x18: r.skip(static_cast<std::size_t>(r.uleb())); break; // block, exprloc
  case 0x0a: r.skip(static_cast<std::size_t>(r.u(1))); break; // block1
  case 0x0b: v.u = r.u(1); break;                            // data1
  case 0x0c: v.u = r.u(1); break;                            // flag
  case 0x0d: v.u = static_cast<std::uint64_t>(r.sleb()); break; // sdata
  case 0x0e: case 0x1f: case 0x17: v.u = r.u(off); break;    // strp, line_strp, sec_offset
  case 0x0f: v.u = r.uleb(); break;                          // udata
  case 0x10: v.u = r.u(u.version <= 2 ? u.addr_size : off); break; // ref_addr
  case 0x11: v.u = r.u(1); break;                            // ref1
  case 0x12: v.u = r.u(2); break;                            // ref2
  case 0x13: v.u = r.u(4); break;                            // ref4
  case 0x14: v.u = r.u(8); break;                            // ref8
  case 0x15: v.u = r.uleb(); break;                          // ref_udata
  case 0x16: {                                               // indirect
    const std::uint64_t real = r.uleb();
    return real != 0x16 && read_attr(r, real, implicit, u, v);
  }
  case 0x19: v.u = 1; break;                                 // flag_present
  case 0x1a: case 0x1b: case 0x22: case 0x23: v.u = r.uleb(); break; // strx, addrx, loclistx, rnglistx
  case 0x1c: v.u = r.u(4); break;                            // ref_sup4
  case 0x1d: v.u = r.u(off); break;                          // strp_sup
  case 0x1e: r.skip(16); break;                              // data16
  case 0x20: case 0x24: v.u = r.u(8); break;                 // ref_sig8, ref_sup8
  case 0x21: v.u = static_cast<std::uint64_t>(implicit); break; // implicit_const
  case 0x25: case 0x29: v.u = r.u(1); break;                 // strx1, addrx1
  case 0x26: case 0x2a: v.u = r.u(2); break;                 // strx2, addrx2
  case 0x27: case 0x2b: v.u = r.u(3); break;                 // strx3, addrx3
  case 0x28: case 0x2c: v.u = r.u(4); break;                 // strx4, addrx4
  case 0x1f01: case 0x1f02: v.u = r.uleb(); break;           // GNU_addr_index, GNU_str_index
  case 0x1f20: case 0x1f21: v.u = r.u(off); break;           // GNU_ref_alt, GNU_strp_alt
  default: return false;
  }
  return r.ok;
}

bool is_strx(std::uint64_t form) { return form == 0x1a || (form >= 0x25 && form <= 0x28) || form == 0x1f02; }
bool is_addrx(std::uint64_t form) { return form == 0x1b || (form >= 0x29 && form <= 0x2c) || form == 0x1f01; }

const char* section_string(const section& s, std::uint64_t off) {
  if (!s.usable() || off >= s.size || std::memchr(s.data + off, 0, s.size - off) == nullptr)
    return nullptr;
  return reinterpret_cast<const char*>(s.data + off);
}

// The string an attribute value denotes, if any.
const char* attr_string(const object_file& f, const dwarf_unit& u, const attr_value& v) {
  if (v.str)
    return v.str;
  if (v.form == 0x0e)
    return section_string(f.str, v.u);
  if (v.form == 0x1f)
    return section_string(f.line_str, v.u);
  if (is_strx(v.form) && u.has_str_offsets_base) {
    const std::size_t w = u.dwarf64 ? 8 : 4;
    const std::uint64_t at = u.str_offsets_base + v.u * w;
    if (!f.str_offsets.usable() || at + w > f.str_offsets.size)
      return nullptr;
    reader r{f.str_offsets.data + at, f.str_offsets.data + f.str_offsets.size};
    return section_string(f.str, r.u(w));
  }
  return nullptr;
}

// The address an attribute value denotes (addr or addrx forms).
bool attr_address(const object_file& f, const dwarf_unit& u, const attr_value& v, std::uint64_t& out) {
  if (v.form == 0x01) {
    out = v.u;
    return true;
  }
  if (is_addrx(v.form)) {
    const std::uint64_t at = u.addr_base + v.u * u.addr_size;
    if (!f.addr.usable() || at + u.addr_size > f.addr.size)
      return false;
    reader r{f.addr.data + at, f.addr.data + f.addr.size};
    out = r.u(u.addr_size);
    return r.ok;
  }
  return false;
}

// The attributes of an entry this reader uses.
struct die_info {
  std::uint64_t tag = 0;
  bool children = false;
  attr_value low, high, ranges, name, linkage, origin, spec;
  bool has_low = false, has_high = false, has_ranges = false, has_name = false, has_linkage = false,
       has_origin = false, has_spec = false;
  attr_value str_offsets_base, addr_base, rnglists_base;
  bool has_str_offsets_base = false, has_addr_base = false, has_rnglists_base = false;
};

// Reads the entry at r (its code already known to be abbrev a); false on malformed data.
bool read_die(reader& r, const abbrev_entry& a, const dwarf_unit& u, die_info& d) {
  d = die_info();
  d.tag = a.tag;
  d.children = a.children;
  reader spec{a.attrs, r.end};
  for (;;) {
    const std::uint64_t at = spec.uleb(), form = spec.uleb();
    std::int64_t implicit = 0;
    if (form == 0x21)
      implicit = spec.sleb();
    if (!spec.ok)
      return false;
    if (at == 0 && form == 0)
      return true;
    attr_value v;
    if (!read_attr(r, form, implicit, u, v))
      return false;
    switch (at) {
    case 0x11: d.low = v; d.has_low = true; break;
    case 0x12: d.high = v; d.has_high = true; break;
    case 0x55: d.ranges = v; d.has_ranges = true; break;
    case 0x03: d.name = v; d.has_name = true; break;
    case 0x6e: case 0x2007: d.linkage = v; d.has_linkage = true; break;
    case 0x31: d.origin = v; d.has_origin = true; break;
    case 0x47: d.spec = v; d.has_spec = true; break;
    case 0x72: d.str_offsets_base = v; d.has_str_offsets_base = true; break;
    case 0x73: d.addr_base = v; d.has_addr_base = true; break;
    case 0x74: d.rnglists_base = v; d.has_rnglists_base = true; break;
    default: break;
    }
  }
}

// Whether the address ranges of an entry (low_pc/high_pc or DW_AT_ranges) contain addr.
bool die_contains(const object_file& f, const dwarf_unit& u, const die_info& d, std::uint64_t addr) {
  if (d.has_low && d.has_high) {
    std::uint64_t low;
    if (!attr_address(f, u, d.low, low))
      return false;
    std::uint64_t high;
    if (d.high.form == 0x01 || is_addrx(d.high.form)) {
      if (!attr_address(f, u, d.high, high))
        return false;
    } else {
      high = low + d.high.u;
    }
    return low <= addr && addr < high;
  }
  if (!d.has_ranges)
    return false;
  if (u.version < 5) { // .debug_ranges: (begin, end) pairs relative to the base address
    if (!f.ranges.usable() || d.ranges.u >= f.ranges.size)
      return false;
    reader r{f.ranges.data + d.ranges.u, f.ranges.data + f.ranges.size};
    const std::uint64_t all_ones = u.addr_size == 8 ? ~std::uint64_t(0) : 0xffffffffu;
    std::uint64_t base = u.base_address;
    while (r.ok) {
      const std::uint64_t b = r.u(u.addr_size), e = r.u(u.addr_size);
      if (!r.ok || (b == 0 && e == 0))
        return false;
      if (b == all_ones) {
        base = e;
        continue;
      }
      if (base + b <= addr && addr < base + e)
        return true;
    }
    return false;
  }
  // .debug_rnglists
  std::uint64_t off = d.ranges.u;
  if (d.ranges.form == 0x23) { // rnglistx: an index into the offsets after the header
    if (!u.has_rnglists_base)
      return false;
    const std::size_t w = u.dwarf64 ? 8 : 4;
    const std::uint64_t at = u.rnglists_base + off * w;
    if (!f.rnglists.usable() || at + w > f.rnglists.size)
      return false;
    reader r{f.rnglists.data + at, f.rnglists.data + f.rnglists.size};
    off = u.rnglists_base + r.u(w);
  }
  if (!f.rnglists.usable() || off >= f.rnglists.size)
    return false;
  reader r{f.rnglists.data + off, f.rnglists.data + f.rnglists.size};
  std::uint64_t base = u.base_address;
  const auto addrx = [&](std::uint64_t idx, std::uint64_t& out) {
    attr_value v;
    v.form = 0x1b;
    v.u = idx;
    return attr_address(f, u, v, out);
  };
  while (r.ok) {
    const unsigned kind = static_cast<unsigned>(r.u(1));
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
    case 5: base = r.u(u.addr_size); continue; // base_address
    case 6: // start_end
      b = r.u(u.addr_size);
      e = r.u(u.addr_size);
      break;
    case 7: // start_length
      b = r.u(u.addr_size);
      e = b + r.uleb();
      break;
    default: return false;
    }
    if (r.ok && b <= addr && addr < e)
      return true;
  }
  return false;
}

// Sets the unit's bases from its unit entry.
void apply_unit_die(const object_file& f, dwarf_unit& u, const die_info& d) {
  if (d.has_str_offsets_base) {
    u.str_offsets_base = d.str_offsets_base.u;
    u.has_str_offsets_base = true;
  } else if (u.version >= 5 && f.str_offsets.usable()) { // the first contribution's header
    u.str_offsets_base = u.dwarf64 ? 16 : 8;
    u.has_str_offsets_base = true;
  }
  if (d.has_addr_base)
    u.addr_base = d.addr_base.u;
  if (d.has_rnglists_base) {
    u.rnglists_base = d.rnglists_base.u;
    u.has_rnglists_base = true;
  }
  if (d.has_low)
    attr_address(f, u, d.low, u.base_address);
}

// The unit containing the .debug_info offset off, its bases set; false if none.
bool unit_at(const object_file& f, std::uint64_t off, dwarf_unit& u, std::vector<abbrev_entry>& abbrevs) {
  const unsigned char* p = f.info.data;
  const unsigned char* end = f.info.data + f.info.size;
  while (p < end) {
    if (!read_unit_header(f, p, u)) {
      // Skip a unit of a kind this reader does not handle.
      reader r{p, end};
      std::uint64_t len = r.u(4);
      if (len == 0xffffffffu)
        len = r.u(8);
      if (!r.ok || len > static_cast<std::uint64_t>(end - r.p))
        return false;
      p = r.p + len;
      continue;
    }
    if (f.info.data + off < u.end) {
      if (f.info.data + off < u.dies || !read_abbrevs(f, u.abbrev_offset, abbrevs))
        return false;
      reader r{u.dies, u.end};
      const std::uint64_t code = r.uleb();
      die_info d;
      if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].attrs == nullptr ||
          !read_die(r, abbrevs[code], u, d))
        return false;
      apply_unit_die(f, u, d);
      return true;
    }
    p = u.end;
  }
  return false;
}

// The scopes enclosing the entry at .debug_info offset off in the unit u ("ns::C::"): the
// named namespaces, classes, structures, unions and functions among its ancestors.
std::string die_scope(const object_file& f, const dwarf_unit& u, const std::vector<abbrev_entry>& abbrevs,
                      std::uint64_t off) {
  struct level {
    std::uint64_t tag;
    const char* name;
  };
  std::vector<level> stack;
  const unsigned char* target = f.info.data + off;
  reader r{u.dies, u.end};
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
    if (code >= abbrevs.size() || abbrevs[code].attrs == nullptr || !read_die(r, abbrevs[code], u, d))
      break;
    if (d.children)
      stack.push_back({d.tag, d.has_name ? attr_string(f, u, d.name) : nullptr});
  }
  std::string scope;
  for (const level& l : stack) {
    const bool named_scope = l.tag == 0x39 || l.tag == 0x02 || l.tag == 0x13 || l.tag == 0x17 || l.tag == 0x2e;
    if (!named_scope)
      continue;
    if (l.name != nullptr)
      scope += l.name;
    else if (l.tag == 0x39)
      scope += "(anonymous namespace)";
    else
      continue;
    scope += "::";
  }
  return scope;
}

// The name of the function the entry at .debug_info offset off describes.
bool die_name(const object_file& f, std::uint64_t off, int depth, std::string& name) {
  if (depth > 8)
    return false;
  dwarf_unit u;
  std::vector<abbrev_entry> abbrevs;
  if (!unit_at(f, off, u, abbrevs))
    return false;
  reader r{f.info.data + off, u.end};
  const std::uint64_t code = r.uleb();
  die_info d;
  if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].attrs == nullptr || !read_die(r, abbrevs[code], u, d))
    return false;
  if (d.has_linkage)
    if (const char* s = attr_string(f, u, d.linkage)) {
      name = s;
      return true;
    }
  const auto ref_target = [&](const attr_value& v, std::uint64_t& target) {
    if (v.form == 0x10) { // ref_addr: an offset in .debug_info
      target = v.u;
      return true;
    }
    if (v.form >= 0x11 && v.form <= 0x15) { // relative to the unit
      target = static_cast<std::uint64_t>(u.begin - f.info.data) + v.u;
      return true;
    }
    return false;
  };
  std::uint64_t target;
  if (d.has_origin && ref_target(d.origin, target) && die_name(f, target, depth + 1, name))
    return true;
  if (d.has_spec && ref_target(d.spec, target) && die_name(f, target, depth + 1, name))
    return true;
  if (d.has_name)
    if (const char* s = attr_string(f, u, d.name)) {
      name = die_scope(f, u, abbrevs, off) + s; // no linkage name: qualify it by its scopes
      return true;
    }
  return false;
}

// The name of the innermost function (inlined or not) containing addr, from .debug_info.
bool find_function(const object_file& f, std::uint64_t addr, std::string& name) {
  if (!f.info.usable() || !f.abbrev.usable())
    return false;
  const unsigned char* p = f.info.data;
  const unsigned char* end = f.info.data + f.info.size;
  std::vector<abbrev_entry> abbrevs;
  while (p < end) {
    dwarf_unit u;
    if (!read_unit_header(f, p, u)) {
      reader r{p, end};
      std::uint64_t len = r.u(4);
      if (len == 0xffffffffu)
        len = r.u(8);
      if (!r.ok || len > static_cast<std::uint64_t>(end - r.p))
        return false;
      p = r.p + len;
      continue;
    }
    p = u.end;
    if (!read_abbrevs(f, u.abbrev_offset, abbrevs))
      continue;
    reader r{u.dies, u.end};
    // The unit entry: its bases, and whether the unit covers addr at all.
    std::uint64_t code = r.uleb();
    die_info d;
    if (!r.ok || code == 0 || code >= abbrevs.size() || abbrevs[code].attrs == nullptr || !read_die(r, abbrevs[code], u, d))
      continue;
    apply_unit_die(f, u, d);
    if ((d.has_low && d.has_high) || d.has_ranges) {
      if (!die_contains(f, u, d, addr))
        continue;
    }
    if (!d.children)
      continue;
    int depth = 1, best_depth = -1;
    const unsigned char* best = nullptr;
    while (r.ok && r.p < r.end && depth > 0) {
      const unsigned char* at = r.p;
      code = r.uleb();
      if (!r.ok)
        break;
      if (code == 0) {
        --depth;
        continue;
      }
      if (code >= abbrevs.size() || abbrevs[code].attrs == nullptr || !read_die(r, abbrevs[code], u, d))
        break;
      if ((d.tag == 0x2e || d.tag == 0x1d) && depth > best_depth && die_contains(f, u, d, addr)) {
        best = at;
        best_depth = depth;
      }
      if (d.children)
        ++depth;
    }
    if (best != nullptr)
      return die_name(f, static_cast<std::uint64_t>(best - f.info.data), 0, name);
  }
  return false;
}

// ---- the object cache -------------------------------------------------------------------------------

constexpr int cache_size = 8;
std::mutex cache_mutex;
object_file cache[cache_size];
unsigned long long cache_clock = 0;

// Copies a NUL-terminated path into a fixed buffer, truncating it (the buffer ends in NUL).
template <std::size_t N>
void copy_path(char (&dst)[N], const char* src) noexcept {
  std::size_t n = 0;
  for (; n + 1 < N && src[n] != '\0'; ++n)
    dst[n] = src[n];
  dst[n] = '\0';
}

// The cached, mapped object named path (loading it, evicting the least recently used one); null
// if it cannot be read. Called with cache_mutex held.
object_file* object_named(const char* path) {
  object_file* victim = &cache[0];
  for (object_file& f : cache) {
    if (f.map != nullptr && std::strcmp(f.path, path) == 0) {
      f.last_use = ++cache_clock;
      return &f;
    }
    if (f.map == nullptr || (victim->map != nullptr && f.last_use < victim->last_use))
      victim = &f;
  }
  if (victim->map != nullptr)
    ycxx_pal_unmap_file(victim->map, victim->map_size);
  *victim = object_file();
  if (ycxx_pal_map_file(path, &victim->map, &victim->map_size) != 0) {
    victim->map = nullptr;
    return nullptr;
  }
  copy_path(victim->path, path);
  if (!parse_elf(*victim)) {
    // Nothing usable; the mapping stays cached as a negative answer.
    const ycxx_pal_size size = victim->map_size;
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
  std::string function, file;
  std::uint32_t line = 0;
};

symbolized symbolize(std::uintptr_t pc, bool want_function, bool want_line) {
  symbolized out;
  char path[4096];
  ycxx_pal_handle bias = 0;
  const char* raw_name = nullptr;
  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (ycxx_pal_object_of(pc, path, sizeof path, &bias) == 0) {
      if (object_file* f = object_named(path)) {
        const std::uint64_t addr = pc - bias;
        if (want_function && !find_function(*f, addr, out.function)) {
          raw_name = find_symbol(*f, f->symtab, f->symstr, addr);
          if (raw_name == nullptr)
            raw_name = find_symbol(*f, f->dynsym, f->dynstr, addr);
          if (raw_name != nullptr)
            out.function = raw_name;
        }
        if (want_line) {
          line_result lr = find_line(*f, addr);
          if (lr.found) {
            out.file = static_cast<std::string&&>(lr.file);
            out.line = lr.line;
          }
        }
      }
    }
  }
  if (want_function && out.function.empty()) {
    ycxx_pal_handle start;
    if (ycxx_pal_dynamic_symbol(pc, &raw_name, &start) == 0 && raw_name != nullptr)
      out.function = raw_name;
  }
  if (!out.function.empty()) {
    std::string d;
    if (ycxx::detail::demangle(out.function.c_str(), d))
      out.function = static_cast<std::string&&>(d);
  }
  return out;
}

} // namespace

std::size_t ycxx::detail::stacktrace_capture(const void* ra, std::size_t skip, std::uintptr_t* buf,
                                             std::size_t n) noexcept {
  if (n == 0)
    return 0;
  capture_state st{reinterpret_cast<std::uintptr_t>(ra), false, skip, buf, n, 0};
  _Unwind_Backtrace(capture_frame, &st);
  return st.count;
}

std::string ycxx::detail::stacktrace_describe(std::uintptr_t pc, stacktrace_query what) {
  if (what == stacktrace_query::description)
    return symbolize(pc, true, false).function;
  return symbolize(pc, false, true).file;
}

std::uint_least32_t ycxx::detail::stacktrace_line(std::uintptr_t pc) { return symbolize(pc, false, true).line; }

std::string std::to_string(const stacktrace_entry& f) {
  if (!f)
    return std::string();
  symbolized s = symbolize(f.native_handle(), true, true);
  std::string out;
  if (!s.function.empty()) {
    out = static_cast<std::string&&>(s.function);
  } else { // no symbol: the address
    char hex[2 * sizeof(std::uintptr_t)];
    char* p = hex + sizeof hex;
    std::uintptr_t v = f.native_handle();
    do {
      *--p = "0123456789abcdef"[v & 0xf];
      v >>= 4;
    } while (v != 0);
    out = "0x";
    out.append(p, hex + sizeof hex);
  }
  if (!s.file.empty()) {
    out += " at ";
    out += s.file;
    if (s.line != 0) {
      out += ':';
      out += std::to_string(s.line);
    }
  }
  return out;
}

std::ostream& std::operator<<(std::ostream& os, const stacktrace_entry& f) { return os << std::to_string(f); }
