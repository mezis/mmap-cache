#include "hash.h"
#include "lookup3.h"

#define MMAP_HASH_SEED 0x00000000

uint32_t mmap_hash(cont char* input)
{
  return hashlittle((const void*)input, strnlen(input, MMAP_HASH_KEY_MAX), MMAP_HASH_SEED);
}
