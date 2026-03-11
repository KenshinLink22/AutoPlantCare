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

// Moisture Sensor
#define MSTR_SNSR_HIGH      1023    // Fully Dry in the Air -- Calibration
#define MSTR_SNSR_LOW       230     // Submerged Electrodes -- Calibration
#define MSTR_SNSR_ADDR      0x28    // I2C Address
#define MSTR_SNSR_READ      0x05    // CMD to get ADC Data
#define MSTR_SNSR_LED_ON    0x01    // CMD to turn LED on
#define MSTR_SNSR_LED_OFF   0x00    // CMD to turn LED off

// Water Pump
#define PUMP_BASE   GPIOA0_BASE     // GPIO base for water pump
#define PUMP_PIN    GPIO_PIN_0      // Pin to toggle on/off for water pump

// SSD1351 OLED
#define RstPin  0x40        // Pin 61
#define CSPin   0x80        // Pin 62
#define DCPin   0x1         // Pin 63

// UI
#define TXT_SZ              1
#define TXT_CLR             WHITE
#define BG_CLR              BLACK
#define TRGT_WATER_CURSOR   0
#define TRGT_LIGHT_CURSOR   12
#define ACT_WATER_CURSOR    24
#define ACT_LIGHT_CURSOR    48

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

static const char targetWaterDisp[100] = "Minimum VWC:  %d%%";
static const char targetLightDisp[100] = "Max Light Level: %s";
static const char actualWaterDisp[100] = "Actual VWC: %d%";
static const char actualLightDisp[100] = "Actual Light: %s";

static const char *lightLevels[] = { "Soft", "Ambient", "Moderate", "Bright", "Intense" };

static volatile minVWC;

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL DEFINITION                                   
//*****************************************************************************

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

unsigned int GetVWC(void) {
    unsigned int ADCRange = MSTR_SNSR_HIGH - MSTR_SNSR_LOW;
    unsigned int ADCVal = (int)ReadMoisture();

    return 100 - ((100 * (ADCVal - MSTR_SNSR_LOW)) / (ADCRange));
}

void ModulateMoisture(void) {
    MoistureLEDON();
    unsigned int currVWC = GetVWC();

    if (currVWC < minVWC) {
        // Enable Water Pump if VWC is Below Minimum
        MAP_GPIOPinWrite(PUMP_BASE, PUMP_PIN, PUMP_PIN);
    }

    // Wait for VWC to Pass Minimum if necessary
    while (GetVWC() < minVWC);

    // Ensure Water Pump is Disabled
    MAP_GPIOPinWrite(PUMP_BASE, PUMP_PIN, 0x00);

    MoistureLEDOFF();
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
    minVWC = 12;

    // Initialize board configurations
    BoardInit();
    PinMuxConfig();

    // I2C Init
    I2C_IF_Open(I2C_MASTER_MODE_STD);

    // Initialize SPI and SSD1351 OLED
    SPIInit();
    Adafruit_Init(DCPin, CSPin, RstPin);

    while (1) {

    }
}

    

