#ifndef _POWMEAS_H_
#define _POWMEAS_H_

#include "io430.h"
#include "types.h"
#include "delay.h"

// Timeouts ////////////////////////////////////////////////////////////////////
// 1 == 2 ms approx
#define BATMEAS_PER            500

// Samples per 1 measurement
#define NUM_SMPLS_PER_ONE_MEASUREMENT 20

// Voltage levels
#define BATTERY_CRYT_VOLTAGE            466 // 366 = 4.4V 433 = 5.2V 466 = 5.6V
#define BATTERY_HYSTERESIS              8 // 8 = 0.1V
#define EXTERNAL_SUPPLY_CRYT_VOLTAGE    179 // 179 = 4.8V with 1:11 divider
#define EXTERNAL_SUPPLY_HYSTERESIS      10

// Enable battery measurement circuit
#define ENBAT_INIT {P5OUT &= ~BIT2; P5DIR |= BIT2; P5REN &= ~BIT2; P5SEL &= ~BIT2; P5DS &= ~BIT2;}
#define ENBAT_SET (P5OUT |= BIT2)
#define ENBAT_CLR (P5OUT &= ~BIT2)

// LMT84 supply pin
#define LMT84_INIT {P1OUT &= ~BIT0; P1DIR |= BIT0; P1REN &= ~BIT0; P1SEL &= ~BIT0; P1DS &= ~BIT0;}
#define LMT84_ON {P1OUT |= BIT0;}
#define LMT84_OFF {P1OUT &= ~BIT0;}

// External power measurement ADC channel
#define INPWR_INIT {P6OUT &= ~BIT0; P6DIR &= ~BIT0; P6REN &= ~BIT0; P6SEL |= BIT0; P6DS &= ~BIT0;}
#define INPWR_ADC_CH 0

// Battery power measurement  ADC channel
#define INBAT_INIT {P6OUT &= ~BIT1; P6DIR &= ~BIT1; P6REN &= ~BIT1; P6SEL |= BIT1; P6DS &= ~BIT1;}
#define INBAT_ADC_CH 1

// Temperature sensor ADC channel
#define TEMP_INIT {P6OUT &= ~BIT2; P6DIR &= ~BIT2; P6REN &= ~BIT2; P6SEL |= BIT2; P6DS &= ~BIT2;}
#define TEMP_ADC_CH 2

// Constants for convertion ADC value to actual temperature.
#define TEMP_MIN (-30)
#define TEMP_MAX (80)
#define ADC_VALUE_AT_TEMP_MIN (489)
#define ADC_VALUE_AT_TEMP_MAX (242)

void PowMeas_Init(void);
u8 PowMeas_BatteryStatus(void);
u8 PowMeas_ExternSupplyStatus(void);
u16 PowMeas_AdcGet(u8 channel);
s8 PowMeas_GetTemp(void);

#endif
