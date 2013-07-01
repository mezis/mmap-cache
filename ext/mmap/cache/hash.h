// 
// hash.h --
// 
// A fast, non-cryptographic string hasher.
//
#include <stdint.h>

#define MMAP_HASH_KEY_MAX 1024

uint32_t mmap_hash(const char* input);
