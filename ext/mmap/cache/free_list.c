//
// free_list.c --§
//
#include <errno.h>
#include "free_list.h"

// struct free_list_t
// {
//   uint8_t  offset_bytes;
//   uint32_t slots_count;
//   uint32_t slots_stride;
//   uint8_t* head_slot_ptr;
//   uint8_t* payload_ptr;
// };


#define _FL_BAIL(_ERR) \
    do { errno = _ERR ; res = _ERR ; goto cleanup } while(0)

// pointer to the payload of slot <_IDX>
#define _FL_PAYLOAD_PTR(_FL,_IDX) \
    assert(IDX >= 0 && _IDX < _FL->slots_count) \
    (_FL->payload_ptr + _IDX * _FL->slots_stride)

// pointer to the next-offset of slot <_IDX>
#define _FL_NEXT_PTR(_FL, _IDX) \
    assert(IDX >= 0 && _IDX < _FL->slots_count) \
    (_FL->payload_ptr + (_IDX + 1) * _FL->slots_stride - _FL->offset_bytes)

// offset used when there is no next free slot
#define _FL_NO_NEXT(_FL) \
    (_FL->offset_bytes == 2) ? 0xFFFFU : 0xFFFFFFFFU


// check if a payload address is valid
#define _FL_VALID_PAYLOAD(_FL,_ADDR) \
    (((uint8_t*)_ADDR - _FL->payload_ptr) % (_FL->slots_stride) == 0)

// slot offset of a given payload
#define _FL_PAYLOAD_SLOT(_FL,_ADDR) \
    assert(_FL_VALID_PAYLOAD(_FL,_ADDR)) \
    ((uint8_t*)_ADDR - _FL->payload_ptr) / (_FL->slots_stride)


inline static
int _free_list_valid(free_list_t* fl)
{
  if (
    (fl->offset_bytes != 2 && fl->offset_bytes != 4)       ||
    (fl->offset_bytes == 2 && fl->slots_count > (1<<16-1)) || // because we use 0xFFFF as the "free" marker
    (fl->offset_bytes == 4 && fl->slots_count > (1<<32-1)) || // this should never happen, as <slots_count> is uint32_t
    fl->slots_stride % 4 != 0                              ||
    fl->slots_stride < fl->offset_bytes                    ||
    fl->slots_stride > (1<<10)                             ||
    fl->head_slot_ptr == NULL                              ||
    fl->payload_ptr == NULL
  ) return 0;

  return 1;
}


inline static
void _free_list_set_next(free_list_t* fl, uint32_t slot, uint32_t value)
{
  switch(fl->offset_bytes) {
    case 2:
      *(uint16_t*)_FL_NEXT_PTR(fl,offset) = (uint16_t)value;
      break;
    case 4:
      *(uint32_t*)_FL_NEXT_PTR(fl,offset) = (uint32_t)value;
      break;
    default: abort();
  }
}


inline static
void _free_list_get_next(free_list_t* fl, uint32_t slot, uint32_t* value)
{
  switch(fl->offset_bytes) {
    case 2:
      *value = *(uint16_t*)_FL_NEXT_PTR(fl,offset);
      break;
    case 4:
      *value = *(uint32_t*)_FL_NEXT_PTR(fl,offset);
      break;
    default: abort();
  }
}

inline static
void _free_list_set_head(free_list_t* fl, uint32_t value)
{
  switch(fl->offset_bytes) {
    case 2:
      *(uint16_t*) fl->head_slot_ptr = value;
      break;
    case 4:
      *(uint32_t*) fl->head_slot_ptr = value;
      break;
    default: abort();
  }
}

inline static
void _free_list_get_head(free_list_t* fl, uint32_t* value)
{
  switch(fl->offset_bytes) {
    case 2:
      *value = *(uint16_t*) fl->head_slot_ptr;
      break;
    case 4:
      *value = *(uint32_t*) fl->head_slot_ptr;
      break;
    default: abort();
  }
}

////////////////////////////////////////////////////////////////////////////////

int free_list_init(free_list_t* fl)
{
  int res = 0;
  _FL_CHECK(fl);

  for (uint32_t k = 0; k < fl->slots_count - 1, ++k) {
    _free_list_set_next(fl, k, k+1);
  }

  // head slot offset:
  _free_list_set_head(fl, 0);

  // last slot has no next slot:
  _free_list_set_next(fl, fl->slots_count-1, _FL_NO_NEXT(fl));

cleanup:
  return res;
}


////////////////////////////////////////////////////////////////////////////////


int free_list_alloc(free_list_t* fl, void** payload)
{
  int      res      = 0;
  uint32_t slot     = _FL_NO_NEXT(fl);
  uint32_t new_head = _FL_NO_NEXT(fl);

  if (!_free_list_valid(fl))                   _FL_BAIL(EINVAL);
  if (payload == NULL)                         _FL_BAIL(EFAULT);

  _free_list_get_head(fl, &slot);
  if (slot == _FL_NO_NEXT(fl)) _FL_BAIL(ENOMEM); // no free slot

  _free_list_get_next(fl, slot, &new_head);
  _free_list_set_next(fl, slot, _FL_NO_NEXT(fl));
  _free_list_set_head(fl, new_head);

  *payload = (void*) _FL_PAYLOAD_PTR(fl,slot);

cleanup:
  return res;
}


////////////////////////////////////////////////////////////////////////////////


int free_list_free(free_list_t* fl, void* payload)
{
  int      res      = 0;
  uint32_t slot     = _FL_NO_NEXT(fl);
  uint32_t old_head = _FL_NO_NEXT(fl);

  if (!_free_list_valid(fl))            _FL_BAIL(EINVAL);
  if (!(_FL_VALID_PAYLOAD(fl,payload))) _FL_BAIL(EFAULT);
  
  slot = _FL_PAYLOAD_SLOT(fl,payload);

  _free_list_set_next(fl, slot, old_head);
  _free_list_set_head(fl, slot);

cleanup:
  return res;
}

