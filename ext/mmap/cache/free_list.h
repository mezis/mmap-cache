//
// free_list.h --
//
// Allocation of identical chunks of memory within a fixed blob using 
// a linked link.
// None of the functions here perform memory allocation.
// The payload memory is never touched.
//
#include <stdint.h>
#include "helpers.h"

/*
  How a "Free list" works:

              head   |adr data ptr|  |adr data ptr|  |adr data ptr|  |adr data ptr|  |adr data ptr| 
  initial        0   |0          1|  |1          2|  |2          3|  |3          4|  |4          -|
  write 0        1   |0   XXXX   -|  |1          2|  |2          3|  |3          4|  |4          -|
  write 1        2   |0   XXXX   -|  |1   XXXX   -|  |2          3|  |3          4|  |4          -|
  delete 0       0   |0          2|  |1   XXXX   -|  |2          3|  |3          4|  |4          -|
*/

// Notes:
// - each slot can hold <slots_stride> - <offset_bytes> payload.
// - the offset to next free slot is at the *end* of each slot.
//
// You need to allocate space yourself for
// - the heade offset (<offset_bytes>)
// - the slots (<slots_stride> * <slots_count>)
struct free_list_
{
  // 1, 2 or 4 - bits per slot address
  uint8_t  offset_bytes;
  // number of allocated slots in the list.
  // should be compatible with <offset_bytes> (max 255 if 1, 65535 if 2, max 16,777,215 if 4).
  uint32_t slots_count;
  // number of free slots.
  uint32_t slots_free;
  // bytes between consecutive slots (max 1MB)
  uint32_t slots_stride;
  // address of the index to the first free slot
  uint8_t* head_slot_ptr;
  // address of the first slot payload
  uint8_t* payload_ptr;
};

typedef struct free_list_ free_list_t;

// Ignores data in all slots but sets up all offsets.
// Returns 0 on success, non-0 on failure and sets errno.
int free_list_init(free_list_t* free_list);

// Attaches a free structre to memory block previously set up with <free_list_init>.
// As a side effect, updates <slots_free>
int free_list_attach(free_list_t* free_list);

// Return in <payload> the adress of a free slot, and mark it as used.
// Will return ENOMEM if the list is full.
// Returns 0 on success, non-0 on failure and sets errno.
int free_list_alloc(free_list_t* free_list, void** payload);

// Marks slot at <payload> as unused.
// Will return EFAULT if the offset is out of range.
// Returns 0 on success, non-0 on failure and sets errno.
int free_list_free(free_list_t* free_list, void* payload);

// FIXME: add free list extension (memory block gets bigger)