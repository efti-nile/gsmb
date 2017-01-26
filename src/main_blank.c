
#include "io430.h"

int main( void )
{
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW + WDTHOLD;

    P4OUT = 0x00;
    P4DIR = BIT5 | BIT4;
	P4REN = 0x00;
	P4SEL = 0x00;
	P4DS  = 0x00;

    while(1){
        for(int i = 0; i < 10000; i++);
        P4OUT |= BIT5;
        for(int i = 0; i < 10000; i++);
        P4OUT &= BIT5^0xFF;
    }

    return 0;
}
