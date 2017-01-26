#ifndef _UCS2_H
#define _UCS2_H

#include "string.h"
#include "types.h"

#define ENG_BEGIN 0x20
#define ENG_END 0x7E

#define UNICODE_RUS_BEGIN 0X410
#define UNICODE_RUS_END 0x044f

#define CP1251_RUS_BEGIN 0xC0
#define CP1251_RUS_END 0xFF

void strToCP1251(u8 *dst, u8 *src);
void strToUCS2(u8 *dst, u8 *src);
u8 *toUCS2(u8 c);
u8 toCP1251(u8 *c);
static u8 hextodig(u8 c);
static u8 digtohex(u8 n);

#endif
