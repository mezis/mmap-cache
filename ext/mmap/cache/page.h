//
// page.h --
//
// Common definition for pages
//
#include <inttypes.h>
#include "helpers.h"


struct page_t
{
  page_header_t* header_ptr;
  uint8_t*       payload_ptr;
}; 

