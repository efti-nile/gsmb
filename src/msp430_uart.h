#ifndef _MSP430_UART_H_
#define _MSP430_UART_H_

#include "io430.h"
#include "types.h"
#include "main.h"

#define UART_RS485 0 // USCI_A0
#define UART_SIM900 1 // USCI_A1

#define GSM_COMMAND_ACK           1
#define GSM_COMMAND_SMST          2
#define GSM_COMMAND_GOTIT         3

#define GSM_COMMAND_OFF           255
#define GSM_COMMAND_ON            254
#define GSM_COMMAND_SET_TDAY      253
#define GSM_COMMAND_SET_TNIGHT    252
#define GSM_COMMAND_NIGHT_ON      251
#define GSM_COMMAND_NIGHT_OFF     250
#define GSM_COMMAND_NIGHT_SET     249
#define GSM_COMMAND_SEN_GET       248
//#define GSM_COMMAND_SETTINGS_GET  247
#define GSM_COMMAND_TARGET_T_GET  246
#define GSM_COMMAND_DONOTHING     245

// R\E\ RS485 - P3.2
#define RxTx_RS485_INIT {P3OUT &= ~BIT2; P3DIR |= BIT2; P3REN &= ~BIT2; P3SEL &= ~BIT2; P3DS &= ~BIT2;}
#define RxTx_RS485_RxEnable P3OUT &= ~BIT2
#define RxTx_RS485_TxEnable P3OUT |= BIT2

// Circular buffer for USART connected to SIM900 /////////////////////

void MSP430_UART_Init(void);
void MSP430_UART_SendAddress(u8 interface, u8 address);
void MSP430_UART_Send(u8 interface, u8 *src, u16 num);
__interrupt void USCI_A0_ISR(void);
__interrupt void USCI_A1_ISR(void);

#endif
