//*****************************************************************************
//
//  Project used to test ADC readings from the 12-bit ADC on the CC3200.
//  ADC max voltage should be 1.467V (tolerant up to 1.8V).
//
//  Pins Used:
//      ADC_CHN_1 -- Pin 58
//
//  Other ADC Pins (Unused), change MACRO for ADC_CHN below to switch
//      ADC_CHN_0 -- Pin 57
//      ADC_CHN_2 -- Pin 59
//      ADC_CHN_3 -- Pin 60
//
//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

// ADC includes
#include "adc.h"
#include "hw_adc.h"

#include "pin_mux_config.h"


//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************

// ADC Values
#define ADC_CHN     ADC_CH_1        // ADC Channel used in this test program
#define ADC_MASK    0x00000FFF      // Used to extract only the ADC value from the CC3200 after bit-shifting down by 2

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                          
//****************************************************************************

/**
  * @brief Reads from ADC Channel
  * @param adcChannel the channel to read from
  * @returns 12 bit ADC value
**/
unsigned long ReadPhoto(unsigned long adcChannel) {

    // Empty out the FIFO (process should be faster than FIFO getting populated)
    while (ADCFIFOLvlGet(ADC_BASE, adcChannel)) {
        ADCFIFORead(ADC_BASE, adcChannel);
    }

    // Wait for FIFO to get populated and then read from it (blocking statement but it should be fast enough for most practices)
    while (ADCFIFOLvlGet(ADC_BASE, adcChannel) == 0) {}
    unsigned long val = ADCFIFORead(ADC_BASE, adcChannel);

    // Return ONLY the ADC value (removing reserved and timestamp bits)
    return (val >> 2) & ADC_MASK;
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Read Photoresistor through CC3200 ADC
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main()
{
    
    // Initialize board configurations
    BoardInit();
    PinMuxConfig();
    
    // Initialize ADC
    MAP_ADCEnable(ADC_BASE);
    MAP_ADCChannelEnable(ADC_BASE, ADC_CHN);
    
    // Read Photresistor ADC periodically
    while (1) {
        printf("photoresistor adc: %d\r\n", ReadPhoto(ADC_CHN));
        MAP_UtilsDelay(20000000);
    }
}


