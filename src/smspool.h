#ifndef _SMSPOOL_H
#define _SMSPOOL_H

#include <string.h>
#include "types.h"

u8 *SmsPool_GetPtrForPush(u8 qty);
u8* SmsPool_Push(u8 *text, u8 qty);
u8* SmsPool_Pull(u8 *text);
u8 SmsPool_Find(u8 *text);
void SmsPool_Del(u8 *text);

#endif
