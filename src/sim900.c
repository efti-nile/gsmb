#include "sim900.h"

extern struct TelDir_TypeDef TelDir;

u8 SMS_Balance[SMS_TEXT_MAXLEN]; // TODO: maybe put it in stack?

/*!
    \brief Tryes to initialize SIM900 untill success.

    LED switched on for 0.5 second on eache attemption to initialize
    SIM900.
*/
void SIM900_ReInit(void){
    g_PWR_INIT;
    g_HPWR_INIT;
    g_STS_INIT;

func_begin:
    SIM900_PowerOff(); // Power-off SIM900

    Delay_DelayMs(5000);

    SIM900_PowerOn(); // Start SIM900

    Delay_DelayMs(100); // To have in concern that this time may be not enough

    SIM900_SoftReset();

    Delay_DelayMs(3000);

    // Switch off echo
    SIM900_SendStr("ATE0\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        goto func_begin;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(3000);

    // Set full error mode
    SIM900_SendStr("AT+CMEE=2\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        goto func_begin;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(3000);

    // Text mode
    SIM900_SendStr("AT+CMGF=1\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        goto func_begin;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(3000);

    // Choose character Set
    SIM900_SendStr("AT+CSCS=\"UCS2\"\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        goto func_begin;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(3000);

    // Extra settings
    SIM900_SendStr("AT+CSMP=17,167,0,25\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        goto func_begin;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(3000);

    // Delete all SMS
    SIM900_SendStr("AT+CMGD=1,4\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        if(SIM900_CircularBuf_Search("SMS res") == -1){ // If we received +CMS ERROR: SMS reserved, it's OK
            goto func_begin;
        }
    }

    // Give SIM900 some time for thinking
    Delay_DelayMs(3000);

    if(!SIM900_GetStatus()){
        goto func_begin;
    }

    SIM900_CircularBuffer_Purge();
}

void SIM900_ReadSms(void){
    u8 TelNum[SMS_TELNUM_LEN], TelNum_Balance[SMS_TELNUM_LEN];

    SIM900_CircularBuffer_Purge(); // Just in case

    SIM900_SendStr("AT+CMGR=1\r"); // Read received SMS

    if(!SIM900_WaitForResponse("OK", "ERROR")){
        // If we didn't receive response than delete all
        // SMSs then return to the main loop
        SIM900_SendStr("AT+CMGD=1,4\r");
        if(!SIM900_WaitForResponse("OK", "ERROR")){
            ErrorHandler(7);
        }
        SIM900_CircularBuffer_Purge();
        return;
    }

    // Extract telephone number SMS was sent by wich
    if(!SIM900_CircularBuffer_ExtractTelNum(TelNum)){
        // If the telephone number wasn't extracted
        // Such case occurs if a telephone number doesn't begin with '+7'
        SIM900_CircularBuffer_Purge();

        SIM900_SendStr("AT+CMGD=1,4\r");
        if(!SIM900_WaitForResponse("OK", "ERROR")){
            ErrorHandler(8);
        }

        SIM900_CircularBuffer_Purge();

        return;
    }

    TelNum[3] = '8';

    // Add the telephone number to the telephone dictionary
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_ADD) != -1 &&
       SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1){
        switch(TelDir_Push(TelNum)){
        // Telephone number has been pushed successfully
            case TELDIR_PUSH_RES_PUSHED:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_ADD_OK, SMS_LIFETIME);
                break;
            }
        // Telephone number has been already pushed
            case TELDIR_PUSH_RES_ALREADY_PUSHED:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_ADD_ALREADY_ADDED, SMS_LIFETIME);
                break;
            }
        // There isn't enough memory to push telephone number
            case TELDIR_PUSH_RES_NO_MEMORY:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_ADD_NO_MEMORY, SMS_LIFETIME);
                break;
            }
        // Flash writing failed
            case TELDIR_RES_FLASH_ERROR:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_ADD_FLASH_ERROR, SMS_LIFETIME);
                break;
            }
        // It never must be
            default:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_ADD_FATAL_ERROR, SMS_LIFETIME);
                break;
            }
        }
    }else
    // Delete telephone number from the dictionary
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_DEL) != -1 &&
       SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1){
        switch(TelDir_Del(TelNum)){
        // Telephone number has been deleted successfully
            case TELDIR_DEL_RES_DELETED:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_DEL_OK, SMS_LIFETIME);
                break;
            }
        // Flash writing failed
            case TELDIR_RES_FLASH_ERROR:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_DEL_FLASH_ERROR, SMS_LIFETIME);
                break;
            }
        // It never must be
            default:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_DEL_FATAL_ERROR, SMS_LIFETIME);
                break;
            }
        }
    }else
    // Clean the telephone dictionary
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_CLEAN) != -1 &&
       TelDir_FindTelNumber(TelNum) != -1 && SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1){
        switch(TelDir_Clean()){
        // The telephone dictionary cleaned
            case TELDIR_CLEAN_RES_CLEANED:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_CLEAN_OK, SMS_LIFETIME);
                break;
            }
        // Flash writing failed
            case TELDIR_RES_FLASH_ERROR:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_CLEAN_FLASH_ERROR, SMS_LIFETIME);
                break;
            }
        // It never must be
            default:{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_CLEAN_FATAL_ERROR, SMS_LIFETIME);
                break;
            }
        }
    }else
    // Set new password
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_SET_PASSWORD) != -1 &&
       TelDir_FindTelNumber(TelNum) != -1){
        // Extract new password -- all characters from space after the command to the end of SMS
        SIM900_CircularBuffer_ExtractWithUnicodeDelimeter(SIM900_SMS_CMD_SET_PASSWORD, \
                TelDir_GetPwd(), TELDIR_PWD_MAXLEN, SIM900_SMS_END_PATTERN);
        // Compile the answer in the pool
        u8 *text = SmsPool_GetPtrForPush(1);
        strcpy((char *) text, (const char *) SIM900_SMS_REPORT_PASSWORD_SET);
        strcpy((char *) text + sizeof(SIM900_SMS_REPORT_PASSWORD_SET) - 1, \
            (const char *) TelDir_GetPwd());
        // Push answer in the SMS queue
        SMS_Queue_Push(TelNum,  (u8 *)text, SMS_LIFETIME);
    }else
    // HW reset the burner
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_RESET_BURNER) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        // Init the reset pin P3.2
        P3DIR |= BIT2;
        P3REN &= BIT2^0xFF;
        P3SEL &= BIT2^0xFF;
        P3DS  &= BIT2^0xFF;
        // Pull down
        P3OUT &= BIT2^0xFF;
        Delay_DelayMs(100);
        P3OUT |= BIT2;
        //SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_RESET_BURNER, SMS_LIFETIME);
    }else
    // Request to change the begin and the end of the night
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_SET_NIGHT_TIME) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
        static volatile u8 numBuffer[(2+1+2)*4]; // 2 digits + 1 space + 2digits, all in UCS2
		// Extract 5 UCS2 symbols after SIM900_SMS_CMD_SET_NIGHT_TIME substring
        SIM900_CircularBuffer_Extract(SIM900_SMS_CMD_SET_NIGHT_TIME, (u8 *)numBuffer, (2+1+2)*4, '\x00');
        if(numBuffer[6] == '2'){ // If the begin hour number contains 1 digit
            State.night_begin = numBuffer[3] - '0';
            if(numBuffer[14] == '3'){
                State.night_end = 10 * (numBuffer[11] - '0') + (numBuffer[15] - '0');
            }else{
                State.night_end = numBuffer[11] - '0';
            }
        }else // If the begin hour number contains 2 digits
        if(numBuffer[10] == '2'){
            State.night_begin = 10 * (numBuffer[3] - '0') + (numBuffer[7] - '0');
            if(numBuffer[18] == '3'){
                State.night_end = 10 * (numBuffer[15] - '0') + (numBuffer[19] - '0');
            }else{
                State.night_end = numBuffer[15] - '0';
            }
        }
        // Check parameters
        if(State.night_begin <= 23 && State.night_end <= 23 && State.night_begin != State.night_end){
            State.request_set_night_time = 1;
            State.request_in_progress = 1;
        }else{
            State.request_set_night_time = 0;
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_GENERAL_INVALID_COMMAND, SMS_LIFETIME);
        }
    }else
    // Request to change the night temperature
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_SET_NIGHT_TEMP) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
        static volatile u8 numBuffer[(2+1+2)*4]; // 2 digits + 1 space + 2digits, all in UCS2
        SIM900_CircularBuffer_Extract(SIM900_SMS_CMD_SET_NIGHT_TEMP, (u8 *)numBuffer, (2+1+2)*4, '\x00');
        // Read max. and min. temperatures
        State.temp_night_max = 10 * (numBuffer[3] - '0')  + (numBuffer[7] - '0');
        State.temp_night_min = 10 * (numBuffer[15] - '0') + (numBuffer[19] - '0');
        if(State.temp_night_min < 10 || State.temp_night_min > 90 ||
           State.temp_night_max < 10 || State.temp_night_max > 90 ||
           State.temp_night_min == State.temp_night_max){
            State.request_set_night_temp = 0;
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_GENERAL_INVALID_COMMAND, SMS_LIFETIME);
        }else{
            // If max. temp. < min. temp., swap them
            if(State.temp_night_max < State.temp_night_min){
                u8 swap;
                swap = State.temp_night_max;
                State.temp_night_max = State.temp_night_min;
                State.temp_night_min = swap;
            }
            State.request_set_night_temp = 1;
            State.request_in_progress = 1;
        }
    }else
    // Request to change the day temperature
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_SET_TEMP) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
        static volatile u8 numBuffer[(2+1+2)*4]; // 2 digits + 1 space + 2digits, all in UCS2
        SIM900_CircularBuffer_Extract(SIM900_SMS_CMD_SET_TEMP, (u8 *)numBuffer, (2+1+2)*4, '\x00');
        // Read max. and min. temperatures
        State.temp_day_max = 10 * (numBuffer[3] - '0')  + (numBuffer[7] - '0');
        State.temp_day_min = 10 * (numBuffer[15] - '0') + (numBuffer[19] - '0');
        if(State.temp_day_min < 10 || State.temp_day_min > 90 ||
           State.temp_day_max < 10 || State.temp_day_max > 90 ||
           State.temp_day_min == State.temp_day_max){
            State.request_set_day_temp = 0;
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_GENERAL_INVALID_COMMAND, SMS_LIFETIME);
        }else{
            // If max. temp. < min. temp., swap them
            if(State.temp_day_max < State.temp_day_min){
                u8 swap;
                swap = State.temp_day_max;
                State.temp_day_max = State.temp_day_min;
                State.temp_day_min = swap;
            }
            State.request_set_day_temp = 1;
            State.request_in_progress = 1;
        }
    }else
    // Check current parameters and settings
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_RECV_SETTINGS) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        State.request_sen_get = 1;
        State.request_in_progress = 1;
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
    }else
    // Disable night mod
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_NIGHT_MODE_OFF) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        State.request_night_mode_off = 1;
        State.request_night_mode_on = 0;
        State.request_in_progress = 1;
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
    }else
    // Enable night mod
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_NIGHT_MODE_ON) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        State.request_night_mode_on = 1;
        State.request_night_mode_off = 0;
        State.request_in_progress = 1;
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
    }else
    // Switch off the burner
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_BURNER_OFF) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        State.request_burner_switch_on = 0;
        State.request_burner_switch_off = 1;
        State.request_in_progress = 1;
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
    }else
    // Switch on the burner
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_BURNER_ON) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        State.request_burner_switch_on = 1;
        State.request_burner_switch_off = 0;
        State.request_in_progress = 1;
        strcpy((char *)State.TelNumOfSourceOfRequest, (char const *)TelNum);
    }else
    // Set number for balance checking
    if(SIM900_CircularBuffer_ExtractBalanceNum(SIM900_SMS_CMD_SET_BALANCE, TelNum_Balance, sizeof(TelNum_Balance) - 1) &&
        (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        if(TelDir_SetBalanceNumber(TelNum_Balance) == TELDIR_SET_BALANCE_TELNUM_RES_OK){
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_BALANCE_SET_OK, SMS_LIFETIME);
        }else{
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_BALANCE_SET_ERROR, SMS_LIFETIME);
        }
    }else
    // Request balance
    if(SIM900_CircularBuf_Search(SIM900_SMS_CMD_CHECK_BALANCE) != -1 &&
       (TelDir_FindTelNumber(TelNum) != -1 || SIM900_CircularBuf_Search(TelDir_GetPwd()) != -1)){
        // Check if user set the telephone number for balance check
        if(TelDir_isBalanceNumberSet()){
            // Make up command to request balance
            u8 CMD[sizeof("AT+CUSD=1,\"AAAABBBBCCCCDDDDEEEE\"\r") + 8] = "AT+CUSD=1,\""; // +8 for just in case
            strcat((char *)CMD, (char const *)TelDir_GetBalanceNumber());
            strcat((char *)CMD, "\"\r");

            SIM900_SendStr(CMD);

            if(SIM900_WaitForResponse("+CUSD: 0,\"", "ERROR")){
                SIM900_CircularBuffer_Extract(",\"", SMS_Balance, sizeof(SMS_Balance) - 1, '"');
                SMS_Queue_Push(TelNum, SMS_Balance, SMS_LIFETIME);
            }else{
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_BALANCE_ERROR, SMS_LIFETIME);
            }
        }else{
            // Ask user to set this telephone number
            SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_BALANCE_TELNUM_NOT_SET, SMS_LIFETIME);
        }
    }

    SIM900_CircularBuffer_Purge();

    SIM900_SendStr("AT+CMGD=1,4\r");
    if(!SIM900_WaitForResponse("OK", "ERROR")){
        ErrorHandler(10);
        SIM900_CircularBuffer_Purge();
        return;
    }

    SIM900_CircularBuffer_Purge();
}

