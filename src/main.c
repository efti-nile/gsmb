#include "main.h"

struct State_TypeDef State;
struct InPack_TypeDef InPack;
struct OutPack_TypeDef OutPack;

int main(void)
{
    WDTCTL = WDTPW + WDTCNTCL; // 1 min 25 s watchdog @ SMCLK 25MHz

    State.initialization_in_progress = 1;

    LED_INIT;
    MSP430_UCS_Init();
    Delay_Init();
    TelDir_Init();

#ifdef __DBG__
    //TelDir_SetBalanceNumber("002A0031003000300023");
    TelDir_Push("00380039003100390038003000340031003100350034");
#endif

    MSP430_UART_Init();
    SMS_Queue_Init();
    __bis_SR_register(GIE);
    SysTimer_Start();
    SIM900_ReInit();

    State.initialization_in_progress = 0;
    LED_ON;

    while(1){
        if(!SIM900_GetStatus()){
            SIM900_ReInit();
        }

        // With 33 * 0.15 = 5 seconds period approx. check the signal
        if(State.sys_time % 33 == 0){
            u8 result_str[2*4 + 1];
            SIM900_SendStr("AT+CSQ=1\r");
            if(!SIM900_WaitForResponse("OK", "ERROR")){
                ErrorHandler(5);
            }
            // Extract required substring from the answer
            SIM900_CircularBuffer_Extract("+CSQ: ", result_str, 2*4, ',');
            // Check if the given substring is correct
            if(!isOnlyUCS2Numbers(result_str)){
                ErrorHandler(11);
            }
            // Extract the result and blink it out via LED
            u16 GSM_signal_level = UCS2ToNumber(result_str)
            if( GSM_signal_level <= 10){ // Low levl
                BlinkOut(2);
            }else if(GSM_signal_level <= 20){ // Normal level
                BlinkOut(3);
            }else if(GSM_signal_level <= 31){ // Good level
                BlinkOut(4);
            }else if(GSM_signal_level == 99){ // No signal
                BlinkOut(1);
            }else{
                ErrorHandler(12);
            }
        }

		WDTCTL = WDTPW + WDTCNTCL;

        if(SIM900_CircularBuf_Search("+CMTI") != -1){
            // If we have any pending requests than do not read new SMS
            if(!State.request_burner_switch_off &&
               !State.request_burner_switch_on &&
               !State.request_sen_get &&
               !State.request_night_mode_on &&
               !State.request_night_mode_off){
                SIM900_ReadSms();
            }
        }
        SIM900_SendSms();
    }
}

/*!
    \brief Blink out via LEDs given number
*/
void BlinkOut(u16 n){
    LED_OFF;
    Delay_DelayMs(1000);
    while(n-- != 0){
        LED_ON;
        Delay_DelayMs(200);
        LED_OFF;
        Delay_DelayMs(200);
    }
    Delay_DelayMs(1000);
    LED_ON;
}

/*!
    \brief Check if the given string contains only numbers in UCS2

    Examples of string, which makes the function to return 1 (i. e. true):
        "00300031", "0032003500390032"
    Examples of string, which makes the function to return 0 (i. e. false):
        "0300031", "0062003500390032"
*/
u16 isOnlyUCS2Numbers(u8 str[]){
    u16 i, l = strlen((const char *)str);
    if(l % 4 != 0){
        return 0;
    }
    for(i < 0; i < l; i += 4){
        if(result[i + 0] != '0' ||
           result[i + 1] != '0' ||
           result[i + 2] != '3' ||
           result[i + 3]  < '0' || result[i + 3] > '9'){
            return 0;
        }
    }
    return 1;
}

/*!
    \brief Converts UCS2 number string in the number

    May converts numbers only 0..65535. Doesn't any checks of the given
    string.

    Example of valid strings: "00300031", "0032003500390032"
*/
u16 UCS2ToNumber(u8 str[]){
    u16 i = 0, res = 0;
    do{
        res = (str[i + 3] - '0') + res * 10;
        i += 4;
    }while(str[i] != '\0');
    return res;
}

/*!
    \brief Blinkes the error and restarts SIM900

    The function blinks out the number of the error by the LED.
*/
void ErrorHandler(u16 ErrNum){
    u16 i;

    LED_OFF;

    Delay_DelayMs(4000);

    for(i = 0; i < ErrNum; ++i){
        Delay_DelayMs(300);
        LED_ON;
        Delay_DelayMs(300);
        LED_OFF;
    }

    SIM900_ReInit();
}

/*!
    \brief Starts 150ms system timer, enables interrupt uppon timer every 150ms
*/
void SysTimer_Start(){
    TA1CTL = 0x0000; // Hault the timer
    TA1CTL = 0X00C0; // ID = 8 (i.e. /8 divider)
    TA1CTL |= 0X2 << 8; // Use SMCLK as a source clock
    TA1EX0 = 0X7; // Set /8 additional divider
    TA1R = 0x0000; // Zero the timer
    TA1CCR0 = 58650; // Set period 58650 is 150 ms period with SMCLK == 25MHz, IDEX = 8, ID = 8
    TA1CTL |= 0X1 << 4; // Launch the timer in up mode
    TA1CTL |= TAIE;
}

/*!
    \brief TIMER_A1 ISR counts down timeouts in the system
*/
#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER1_A1_ISR(void){
    if(TA1CTL & TAIFG){
        TA1R = 0x0000;
        num_received_bytes = 0;
        if(State.initialization_in_progress){
            LED_TOGGLE;
        }
        State.sys_time++;
        TA1CTL &= ~TAIFG;
    }
}

/*!
    \brief The function sends 1 byte command to a main WaterLeak controller

    The function builds the outgoing packet, calculates the check sum and
    sends all that.
*/
void SendCmd(void){
    // OutPack.DevID = State.controller_address;
    // OutPack.SourceAddress = MY_ADDRESS;
    // OutPack.TID = 0;

    ((u8 *)&OutPack)[OutPack.Length + 1] = CRC_Calc((u8*)&OutPack, OutPack.Length + 1); // Set CRC

    RxTx_RS485_TxEnable;

    __delay_cycles(40000); // 175 = 7us @ 25MHz

    // Send the head of the outgoing packet - address byte
    // MSP430_UART_SendAddress(UART_RS485, OutPack.DevID);

    // Send the rest of the outgoing packet - data bytes
    MSP430_UART_Send(UART_RS485, (u8 *)&OutPack + 1, OutPack.Length + 1);

    __delay_cycles(40000); // 175 = 7us @ 25MHz

    RxTx_RS485_RxEnable;
}

/*!
    \brief Calculates simple control sum via XOR with 0xAA
*/
u8 CRC_Calc(u8* src, u16 num){
    u8 crc = 0xAA;
    while(num--) crc ^= *src++;
    return crc;
}

/*!
    \brief Configures UCS for GSME2 needs

    The function sets DCOCLK and DCOCLKDIV to 25MHz

    Source clock for FFL: REFOCLK 32768Hz
    FLLREFDIV = 1
    FLLN = 762
    DCORSEL = 6
    DCO = 15
    FLLD = 1
*/
void MSP430_UCS_Init(void){
    UCSCTL3 = UCSCTL3 & 0xFF8F | 0x0020; // Set REFOCLK as FLLREFCLK
    UCSCTL2 = UCSCTL2 & 0xFC00 | 762; // Set N = 762
    UCSCTL1 = UCSCTL1 & 0xFF8F | 0x0060; // Set DCORSEL = 6
    UCSCTL1 = UCSCTL1 & 0xE0FF | 0x0F00; // Set DCO = 15
}
