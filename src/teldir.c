#include "teldir.h"

__no_init static struct TelDir_TypeDef TelDir;
static const u8 TELDIR_DEFAULT_PWD[] = "003100320033"; // Default password

void TelDir_Init(void){
    if(*((u8*)BEGIN_AVAILABLE_SPACE) == 0xFF){ // If it is the first device start
        TelDir_Clean();
        TelDir_SetPwd((u8 *)TELDIR_DEFAULT_PWD);
    }

    flash_read((u8 *)&TelDir, sizeof(TelDir));
}

u8 TelDir_Clean(void){
    TelDir.NumItems = 0;
    TelDir.isBalanceTelNumSet = 0;
    if(flash_write((u8 *)&TelDir, sizeof(TelDir)) == 0){
        return TELDIR_CLEAN_RES_CLEANED;
    }else{
        return TELDIR_RES_FLASH_ERROR;
    }
}

u8 TelDir_Push(u8 *TelNum){
    u8 i;
    for(i = 0; i < TelDir.NumItems; ++i){
        if(strcmp((char const *)TelDir.List[i], (char const *)TelNum) == 0){
            return TELDIR_PUSH_RES_ALREADY_PUSHED;
        }
    }
    if(TelDir.NumItems < TELDIR_SIZE){
        strcpy((char *)TelDir.List[TelDir.NumItems++], (char const *)TelNum);
        if(flash_write((u8 *)&TelDir, sizeof(TelDir)) == 0){
            return TELDIR_PUSH_RES_PUSHED;
        }else{
            return TELDIR_RES_FLASH_ERROR;
        }
    }else{
        return TELDIR_PUSH_RES_NO_MEMORY;
    }
}

u8 TelDir_SetBalanceNumber(u8 *TelNum){
    strcpy((char *)TelDir.BalanceTelNum, (char const *)TelNum);
    if(flash_write((u8 *)&TelDir, sizeof(TelDir)) == 0){
        TelDir.isBalanceTelNumSet = 1;
        return TELDIR_SET_BALANCE_TELNUM_RES_OK;
    }else{
        return TELDIR_RES_FLASH_ERROR;
    }
}

u8 *TelDir_GetBalanceNumber(void){
    return TelDir.BalanceTelNum;
}

u8 TelDir_Del(u8 *TelNum){
    u8 i, j;
    for(i = 0; i < TelDir.NumItems; ++i){
        if(strcmp((char const *)TelDir.List[i], (char const *)TelNum) == 0){
            for( j = i; j + 1 < TelDir.NumItems; ++j){
                strcpy((char *)TelDir.List[j], (char const *)TelDir.List[j + 1]);
            }
            TelDir.NumItems--;
        }
    }
    if(flash_write((u8 *)&TelDir, sizeof(TelDir)) == 0){
        return TELDIR_DEL_RES_DELETED;
    }else{
        return TELDIR_RES_FLASH_ERROR;
    }
}

s8 TelDir_FindTelNumber(u8 *TelNum){
    u8 i;
    for(i = 0; i < TelDir.NumItems; ++i){
        if(strcmp((char const *)TelDir.List[i], (char const *)TelNum) == 0){
            return i;
        }
    }
    return -1;
}

void TelDir_Iterator_Init(void){
    TelDir.Iterator = 0;
}

u8 TelDir_GetNextTelNum(u8 *TelNum){
    if(TelDir.Iterator < TelDir.NumItems){
        strcpy((char *)TelNum, (char const *)TelDir.List[TelDir.Iterator++]);
        return 1;
    }else{
        return 0;
    }
}

u8 TelDir_isBalanceNumberSet(void){
    if(TelDir.isBalanceTelNumSet){
        return 2;
    }else{
        return 0;
    }
}

u8 TelDir_NumItems(void){
    return TelDir.NumItems;
}

u8 TelDir_SetPwd(u8 *new_pwd){
    strcpy((char *)TelDir.Pwd, (char const *)new_pwd);
    if(flash_write((u8 *)&TelDir, sizeof(TelDir)) == 0){
        return TELDIR_PWD_SET_RES_OK;
    }else{
        return TELDIR_RES_FLASH_ERROR;
    }
}

u8 *TelDir_GetPwd(void){
    return TelDir.Pwd;
}