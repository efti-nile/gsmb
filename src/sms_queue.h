#ifndef _SMS_QUEUE_H_
#define _SMS_QUEUE_H_

#include "io430.h"
#include "types.h"
#include "smspool.h"
#include <string.h>

#define SMS_LIFETIME 5

void SMS_Queue_Init(void);
void SMS_Queue_Push(u8 *TelNum, const u8 SmsText[], u8 LifeTime);
u8 SMS_Queue_Pop(u8 *TelNum, u8 **SmsText);
u8 SMS_Queue_NumItems(void);

#endif
