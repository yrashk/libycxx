/* The few parts of the Limine boot protocol this kernel uses, written from the protocol's
 * specification (PROTOCOL.md of github.com/limine-bootloader/limine-protocol): the base revision
 * tag, the request delimiters, and the HHDM and memory map features. A kernel that needs more
 * would use the protocol's own limine.h. */
#ifndef HOSTED_LAYERS_LIMINE_PROTOCOL_H
#define HOSTED_LAYERS_LIMINE_PROTOCOL_H

#include <stdint.h>

/* "Base Revisions": the bootloader sets the third word to 0 when it supports the revision asked
   for, and the second to the revision it actually used (base revision 3 and later). */
#define LIMINE_BASE_REVISION_TAG(n) {0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (n)}

/* "Requests Delimiters". */
#define LIMINE_REQUESTS_START_MARKER {0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, 0x785c6ed015d3e316, 0x181e920a7852b9d9}
#define LIMINE_REQUESTS_END_MARKER {0xadc0e0531bb10d03, 0x9572709f31764c62}

/* "Features": every request starts with its 4-word id (the first two words are common), a
   revision and the response pointer the bootloader fills in. */
#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b

/* "HHDM (Higher Half Direct Map) Feature". */
#define LIMINE_HHDM_REQUEST_ID {LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b}
struct limine_hhdm_response {
  uint64_t revision;
  uint64_t offset;
};
struct limine_hhdm_request {
  uint64_t id[4];
  uint64_t revision;
  struct limine_hhdm_response* response;
};

/* "Memory Map Feature". */
#define LIMINE_MEMMAP_REQUEST_ID {LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62}
#define LIMINE_MEMMAP_USABLE 0
struct limine_memmap_entry {
  uint64_t base;
  uint64_t length;
  uint64_t type;
};
struct limine_memmap_response {
  uint64_t revision;
  uint64_t entry_count;
  struct limine_memmap_entry** entries;
};
struct limine_memmap_request {
  uint64_t id[4];
  uint64_t revision;
  struct limine_memmap_response* response;
};

#endif
