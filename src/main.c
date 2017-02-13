#include "main.h"

struct State_TypeDef State;
struct InPack_TypeDef InPack;
struct OutPack_TypeDef OutPack;

int main(void)
{
    WDTCTL = WDTPW + WDTCNTCL; // 1 min 25 s watchdog @ SMCLK 25MHz
    //WDTCTL = WDTHOLD + WDTPW;

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
//    Loads_Init();
    SMS_Queue_Init();
    //PowMeas_Init();
    __bis_SR_register(GIE);
    SysTimer_Start();
    SIM900_ReInit();

//    if(!PowMeas_ExternSupplyStatus()){
//        PowerDisappeared();
//    }

    State.initialization_in_progress = 0;

    while(1){
        if(!SIM900_GetStatus()){
            SIM900_ReInit();
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

        // Check link with main controller
//        State.link_ok_with_main_controller_prev = State.link_ok_with_main_controller_now;
//        State.link_ok_with_main_controller_now = (State.controller_link_timeout > 0);
//        if( State.link_ok_with_main_controller_prev &&
//            !State.link_ok_with_main_controller_now )
//        {
//            u8 TelNum[SMS_TELNUM_LEN];
//            TelDir_Iterator_Init();
//            while(TelDir_GetNextTelNum(TelNum)){
//                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_LINK_LOST_WITH_MAIN_CONTROLLER, SMS_LIFETIME);
//            }
//        }

#ifdef __PWR_CONTROL__
        // Check battery in GSM Extender
        State.battery_ok_in_gsm_extender_prev = State.battery_ok_in_gsm_extender_now;
        State.battery_ok_in_gsm_extender_now = PowMeas_BatteryStatus();
        if( State.battery_ok_in_gsm_extender_prev &&
            !State.battery_ok_in_gsm_extender_now )
        {
            u8 TelNum[SMS_TELNUM_LEN];
            TelDir_Iterator_Init();
            while(TelDir_GetNextTelNum(TelNum)){
                SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_BATTERY_LOW_IN_GSM_EXTENDER, SMS_LIFETIME);
            }
        }

        // Check external supply in GSM Extender
        State.ext_supply_ok_prev = State.ext_supply_ok_now;
        State.ext_supply_ok_now = PowMeas_ExternSupplyStatus();
        if( State.ext_supply_ok_prev &&
            !State.ext_supply_ok_now )
        {
            // Normally MCU is reset when external pwr disappear, so
            // that code is not executed.
            PowerDisappeared();
        }
#endif

        // If open valves timeout elapsed
        if(!State.open_valves_timeout && State.request_open_valves){
            State.request_open_valves = 0;
            SMS_Queue_Push(State.TelNumOfSourceOfRequest, SIM900_SMS_REPORT_OPEN_NOT_ALL, SMS_LIFETIME);
        }

        // If close valves timeout elapsed
        if(!State.close_valves_timeout && State.request_close_valves){
            State.request_close_valves = 0;
            SMS_Queue_Push(State.TelNumOfSourceOfRequest, SIM900_SMS_REPORT_CLOSE_NOT_ALL, SMS_LIFETIME);
        }
    }
}

/*!
    \brief Exetues some action on external power fault

    Sends SMSs for every user in the dictionary that ext. pwr
    disappeared.

    Swithes off the GSM-module

    Waits for ext. pwr resume.
*/
void PowerDisappeared(void){
    u8 TelNum[SMS_TELNUM_LEN];

    TelDir_Iterator_Init();
    while(TelDir_GetNextTelNum(TelNum)){
        SMS_Queue_Push(TelNum, SIM900_SMS_REPORT_EXTERNAL_SUPPLY_LOST, SMS_LIFETIME);
    }

    SIM900_SendSms();

    SIM900_PowerOff();

    __bic_SR_register(GIE);

    LED_OFF;

    while(!PowMeas_ExternSupplyStatus()){
        WDTCTL = WDTPW + WDTCNTCL;
    }

    __bis_SR_register(GIE);

    SIM900_ReInit();
}

/*!
    \brief Blinkes the error and restarts SIM900

    The function blinks out the number of the error by the LED.
*/
void ErrorHandler(u32 ErrNum){
    u32 i;

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

    The timer catches several timeouts.

    1. Close Valves Timeout
    This timeout is set when command to close all valves received and the
    timeout is being expired until acknoledgement of succesful command execution
    gotten.

    2. Open Valves Timeout
    This timeout is set when command to open all valves received and the
    timeout is being expired until acknoledgement of succesful command execution
    gotten.

    3. OK Timeout
    This timeout is reset each time WaterLeak controller riches us. If timeout
    eventually expired that's concerned as the WaterLeak controller lost.

    Etc.
*/
#pragma vector=TIMER1_A1_VECTOR
__interrupt void TIMER1_A1_ISR(void){
    if(TA1CTL & TAIFG){
        TA1R = 0x0000;
        num_received_bytes = 0;
        if(State.initialization_in_progress){
            LED_TOGGLE;
        }
        if(State.close_valves_timeout > 0){
            State.close_valves_timeout--;
        }
        if(State.open_valves_timeout > 0){
            State.open_valves_timeout--;
        }
		if(State.controller_link_timeout > 0){
			State.controller_link_timeout--;
		}
        if(State.leak_flag_timeout > 0){
            State.leak_flag_timeout--;
        }
        if(State.leak_removed_flag_timeout > 0){
            State.leak_removed_flag_timeout--;
        }
        if(State.link_lost_flag_timeout > 0){
            State.link_lost_flag_timeout--;
        }
#ifdef __DBG__
        if(State.leak_now){
            Loads_Command(LOAD1_ON);
        }else{
            Loads_Command(LOAD1_OFF);
        };
        if(State.leak_removed_now){
            Loads_Command(LOAD2_ON);
        }else{
            Loads_Command(LOAD2_OFF);
        };
#endif
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
