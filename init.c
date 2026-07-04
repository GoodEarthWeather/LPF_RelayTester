/*
 * init.c
 *
 *  Created on: June 26, 2019
 *      Author: dmcneill
 *//////
#include "main.h"

//This file contains the routines to initialize everything
// QEX Amplifier Tester

// initialize the clock system
void initClocks(void)
{

    //Initialize external 32.768kHz clock
    CS_setExternalClockSource(32768);
    CS_turnOnXT1LF(CS_XT1_DRIVE_3);

    //Set DCO frequency to 8MHz
    CS_initClockSignal(CS_FLLREF,CS_XT1CLK_SELECT,CS_CLOCK_DIVIDER_1);
    CS_initFLLSettle(8000,244);  // 244*32.768 is approximately 8000kHz = 8MHz
    //Set ACLK = External 32.768kHz clock with frequency divider of 1
    CS_initClockSignal(CS_ACLK,CS_XT1CLK_SELECT,CS_CLOCK_DIVIDER_1);
    //Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);
    //Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK,CS_DCOCLKDIV_SELECT,CS_CLOCK_DIVIDER_1);

    //Clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);
}

// initialize GPIO
void initGPIO(void)
{
  /*
   * Set Pin 2.0, 2.1 to input Primary Module Function, LFXT.
   * This is for configuration of the external 32.768kHz crystal
   */
   GPIO_setAsPeripheralModuleFunctionInputPin(
       GPIO_PORT_P2,
       GPIO_PIN0 + GPIO_PIN1,
       GPIO_PRIMARY_MODULE_FUNCTION
   );


// set GPIO outputs
    GPIO_setAsOutputPin(KEYOUT);
    GPIO_setAsOutputPin(RF_CONTROL);
    GPIO_setAsOutputPin(SW_CONTROL);
    GPIO_setAsOutputPin(TRIGGER);
    GPIO_setAsOutputPin(SW5_CONTROL);
    GPIO_setAsOutputPin(SWR_RESET);


   // set all controls to disable everything
   GPIO_setOutputLowOnPin(KEYOUT);  // keyer out low
   GPIO_setOutputLowOnPin(TRIGGER);  // trigger out low
   GPIO_setOutputLowOnPin(RF_CONTROL);  // disable rf
   GPIO_setOutputLowOnPin(SW_CONTROL);  // disable switching regulator
   GPIO_setOutputLowOnPin(SW5_CONTROL);  // disable switching regulator
   GPIO_setOutputHighOnPin(SWR_RESET);  // enable swr latch

   /*
    * Disable the GPIO power-on default high-impedance mode to activate
    * previously configured port settings
    */
   PMM_unlockLPM5();
}