void SIM900_SendSms(void){
    u8 TelNum[SMS_TELNUM_LEN];
    u8 *SmsText;
    u32 LifeTime;

    // Pop a SMS from the queue. If life-time elapsed -- return from here
    // and don't send the SMS
    if((LifeTime = SMS_Queue_Pop(TelNum, &SmsText)) == 0){
        return;
    }

    // SMS sending...
    SIM900_SendStr("AT+CMGS=\"");
    SIM900_SendStr(TelNum);
    SIM900_SendStr("\"\r");

    if(!SIM900_WaitForResponse(">", "ERROR")){
        SIM900_SendStr("ERROR 9");
        ErrorHandler(9);
        SIM900_CircularBuffer_Purge();
        return;
    }
    SIM900_CircularBuffer_Purge();

    Delay_DelayMs(5000);

    SIM900_SendStr(SmsText); // Send SMS text
    SIM900_SendStr("\x1A\r"); // End of SMS text

    // Try during several time intervals to receive acknoledgment
    for(u8 i = 0; i < 4; i++){
      if(SIM900_WaitForResponse("OK", "ERROR")){
        return;
      }
    }

    // If we didn't receive one, put outgoing SMS in queue again with decreased
    // lifetime
    SMS_Queue_Push(TelNum, SmsText, LifeTime - 1);

    // Then restart SIM900
    SIM900_SendStr("ERROR 6");
    ErrorHandler(6);
    SIM900_CircularBuffer_Purge();
}

