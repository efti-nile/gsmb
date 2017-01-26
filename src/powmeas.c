#include "powmeas.h"

extern struct State_TypeDef State;

/*!
    \brief Initializes IO, ADC_10A and REF for power measurement.

    Initializes IO.
    Initializes ADC_10A module. See comments in source fot details.
    Initializes REF module for ADC_10A. See comments in source fot details.

    Initializes PowMeas_PrevBatteryStatus variable with the current battery status.
*/
void PowMeas_Init(void){
    // IO Inintialization
    ENBAT_INIT; // Enable battery measurement circuit
    INBAT_INIT; // Battery power measurement ADC channel
    INPWR_INIT; // External power measurement  ADC channel
    TEMP_INIT;  // Tepmerature sensor input
    LMT84_INIT; // Temperature sensor supply turning off

    // ADC_10A Initialization
    ADC10CTL0 = 0X00; // Switch ADC_10A off
    ADC10CTL0 |= ADC10ON; // Switch ADC_10A on
    ADC10CTL0 |= 0x8 << 8; // Sample time is 256 cycles of ADC10CLK
    ADC10CTL1 |= 0x4 << 5 | 0x3 << 3 | ADC10SHP; // Use SMCLK divided by 5 as ADC clock, use sample timer
    ADC10CTL2 |= ADC10RES; // 10-bit result
    ADC10MCTL0 |= 1 << 4; // Use internal reference as ADC reference
    REFCTL0 |= REFMSTR | 0X3 << 4 | REFON; // Configure internal 2.5V reference
}

/*!
    \brief Returns 1 if battery is OK
*/
u8 PowMeas_BatteryStatus(void){
    // The function actually makes measurement with BATMEAS_PER period
    static u16 cnt = 0;
    u8 retval;

    if(cnt++ > BATMEAS_PER){
        ENBAT_SET;

        __delay_cycles(125); // 125 = 5 us @ 25MHz

        retval = (
                PowMeas_AdcGet(INBAT_ADC_CH)
                 >
                BATTERY_CRYT_VOLTAGE + (State.battery_ok_in_gsm_extender_prev ? -BATTERY_HYSTERESIS : +BATTERY_HYSTERESIS)
        );

        ENBAT_CLR;

        cnt = 0;
    }else{
        retval = State.battery_ok_in_gsm_extender_now;
    }

    return retval;
}

/*!
    \brief Returns 1 if external power supply is OK.
*/
u8 PowMeas_ExternSupplyStatus(void){
    return PowMeas_AdcGet(INPWR_ADC_CH) > EXTERNAL_SUPPLY_CRYT_VOLTAGE +
      (State.ext_supply_ok_prev ? -EXTERNAL_SUPPLY_HYSTERESIS : +EXTERNAL_SUPPLY_HYSTERESIS);
}

/*!
    \brief Returns ADC value of specified channel.
*/
u16 PowMeas_AdcGet(u8 channel){
    u16 s = 0x0000;
    for(u8 i = 0; i < NUM_SMPLS_PER_ONE_MEASUREMENT; ++i){
        ADC10CTL0 &= ~ADC10ENC; // Disable conversion
        ADC10MCTL0 = (ADC10MCTL0 & 0xF0) | (channel & 0x0F); // Set channel
        ADC10CTL0 |= ADC10ENC; // Enable conversion
        ADC10CTL0 |= ADC10SC; // Start the conversion
        while(!(ADC10IFG & ADC10IFG0)); // Wait until conversion completion
        s += ADC10MEM0;

    }
    return s /= NUM_SMPLS_PER_ONE_MEASUREMENT;
}

/*!
    \brief Returns temperature in celcium degree
*/
s8 PowMeas_GetTemp(void){
    u16 t; s8 retval;
    LMT84_ON;
    Delay_DelayMs(2);
    t = PowMeas_AdcGet(TEMP_ADC_CH);
    LMT84_OFF;
    retval = (s8)(TEMP_MIN + ((ADC_VALUE_AT_TEMP_MIN - t) * (TEMP_MAX - TEMP_MIN))/(ADC_VALUE_AT_TEMP_MIN - ADC_VALUE_AT_TEMP_MAX));
    if(retval < TEMP_MIN){
        retval = TEMP_MIN - 1;
    }else if(retval > TEMP_MAX){
        retval = TEMP_MAX + 1;
    }
    return retval;
}
