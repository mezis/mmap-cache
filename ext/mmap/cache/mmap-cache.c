#include <sys/mman.h>

#include "mmap-cache.h"
#include "common.h"
#include "lock.h"

struct mmap_cache_
{
  int  fd_meta;
  int  fd_data;
  char path_meta[PATH_MAX];
  char path_data[PATH_MAX];

  void* map_meta;
  void* map_data;

  cache_info_t*  cache_info;
  page_info_t*   page_infos;
  hash_bucket_t* hash_table;
  hash_extent_t* hash_extents;

  free_list_t    hash_extents_list;
};


////////////////////////////////////////////////////////////////////////////////
static
void _validate_structure_sizes()
{
  assert(sizeof(cache_info_t)  == 256);
  assert(sizeof(page_info_t)   ==   8);
  assert(sizeof(hash_entry_t)  ==  26);
  assert(sizeof(hash_bucket_t) ==  32);
  assert(sizeof(hash_extent_t) == 128);
}

////////////////////////////////////////////////////////////////////////////////

int mmap_cache_open(mmap_cache_t** cache, char* path, int pages)
{
  int res = 0;
  _validate_structure_sizes();


cleanup:
  return res;
}


////////////////////////////////////////////////////////////////////////////////

int mmap_cache_close(mmap_cache_t* cache)
{
  int res = 0;


cleanup:
  return res;
}


////////////////////////////////////////////////////////////////////////////////

int mmap_cache_get(mmap_cache_t* cache, cache_entry_t* entry)
{
  int res = 0;


cleanup:
  return res;
}


////////////////////////////////////////////////////////////////////////////////

int mmap_cache_put(mmap_cache_t* cache, cache_entry_t* entry)
{
  int res = 0;


cleanup:
  return res;
}
