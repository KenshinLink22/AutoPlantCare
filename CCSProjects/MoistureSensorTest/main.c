//*****************************************************************************
//
//  Project used to test and find calibration values for SparkFun Qwiic Moisture
//  Sensor. Periodically displays ADC readings from the sensor by making I2C
//  requests. Toggles LED on the sensor through I2C commands.
//
//  Pins Used:
//      SCL -- Pin 1
//      SDA -- Pin 2
//      RX  -- Pin 57
//      TX  -- Pin 55
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

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"

#include "pin_mux_config.h"


//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************

// I2C Address + Command Values
#define MSTR_SNSR_ADDR      0x28    // I2C Address
#define MSTR_SNSR_READ      0x05    // CMD to get ADC Data
#define MSTR_SNSR_LED_ON    0x01    // CMD to turn LED on
#define MSTR_SNSR_LED_OFF   0x00    // CMD to turn LED off

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
  * @brief Requests and Reads ADC Values from QWIIC Moisture Sensor
  * @returns two-byte ADC value
**/
uint16_t ReadMoisture(void) {
    unsigned char readCMD = (unsigned char)MSTR_SNSR_READ;
    unsigned char buffer[2];
    uint16_t readVal;

    I2C_IF_Write(MSTR_SNSR_ADDR, &readCMD, 1, 1);       // Tell moisture sensor that data will be read
    MAP_UtilsDelay(20000);   // small delay
    I2C_IF_Read(MSTR_SNSR_ADDR, buffer, 2);      // Read 2 bytes from the moisture sensor

    readVal = buffer[1];    // Read upper bits
    readVal <<= 8;          // Shift upper bits
    readVal |= buffer[0];   // Add lower bits

    return readVal;
}

/**
  * @brief Turns the QWIIC Moisture Sensor LED On
**/
void MoistureLEDON(void) {
    unsigned char LEDCMD = (unsigned char)MSTR_SNSR_LED_ON;
    I2C_IF_Write(MSTR_SNSR_ADDR, &LEDCMD, 1, 1);
}

/**
  * @brief Turns the QWIIC Moisture Sensor LED Off
**/
void MoistureLEDOFF(void) {
    unsigned char LEDCMD = (unsigned char)MSTR_SNSR_LED_OFF;
    I2C_IF_Write(MSTR_SNSR_ADDR, &LEDCMD, 1, 1);
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
//! Main function handling the I2C example
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
    
    // Configuring UART
    InitTerm();
    
    // I2C Init
    I2C_IF_Open(I2C_MASTER_MODE_STD);

    // Read Moisture Level ADC and Toggle LED on Sensor
    while (1) {
        Report("moisture adc: %d\r\n", ReadMoisture());
        MoistureLEDON();
        MAP_UtilsDelay(20000000);
        MoistureLEDOFF();
        MAP_UtilsDelay(20000000);
    }
}


