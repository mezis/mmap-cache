//
// page.h --
//
// Common definition for pages
//
#include <stdint.h>
#include "helpers.h"


struct page_t
{
  page_header_t* header_ptr;
  uint8_t*       payload_ptr;
}; 

