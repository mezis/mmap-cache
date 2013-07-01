#include <sys/syslimits.h> // provides PATH_MAX

#define MMAP_CACHE_BYTES_MAX (1024*1024 - 5)
#define MMAP_CACHE_KEY_MAX   1024

typedef struct mmap_cache_ mmap_cache_t;

struct cache_entry_
{
  char*   key;
  void*   value;
  int     bytes;
};

typedef struct cache_entry_ cache_entry_t;

// Open (and possibly create) a shared memory cache.
// 
// <path> - the cache basename, without any extension (.meta and .data
// <files will be used).
// 
// <pages> - the number of 1MB pages in the cache. If the cache already exists,
// the value 0 may be specified; it will result in and error if the cache does 
// not exist yet.
// 
// Return 0 on success, non-zero and sets errno on error.
// EINVAL:  <pages> too large (max. 2**24).
// EPROTO:  the cache is in an inconsistent state
// ENOTSUP: the cache was created with a different version
int mmap_cache_open(mmap_cache_t** cache, char* path, int pages);

// Close a previously opened cache.
int mmap_cache_close(mmap_cache_t* cache);

// Read an entry from the cache.
// Return 0 on success, non-zero and sets errno on error.
// EINVAL: key too large.
int mmap_cache_get(mmap_cache_t* cache, cache_entry_t* entry);

// Write an entry to the cache.
// Return 0 on success, non-zero and sets errno on error.
// EINVAL: key too large
// EOVERFLOW: key+bytes too large.
int mmap_cache_put(mmap_cache_t* cache, cache_entry_t* entry);