u8 SIM900_PowerOn(void){
    g_HPWR_SET;
    g_PWR_SET;
    g_USART1_TX_ENABLE;
	return 0;
}

u8 SIM900_PowerOff(void){
    g_HPWR_CLEAR;
    g_PWR_CLEAR;
    g_USART1_TX_DISABLE;
    return 0;
}

u8 SIM900_SoftReset(void){
    g_PWR_CLEAR;
    Delay_DelayMs(1000);
    g_PWR_SET;
    return 0;
}

u8 SIM900_HoldReset(void){
    g_PWR_CLEAR;
    return 0;
}

u8 SIM900_GetStatus(void){
    return g_STS_READ;
}

void SIM900_SendStr(u8* str){
    MSP430_UART_Send(UART_SIM900, str, strlen((const char *)str));
}

void SIM900_CircularBuffer_Purge(void){
    CirBuf_Tail = CirBuf_Head;
    CirBuf_NumBytes = 0;
}

s16 SIM900_CircularBuf_Search(const u8 pattern[]){
    u16 i, j, k, l, p;

    // Find length of given pattern
    for(l = 0; pattern[l] != '\0'; ++l);

    // Check if circular buffer can fit in given pattern
    if(l > CirBuf_NumBytes){
        return -1;
    }

    // Index of byte before the last received one
    p = CirBuf_Tail > 0 ? --CirBuf_Tail : CIRBUF_SIZE - 1;

    // Calculate offset for search
    if(CirBuf_Head > l - 1){
        i = CirBuf_Head - l;
    }else{
        i = (CIRBUF_SIZE - 1) - (l - CirBuf_Head - 1);
    }

    while(i != p){
        for(j = i, k = 0; k < l; ++k){
            if(pattern[k] != CirBuf[j]){
                break;
            }else
            if(k == l - 1){
                return i;
            }
            j = j < CIRBUF_SIZE - 1 ? ++j : 0;
        }
        i = i > 0 ? --i : CIRBUF_SIZE - 1;
    }

    return -1;
}

