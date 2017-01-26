#include "loads.h"

/*!
    \brief Initializes controlling outputs and clears them

    Pins definition:
    P5.4 - load 1
    P5.5 - load 2
*/
void Loads_Init(void){
    // load 1
    P5OUT &= ~BIT4;
    P5DIR |=  BIT4;
    P5REN &= ~BIT4;
    P5SEL &= ~BIT4;
    P5DS  &= ~BIT4;

    //load 2
    P5OUT &= ~BIT5;
    P5DIR |=  BIT5;
    P5REN &= ~BIT5;
    P5SEL &= ~BIT5;
    P5DS  &= ~BIT5;
}

/*!
    \brief Drives the controlling pins
    \param[in] cmd May be LOAD1_ON, LOAD1_OFF, LOAD2_ON or LOAD2_OFF
*/
void Loads_Command(enum Load_CmdTypeDef cmd){
    switch(cmd){
        case LOAD1_ON:  P5OUT |=  BIT4; break;
        case LOAD1_OFF: P5OUT &= ~BIT4; break;
        case LOAD2_ON:  P5OUT |=  BIT5; break;
        case LOAD2_OFF: P5OUT &= ~BIT5; break;
    }
}