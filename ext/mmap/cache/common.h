//
// common.h --
//
// General definitions.
//
#include <stdint.h>
#include <assert.h>
#include "helpers.h"

/*

Data Layout

2 files per mapped cache:
- "<name>.meta" is the metadata (variable size, contains hash table and page usage info),
  size is typically ~32 bytes per entry plus about 150kB overhead.
- "<name>.data" is the payload (fixed size, contains data pages)

The metadata file:

- cache_info_t    (64 bytes)
- page_info_t[]   (8 bytes * <page_count>, preallocated and fixed)
- hash_bucket_t[] (36 bytes * 2 ** <hash_table_size>, preallocated + locking expansion mechanism)
- hash_extent_t[] (136 bytes * 1024 initially, freelist-managed)

The payload file:

A flat array of 1MB pages.
Each page is split into chunks, allocated either through a bitmap (large chunks)
or a freelist (small chunks). Note that each (used) chunk contains the cache key
and the cache value (without delimitation).

Pages types:
  type  0  -> 65535 x 16 byte chunks      FL
  type  1  -> 32768 x 32      chunks      FL
  type  2  -> 16384 x 64      chunks      FL
  type  3  ->  8192 x 128     chunks  EXT FL
  type  4  ->  4096 x 256     chunks  EXT FL
  type  5  ->  2048 x 512     chunks  EXT FL
  type  6  ->  1024 x 1K      chunks  EXT FL
  type  7  ->   512 x 2K      chunks  EXT FL
  type  8  ->   256 x 4K      chunks  EXT FL
  type  9  ->   128 x 8K      chunks  EXT FL
  type  10 ->    64 x 16K     chunks  EXT FL
  type  11 ->    32 x 32K     chunks  EXT FL
  type  12 ->    16 x 64K     chunks  EXT
  type  13 ->     8 x 128K    chunks  EXT
  type  14 ->     4 x 256K    chunks  EXT
  type  15 ->     2 x 512K    chunks  EXT
  type  16 ->     1 x 1MB     chunks  EXT


Notes:

- Page of a type marked FL have their last 2 bytes (16 bits) reserved for free
  list management.
- Pages marked with EXT have the last 5 bytes (40 bits), possibly the bytes
  just before the free list footer, reserved for future extension (linked list
  of payload segments for memory efficiency / higher payload sizes)
- The last entry in a type 0 page is reserved (lost to free list limitation, 
  offset 65535 is used for the sentinel value in free lists)
- Keep load factor below 0.8
- Keep load factor plus stddev below 0.9


Storage, performance:

It is recommended to use a power-of-two number of pages greater than or
equal to 8, to make sure the hash table is properly aligned in memory.

For 128MB data pages and 64k non-pathological entries (2kB average), assuming a load of 1,
the metadata file would be typically
64 + 128*8 + 32*(2**16) + 128*(2**10) = 2.1 MB (1.7% overhead).

For 128MB data pages and 1M non-pathological entries (128B average), assuming a load of 1,
the metadata file would be typically
64 + 128*8 + 32*(2**20) + 128*(2**10) = 32 MB (25% overhead).

For 128MB data pages and the pathological maximum of 8M 14-byte entries, still assuming a load of 1,
the metadata file would be typically
64 + 128*8 + 32*(2**23) + 128*(2**10) = 256 MB (200% overhead).

Compare with Memcached's 60+ bytes per entry (http://stackoverflow.com/questions/8129068/memcached-item-overhead)

*/




// 256 byte header of metadata file (4 cache lines)
struct PACKED_STRUCT cache_info_
{
    // should be 'µµch' (0xB5 0xB5 0x63 0x68)
    uint8_t       magic[4];
    // 0x00 -> little endian
    // 0xff -> big endian
    uint8_t       big_endian;
    // should be 0x01
    uint8_t       version;
    // 2 byte padding (all bits set)
    unsigned int  __r0: 16;
    
    // total number of 1MB pages, 1 -> (2^24 -1) ie. max 17TB memory
    unsigned int  page_count: 24;
    // 5 byte padding (all bits set)
    unsigned int  __r1: 40;

