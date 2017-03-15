#include "sms_queue.h"

static struct SMS_Queque_TypeDef SMS_Queue;

void SMS_Queue_Init(void){
    SMS_Queue.FirstItem = 0;
    SMS_Queue.LastItem = 0;
    SMS_Queue.NumItems = 0;
}

void SMS_Queue_Push(u8 *TelNum, const u8 *SmsText, u8 LifeTime){
    TelNum[SMS_TELNUM_LEN-1] = '\0';
    strcpy((char *)SMS_Queue.List[SMS_Queue.LastItem].TelNum, (char const *)TelNum);
    SMS_Queue.List[SMS_Queue.LastItem].SmsText = (u8 *)SmsText;
    SMS_Queue.List[SMS_Queue.LastItem].LifeTime = LifeTime;
    SMS_Queue.LastItem = SMS_Queue.LastItem < SMS_QUEUE_MAXSIZE - 1 ? ++SMS_Queue.LastItem : 0;
    if(SMS_Queue.LastItem == SMS_Queue.FirstItem){
        if(SmsPool_Find(SMS_Queue.List[SMS_Queue.FirstItem].SmsText)){
            SmsPool_Del(SMS_Queue.List[SMS_Queue.FirstItem].SmsText);
        }
        SMS_Queue.FirstItem = SMS_Queue.FirstItem < SMS_QUEUE_MAXSIZE - 1 ? ++SMS_Queue.FirstItem : 0;
        SMS_Queue.NumItems--;
    }
    SMS_Queue.NumItems++;
}

u8 SMS_Queue_Pop(u8 *TelNum, u8 **SmsText){
    u8 LifeTime;
    if(SMS_Queue.NumItems > 0){
        strcpy((char *)TelNum, (char const *)SMS_Queue.List[SMS_Queue.FirstItem].TelNum);
        *SmsText = SMS_Queue.List[SMS_Queue.FirstItem].SmsText;
        if(SmsPool_Find(*SmsText)){
            SmsPool_Pull(*SmsText);
        }
        LifeTime = SMS_Queue.List[SMS_Queue.FirstItem].LifeTime;
        SMS_Queue.FirstItem = SMS_Queue.FirstItem < SMS_QUEUE_MAXSIZE - 1 ? ++SMS_Queue.FirstItem : 0;
        SMS_Queue.NumItems--;
        return LifeTime;
    }else{
        return 0;
    }
}
