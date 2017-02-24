/*******************************************************************************
* File Name: helper_functions.h
*
* Description:
*  Functions to used by main.
TODO:  Move the DAC related APIS to new DAC file 
*
**********************************************************************************
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/

#include "helper_functions.h"

/******************************************************************************
* Function Name: helper_check_voltage_source
*******************************************************************************
*
* Summary:
*  Look in the EEPROM to for what Voltage source is selected
*
* Parameters:
*
* Return:
*  VDAC_NOT_SET [0] - if no voltage source has been selected yet
*  VDAC_IS_VDAC [1] - user indicated no external capacitor is installed, 
*                     so 8-bit VDAC should be used
*  VDAC_IS_DVDAC [2] - user has indicated and external capacitor is installed
*                      so the dithering VDAC (DVDAC) should be set
*
* Global variables:
*  OUT_ENDPOINT:  number that is the endpoint coming out of the computer
*
*******************************************************************************/

uint8 helper_check_voltage_source(void) {
    // start eeprom and read the value at voltage source address
    return helper_Readbyte_EEPROM(VDAC_ADDRESS);
}

/******************************************************************************
* Function Name: helper_set_voltage_source
*******************************************************************************
*
* Summary:
*  Set the voltage source.  Connects the analog mux to the correct channel and 
*  stops the other voltage source if it was on and starts and puts to sleep the current 
*
* Parameters:
*
* Return:
*  VDAC_NOT_SET [0] - if no voltage source has been selected yet
*  VDAC_IS_VDAC [1] - user indicated no external capacitor is installed, 
*                     so 8-bit VDAC should be used
*  VDAC_IS_DVDAC [2] - user has indicated and external capacitor is installed
*                      so the dithering VDAC (DVDAC) should be set
*
* Global variables:
*  OUT_ENDPOINT:  number that is the endpoint coming out of the computer
*
*******************************************************************************/


void helper_set_voltage_source(uint8 voltage_source) {
    selected_voltage_source = voltage_source;
    helper_Writebyte_EEPROM(voltage_source, VDAC_ADDRESS);
    
    if (selected_voltage_source == VDAC_IS_DVDAC) {
        // the DVDAC is being used so set the AMux to the correct channel
        //AMux_V_source_Select(DVDAC_channel);
        VDAC_source_Stop();
//        DVDAC_Start();
//        DVDAC_Sleep();
        LCD_Position(1,0);
        LCD_PrintString("DVDAC AMux");
        
    }
    else {
        //AMux_V_source_Select(VDAC_channel);
        DVDAC_Stop();
//        VDAC_source_Start();
//        VDAC_source_Sleep();
        LCD_Position(1,0);
        LCD_PrintString("VDAC AMux");
    }
    DAC_Start();
    DAC_Sleep();
}



/******************************************************************************
* Function Name: helper_Writebyte_EEPROM
*******************************************************************************
*
* Summary:
*    Start the eepromm, update the temperature and write a byte to it
*
* Parameters:
*     data: the byte to write in
*     address: the address to write the data at
*
* Return:
*
*******************************************************************************/

void helper_Writebyte_EEPROM(uint8 data, uint16 address) {
    EEPROM_Start();
    CyDelayUs(10);
    uint8 blank_hold2 = EEPROM_UpdateTemperature();
    blank_hold2 = EEPROM_WriteByte(data, address);
    EEPROM_Stop();
}

/******************************************************************************
* Function Name: helper_Readbyte_EEPROM
*******************************************************************************
*
* Summary:
*    Start the eepromm, update the temperature and read a byte to it
*
* Parameters:
*     address: the address to read the data from
*
* Return:
*     data that was read
*
*******************************************************************************/

uint8 helper_Readbyte_EEPROM(uint16 address) {
    EEPROM_Start();
    CyDelayUs(10);
    uint8 blank_hold2 = EEPROM_UpdateTemperature();
    CyDelayUs(10);
    uint8 hold = EEPROM_ReadByte(address);
    EEPROM_Stop();
    return hold;
}

/* [] END OF FILE */