    // used storage i.e. sum of used chunk size (for reporting)
    uint64_t      bytes_used;
    // wasted chunk storage
    uint64_t      bytes_wasted;

    // origin of expiration times (in seconds since epoch, UTC)
    uint32_t      time_origin;

    // order of hash table, minimum 10 -> 1024 buckets
    uint8_t       hash_table_size;
    // 3 byte padding (all bits set)
    unsigned int  __r2: 24;
    // number of hash table extents, initially 1024
    uint32_t      hash_extents_count;
    // first free extent
    uint32_t      hash_extents_head;
    // index of oldest entry in hash table (head of double linked list)
    unsigned int  hash_oldest: 40;
    // 3 byte padding (all bits set)
    unsigned int  __r3: 24;
    // index of newest entry in hash table (tail of double linked list)
    unsigned int  hash_oldest: 40;
    // 3 byte padding (all bits set)
    unsigned int  __r4: 24;

    // used hash table entries (to determine load)
    uint64_t      entries_used;
    // sum squares used entries per bucket (to determine load variance)
    uint64_t      entries_squared;

    // padding, reserved for future extra metadata (all bits set)
    uint8_t       __r5[176];
};

typedef struct cache_info_ cache_info_t;


// 8 byte per-page metadata (1/8 cache line)
struct PACKED_STRUCT page_info_
{
  // 0-16:   the page is used to store 1 << (N + 4) byte chunks
  // 17-254: invalid
  // 255:    the page is unused
  uint8_t       type;
  // number of unused chunks in page
  uint16_t      free_chunks;
  // padding (all bits set)
  uint8_t       __r1;

  union {
    // offset of first free item (head of free list, up to 32kB pages)
    uint16_t    head_slot;
    // bitmap of free items (from 64kB pages up), unused bits 0
    // least significant bit refers to first chunk in page
    uint16_t    bitmap;
  };
};

typedef struct page_info_ page_info_t;

/*

  Hash table,
  Maps hashes to entries.

  The table is stored as an array of <hash_bucket_t> of size 2 **
  <hash_table_size> (the main entries), followed by and array of
  <hash_extent_t> of size <hash_extents_count> (the extents).

  The extents array is managed by the free list allocator.

  The hash table doubles as a double-linked-list to maintain the LRU order of
  entries; this is necessary for LRU eviction. This is achieved through the
  <older_entry> and <newer_entry> indices. If an <index> is lower than 2 **
  <hash_table_size> it points to a entry in a bucket. If higher, it points to
  an entry in an extent.

  Given <index_e> = <index> - 2 ** <hash_table_size>, the lower 2 bits of
  <index_e> are the position in the extent and the higher bits to the extent
  itself.

  Of course <index_e> >> 2 must be lower than <hash_extents_count>.

*/



// 26 bytes per entry
struct PACKED_STRUCT hash_entry_
{
  // hash of cache key (2**32-1 if entry unused)
  unsigned int hash: 32;
  // index of page containing payload
  unsigned int page: 24;
  // index of chunk in page
  unsigned int chunk: 16;
  // size of key
  unsigned int keysize: 10;
  // bytes stored (maximum is 1<<20 - 8), excluding key
  unsigned int bytes: 20;
  // index of the entry older than this one (2**56-1 if oldest)
  unsigned int older_entry: 40;
  // index of the entry newer than this one (2**56-1 if newest)
  unsigned int newer_entry: 40;
  // expiry (seconds from time origin, 26 bits ~ 2 years, all bits set to not expire)
  unsigned int expiry: 26;
};

typedef struct hash_entry_ hash_entry_t


// 32 bytes per bucket (1/2 cache line)
struct PACKED_STRUCT hash_bucket_
{
  hash_entry_t entry;
  // padding (all bits set)
  uint16_t     __r1;
  // index of extent, 2**32-1 if no extent
  uint32_t     extent;
};

typedef struct hash_bucket_ hash_bucket_t;


// 128 bytes per extent (2 cache lines)
struct PACKED_STRUCT hash_extent_
{
  hash_entry_t entries[4];
  // padding (all bits set)
  uint8_t      __r1[20];
  // used by the free list allocator
  uint32_t     __r2;
};

typedef struct hash_extent_ hash_extent_t;

