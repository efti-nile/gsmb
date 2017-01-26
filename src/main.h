#include "io430.h"

// Local headers ///////////////////////////////////////////////////////////////

#include "types.h"
#include "msp430_uart.h"
#include "delay.h"
#include "sim900.h"
#include "sms_queue.h"
#include "teldir.h"
#include "powmeas.h"
#include "msp430_flash.h"
#include "loads.h"
#include "ucs2.h"
#include "smspool.h"

// Pins description ////////////////////////////////////////////////////////////

// Indicating LED - P5.3
#define LED_INIT {P5OUT &= BIT3^0xFF; P5DIR |= BIT3; P5REN &= BIT3^0xFF; P5SEL &= BIT3^0xFF; P5DS &= BIT3^0xFF;}

// Communication via RS485 /////////////////////////////////////////////////////

#define MY_ADDRESS 0xC1

#define COMMAND_VALVES_STATE_UNKNOWN 0x00
#define COMMAND_SEND_ALARM           0x01
#define COMMAND_ALL_VALVES_CLOSED    0x02
#define COMMAND_ALL_VALVES_OPENED    0x03
#define COMMAND_CHANGE_BATTERY       0x04

#define RESPONSE_NO_EVENTS 0x00
#define RESPONSE_RESERVED  0x01
#define RESPONSE_CLOSE_ALL 0x02
#define RESPONSE_OPEN_ALL  0x03

#define IN_COMMAND_AVC      (1<<0)
#define IN_COMMAND_AVO      (1<<1)
#define IN_COMMAND_CHB      (1<<2)
#define IN_COMMAND_CHL      (1<<3)
#define IN_COMMAND_LEAK     (1<<4)
#define IN_COMMAND_LEAKR    (1<<5)
#define IN_COMMAND_SEND_SMS (1<<6)

// For debug ///////////////////////////////////////////////////////////////////

#define DBG_PIN_INIT {P6OUT &= BIT0^0xFF; P6DIR |= BIT0; P6REN &= BIT0^0xFF; P6SEL &= BIT0^0xFF; P6DS &= BIT0^0xFF;}
#define DBG_PIN_HIGH P6OUT |= BIT0
#define DBG_PIN_LOW P6OUT &= BIT0^0xFF


// Timeouts ////////////////////////////////////////////////////////////////////
// 1 == 150 ms
#ifndef __DBG__

#define CLOSE_VALVES_TIMEOUT   50
#define OPEN_VALVES_TIMEOUT    50
#define OK_TIMEOUT             50
#define LEAK_FLAG_TIMEOUT      400 // Min. time between two alarm SMSs
#define LINK_LOST_FLAG_TIMEOUT 100

#else

#define CLOSE_VALVES_TIMEOUT   50
#define OPEN_VALVES_TIMEOUT    50
#define OK_TIMEOUT             50
#define LEAK_FLAG_TIMEOUT      30
#define LINK_LOST_FLAG_TIMEOUT 30

#endif

// State variables /////////////////////////////////////////////////////////////

extern struct State_TypeDef State;
extern struct InPack_TypeDef InPack;
extern struct OutPack_TypeDef OutPack;

// Circular buffer for SIM900 communication ////////////////////////////////////

extern u8 CirBuf[CIRBUF_SIZE];
extern u16 CirBuf_Tail;
extern u16 CirBuf_Head;
extern u16 CirBuf_NumBytes;

// Functions prototypes ////////////////////////////////////////////////////////

void MSP430_UCS_Init(void);
void ErrorHandler(u32 ErrNum);
void TIM3_IRQHandler(void);
void SendCmd(void);
u8 CRC_Calc(u8* src, u16 num);
void SIM900_ReInit(void);
void Init(void);
void SysTimer_Start();
__interrupt void TIMER1_A1_ISR(void);
void PowerDisappeared(void);
