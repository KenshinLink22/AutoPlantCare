//*****************************************************************************

//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Driverlib includes
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "uart.h"
#include "interrupt.h"
#include "utils.h"
#include "prcm.h"

#include "adc.h"
#include "hw_adc.h"
#include "spi.h"
#include "i2c_if.h"
#include "gpio.h"

// Pin Configs
#include "pin_mux_config.h"

// SSD1351 OLED
#include "SSD1351/Adafruit_SSD1351.h"
#include "SSD1351/Adafruit_GFX.h"
#include "SSD1351/oled_test.h"

//*****************************************************************************
//                          MACROS                                  
//*****************************************************************************

#define SPI_IF_BIT_RATE  1000000

// SSD1351 OLED
#define RstPin  0x40        // Pin 61
#define CSPin   0x80        // Pin 62
#define DCPin   0x1         // Pin 63

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile int g_iCounter = 0;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL DEFINITION                                   
//*****************************************************************************

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************


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

static void SPIInit(void) {
    // Reset SPI
    MAP_SPIReset(GSPI_BASE);
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    // Configure SPI interface
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                         SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                         (SPI_SW_CTRL_CS |
                         SPI_4PIN_MODE |
                         SPI_TURBO_OFF |
                         SPI_CS_ACTIVEHIGH |
                         SPI_WL_8));

    MAP_SPIEnable(GSPI_BASE);
}

//*****************************************************************************
//
//! Main
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

    // Initialize SPI and SSD1351 OLED
    SPIInit();
    Adafruit_Init(DCPin, CSPin, RstPin);

    while (1) {

    }
}

    

