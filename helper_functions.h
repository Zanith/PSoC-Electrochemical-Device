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

#if !defined(HELPER_FUNCTIONS_H)
#define HELPER_FUNCTIONS_H

#include <project.h>
#include "cytypes.h"
#include "globals.h"
    
/***************************************
*        Variables
***************************************/     
    
uint8 selected_voltage_source;    
    
/***************************************
*        Function Prototypes
***************************************/ 
    
uint8 helper_check_voltage_source(void);
void helper_set_voltage_source(uint8 selected_voltage_source);
    
void helper_Writebyte_EEPROM(uint8 data, uint16 address);
uint8 helper_Readbyte_EEPROM(uint16 address);
void helper_set_dac_value(uint16 value);
void helper_sleep_voltage_source(void);
void helper_wakeup_voltage_source(void);

#endif

/* [] END OF FILE */
