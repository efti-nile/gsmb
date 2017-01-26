#ifndef _MSP430_FLASH_H_
#define _MSP430_FLASH_H_

#include "io430.h"
#include "types.h"

#define BEGIN_AVAILABLE_SPACE 0xF800 // It must be exactly the begin of the segment
#define END_AVAILABLAE_SPACE 0xFDFF
#define SEG_SIZE 512

u8 flash_write(u8 *src, u16 num);
u8 flash_read(u8 *dst, u16 num);

#endif
