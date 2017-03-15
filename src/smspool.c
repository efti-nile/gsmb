#include "smspool.h"

static struct SmsPool_Item_TypeDef Pool[SMSPOOL_SIZE];

u8 *SmsPool_GetPtrForPush(u8 qty){
    for(u8 i = 0; i < SMSPOOL_SIZE; i++){
        if(Pool[i].Qty == 0){
            Pool[i].Qty = qty;
            return Pool[i].Text;
        }
    }
    return NULL;
}

u8* SmsPool_Push(u8 *text, u8 qty){
    for(u8 i = 0; i < SMSPOOL_SIZE; i++){
        if(Pool[i].Qty == 0){
            Pool[i].Qty = qty;
            return (u8 *)strcpy((char *)Pool[i].Text, (char const*)text);
        }
    }
    return NULL;
}

u8* SmsPool_Pull(u8 *text){
    u8 *pqty = &(((struct SmsPool_Item_TypeDef *)text)->Qty);
    if(*pqty > 0){
        (*pqty)--;
    }
    return text;
}

void SmsPool_Del(u8 *text){
    (((struct SmsPool_Item_TypeDef *)text)->Qty) = 0;
}

u8 SmsPool_Find(u8 *text){
  return (u8 *)&Pool <= text && text < (u8 *)&Pool + sizeof(Pool);
}