u8 SIM900_CircularBuffer_Extract(const u8 Pattern[], u8 *Dst, u16 Num, u8 DelChar){
    u16 i, j, k, l, p, q;

    // Find length of given pattern
    for(l = 0; Pattern[l]; ++l);

    // Check if circular buffer can fit in given pattern
    if(CirBuf_NumBytes < l){
        return 0;
    }

    // Index of byte before the last received one
    p = CirBuf_Tail > 0 ? --CirBuf_Tail : CIRBUF_SIZE - 1;

    // Calculate offset for search
    if(CirBuf_Head >= l){
        i = CirBuf_Head - l;
    }else{
        i = CIRBUF_SIZE - (l - CirBuf_Head);
    }

    for( ; i != p; i = i > 0 ? --i : CIRBUF_SIZE - 1){
        for(    j = i, k = 0;
                Pattern[k] == CirBuf[j];
                j = j < CIRBUF_SIZE - 1 ? ++j : 0 ){
            if(k++ >= l - 1){
                j = j < CIRBUF_SIZE - 1 ? ++j : 0; // set pointer to the begin
                for(    q = 0;
                        q < Num && CirBuf[j] != DelChar;
                        j = j < CIRBUF_SIZE - 1 ? ++j : 0, ++q  ) Dst[q] = CirBuf[j];
                Dst[q] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

u8 SIM900_CircularBuffer_ExtractWithUnicodeDelimeter(const u8 Pattern[], u8 *Dst, u16 Num, const u8* DelChar){
    u16 i, j, k, l, p, q;

    // Find length of given pattern
    for(l = 0; Pattern[l]; ++l);

    // Check if circular buffer can fit in given pattern
    if(CirBuf_NumBytes < l){
        return 0;
    }

    // Index of byte before the last received one
    p = CirBuf_Tail > 0 ? --CirBuf_Tail : CIRBUF_SIZE - 1;

    // Calculate offset for search
    if(CirBuf_Head >= l){
        i = CirBuf_Head - l;
    }else{
        i = CIRBUF_SIZE - (l - CirBuf_Head);
    }

    for( ; i != p; i = i > 0 ? --i : CIRBUF_SIZE - 1){
        for(    j = i, k = 0;
                Pattern[k] == CirBuf[j];
                j = j < CIRBUF_SIZE - 1 ? ++j : 0 ){
            if(k++ >= l - 1){
                j = j < CIRBUF_SIZE - 1 ? ++j : 0; // set pointer to the begin
                for(    q = 0;
                        q < Num &&
                            (CirBuf[j] != DelChar[0] ||
                             CirBuf[j + 1 < CIRBUF_SIZE ? j + 1 : j + 1 - CIRBUF_SIZE] != DelChar[1] ||
                             CirBuf[j + 2 < CIRBUF_SIZE ? j + 2 : j + 2 - CIRBUF_SIZE] != DelChar[2] ||
                             CirBuf[j + 3 < CIRBUF_SIZE ? j + 3 : j + 3 - CIRBUF_SIZE] != DelChar[3]);
                        j = j < CIRBUF_SIZE - 1 ? ++j : 0, ++q  )
                    Dst[q] = CirBuf[j];
                Dst[q] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

u8 SIM900_CircularBuffer_ExtractTelNum(u8 *Dst){
    u16 i, j, k, l, p, q;
    u8 pattern[] = "\",\"002B";

    // Find length of the pattern
    l = sizeof(pattern) - 1;

    // Check if circular buffer can fit in the pattern
    if(CirBuf_NumBytes < l){
        return 0;
    }

    // Index of byte before the last received one
    p = CirBuf_Tail > 0 ? --CirBuf_Tail : CIRBUF_SIZE - 1;

    // Calculate offset for search
    if(CirBuf_Head > l - 1){
        i = CirBuf_Head - l;
    }else{
        i = (CIRBUF_SIZE - 1) - (l - CirBuf_Head - 1);
    }

    while(i != p){
        for(j = i, k = 0; k < l; ++k){
            if(pattern[k] != CirBuf[j]){
                break;
            }else
            if(k == l - 1){
                j = j < CIRBUF_SIZE - 1 ? ++j : 0; // set pointer to the begin
                for(q = 0; q < 44; ++q){
                    Dst[q] = CirBuf[j];
                    j = j < CIRBUF_SIZE - 1 ? ++j : 0;
                }
                Dst[q] = '\0';
                return 1;
            }
            j = j < CIRBUF_SIZE - 1 ? ++j : 0;
        }
        i = i > 0 ? --i : CIRBUF_SIZE - 1;
    }

    return 0;
}

u8 SIM900_CircularBuffer_ExtractBalanceNum(const u8 Pattern[], u8 *Dst, u16 Num){
    u16 i, j, k, l, p, q;

    // Find length of given pattern
    for(l = 0; Pattern[l]; ++l);

    // Check if circular buffer can fit in given pattern
    if(CirBuf_NumBytes < l){
        return 0;
    }

    // Index of byte before the last received one
    p = CirBuf_Tail > 0 ? --CirBuf_Tail : CIRBUF_SIZE - 1;

    // Calculate offset for search
    if(CirBuf_Head >= l){
        i = CirBuf_Head - l;
    }else{
        i = CIRBUF_SIZE - (l - CirBuf_Head);
    }

    for( ; i != p; i = i > 0 ? --i : CIRBUF_SIZE - 1){
        for(    j = i, k = 0;
                Pattern[k] == CirBuf[j];
                j = j < CIRBUF_SIZE - 1 ? ++j : 0 )
        {
            if(k++ >= l - 1){
                j = j < CIRBUF_SIZE - 1 ? ++j : 0; // set pointer to the begin
                for(    q = 0;
                        q < Num;
                        j = j < CIRBUF_SIZE - 1 ? ++j : 0, ++q  )
                {
                    Dst[q] = CirBuf[j];
                    if( q + 1 > 0 && (q + 1) % 4 == 0 ){
                        if(!IsTelNumberSymbol( Dst + (q - 3) )){
                            Dst[q-3] = '\0';
                            return 1;
                        }
                    }
                }

                Dst[q] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

u8 SIM900_WaitForResponse(u8 *pos_resp, u8 *neg_resp){
    u32 timeout = SIM900_WAIT_FOR_RESPONSE_TIMEOUT;
    while(timeout--){
        Delay_DelayMs(100);
        if(SIM900_CircularBuf_Search(pos_resp) != -1){
            return 1;
        }
        if(SIM900_CircularBuf_Search(neg_resp) != -1){
            return 0;
        }
    }
    return 0;
}

/*!
    \brief Returns true if given UCS2-symbol may be in tel. number

    The function returns true if spcified UCS2-symbol may be
    in telephone number. I.e. if the symbol is 0..9, * or #.
*/
u8 IsTelNumberSymbol(const u8 symbol[]){
    if(symbol[0] == '0' && symbol[1] == '0'){
        if(symbol[2] == '3'){
            return 1;
        }else
        if(symbol[2] == '2'){
            if(symbol[3] == '3' || symbol[3] == 'A'){
                return 1;
            }
        }
    }
    return 0;
}