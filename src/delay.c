#include "delay.h"


u8 Delay_Init(void){
    TA0CTL = 0X0000; // Hault the timer
    TA0CTL |= 0X2 << 8; // Use SMCLK as a source clock
    TA0EX0 = 0X5; // Set /5 divider
    TA0R = 0x0000; // Zero the timer
    TA0CCR0 = 5000; // Set period 5000 is 1 ms period with SMCLK == 25MHz and IDEX = 5
    TA0CTL |= 0X1 << 4; // Launch the timer in up mode
    return 0;
}

void Delay_DelayMs(u32 value){
    while(value--){
        TA0R = 0X0000; // Clear the timer
        TA0CTL &= ~TAIFG; // Clear the interrupt flag
        while(!(TA0CTL & TAIFG));
    }
}
