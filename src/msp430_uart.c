#include "msp430_uart.h"

u8 CirBuf[CIRBUF_SIZE];
u16 CirBuf_Tail = 0;
u16 CirBuf_Head = 0;
u16 CirBuf_NumBytes = 0;

u8 TelNum[SMS_TELNUM_LEN];

/*!
    \brief Initializes USCI_A0 (for RS486) and USCI_A1 (for SIM900) as UARTs
*/
void MSP430_UART_Init(void){
    // USCI_A0 Initialization //////////////////////////////////////////////////
    //RxTx_RS485_INIT;
    //RxTx_RS485_RxEnable;

    UCA0CTL1 |= UCSWRST; // Hault UART
    
    UCA0CTL1 |= UCSSEL_2; // BRCLK drawn from SMCLK (I.e. UART clocked by SMCLK)

    // UCA0CTL0 |= BIT2; // Use multiprocessor mode with address-bit

    // Set IO for UART operation P3.4 - RXD, P3.3 - TXD
    /*P3OUT &= BIT4^0xFF; P3DIR |= BIT4; P3REN &= BIT4^0xFF;*/ P3SEL |= BIT4; /*P3DS &= BIT4^0xFF;*/ // UCA1TXD
    /*P3OUT &= BIT5^0xFF; P3DIR |= BIT5; P3REN &= BIT5^0xFF;*/ P3SEL |= BIT3; /*P3DS &= BIT5^0xFF;*/ // UCA1RXD

    UCA0BRW = 1314; // 2628 = 9600

    UCA0CTL1 &= ~UCSWRST; // Release UART

    UCA0IE |= UCRXIE;
    //UCA0IE |= UCRXIE | UCTXIE; // Enable interrupts

    // USCI_A1 Initialization //////////////////////////////////////////////////
    UCA1CTL1 |= UCSWRST; // Hault UART

    UCA1CTL1 |= UCSSEL_2; // BRCLK drawn from SMCLK (I.e. UART clocked by SMCLK)

    // Set IO for UART operation P4.4 - TXD, P4.5 - RXD
    /*P4OUT &= BIT4^0xFF; P4DIR |= BIT4; P4REN &= BIT4^0xFF;*/ P4SEL |= BIT4; /*P4DS &= BIT4^0xFF;*/ // UCA1TXD
    /*P4OUT &= BIT5^0xFF; P4DIR |= BIT5; P4REN &= BIT5^0xFF;*/ P4SEL |= BIT5; /*P4DS &= BIT5^0xFF;*/ // UCA1RXD

    UCA1BRW = 219; // Baud rate configuring. 219 for 115200 @ 25MHz BRCLK

    UCA1CTL1 &= ~UCSWRST; // Release UART

    UCA1IE |= UCRXIE;
    //UCA1IE |= UCRXIE | UCTXIE; // Enable interrupts
}

/*!
    \brief Sends address-byte via specified USCI
*/
void MSP430_UART_SendAddress(u8 interface, u8 address){
    switch(interface){
        case UART_RS485: // USCI_A0
            UCA0CTL1 |= UCTXADDR; // To send the next byte as address
            UCA0TXBUF = address;
            while (!(UCA0IFG&UCTXIFG));
            break;
        case UART_SIM900: // USCI_A1 doesn't use the address bytes in this application
            break;
        default:
            break;
    }
}

/*!
    \brief Sends data via specified USCI
*/
void MSP430_UART_Send(u8 interface, u8 *src, u16 num){
    switch(interface){
        case UART_RS485: // USCI_A0
            for(; num != 0; num--){
                UCA0TXBUF = *(src++);
                while (!(UCA0IFG&UCTXIFG));
            }
            break;
        case UART_SIM900: // USCI_A1
            for(; num != 0; num--){
                UCA1TXBUF = *(src++);
                while (!(UCA1IFG&UCTXIFG));
            }
            break;
        default:
            break;
    }
}

/*!
    \brief USCI_A0 ISR (Used for RS485 communication)
*/
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void){
    volatile u8 tmp, if_address;
    static u8 num_received_bytes = 0;
    static u8 kumarin = 1; // DELETE
    switch(__even_in_range(UCA0IV,4))
    {
    case 0: // Vector 0 - no interrupt
        break;
    case 2: // Vector 2 - RXIFG
        // if_address = UCA0STAT & UCADDR;
        tmp = UCA0RXBUF;

        // Save received byte
        ((u8*)&InPack)[num_received_bytes++] = tmp;

        // If we managed to receive all pack's bytes until time-out expired
        if(num_received_bytes >= InPack.Length + 1){
            //volatile static u8 crc = 0;
            //crc = CRC_Calc((u8*)&InPack, InPack.Length + 1);
            if(((u8*)&InPack)[InPack.Length + 1] == CRC_Calc((u8*)&InPack, InPack.Length + 1)){ // Check incoming packet CRC
                State.controller_link_timeout = OK_TIMEOUT; // Update OK timeout
                
                switch(InPack.COMMAND){
                    case GSM_COMMAND_ACK: {
                        OutPack.Length = 2;
                        kumarin++;
                        if(State.request_burner_switch_off){
                            OutPack.COMMAND = GSM_COMMAND_OFF;
                        }else
                        if(State.request_burner_switch_on){
                            OutPack.COMMAND = GSM_COMMAND_ON;
                            kumarin = 1;
                        }else{
                            OutPack.COMMAND = GSM_COMMAND_DONOTHING;
                        }
                        *(((u8 *)(&OutPack)) + OutPack.Length) = 
                            CRC_Calc((u8*)&OutPack, OutPack.Length);
                        MSP430_UART_Send(UART_RS485, (u8 *)(&OutPack), OutPack.Length + 1);
                        break;
                    }
                    case GSM_COMMAND_SMST: {
                        u8 *text = SmsPool_GetPtrForPush(TelDir_NumItems());
                        strToUCS2(text, InPack.Optional);
                        
                        TelDir_Iterator_Init();
                        while(TelDir_GetNextTelNum(TelNum)){
                            SMS_Queue_Push(TelNum, text, SMS_LIFETIME);
                        }
                        break;
                    }
                    case GSM_COMMAND_GOTIT: {
                        State.request_burner_switch_off = State.request_burner_switch_on = 0;
                        break;
                    }                    
                }
            }
            num_received_bytes = 0;
        }
        break;
    case 4: // Vector 4 - TXIFG
        break;
    default:
        break;
    }
}

/*!
    \brief USCI_A1 ISR (Used for SIM900 communication)
*/
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void){
    switch(__even_in_range(UCA1IV,4))
    {
    case 0: // Vector 0 - no interrupt
        break;
    case 2: // Vector 2 - RXIFG
        CirBuf[CirBuf_Head++] = UCA1RXBUF;

        if(CirBuf_Head >= CIRBUF_SIZE){
            CirBuf_Head = 0;
        }

        if(CirBuf_Tail == CirBuf_Head){
            if(++CirBuf_Tail >= CIRBUF_SIZE){
                CirBuf_Tail = 0;
            }
        }

        ++CirBuf_NumBytes;
        break;
    case 4: // Vector 4 - TXIFG
        break;
    default:
        break;
    }
}