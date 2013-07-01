// 
// lock.h --
// 
// Abstraction over a platform cross-process shared/exclusive lock mechanism.
// 
#include "mmap-cache.h"

// Acquire a share lock on this <cache>
int lock_acquire_read(mmap_cache_t* cache);

// Acquire an exclusive lock on this <cache>
int lock_acquire_write(mmap_cache_t* cache);

// Release any acquired lock.
int lock_release(mmap_cache_t* cache);
