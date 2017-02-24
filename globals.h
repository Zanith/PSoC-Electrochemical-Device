/*******************************************************************************
* File Name: globals.h
*
* Description:
*  Global variables, unions and structures to use in the project
*
**********************************************************************************
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/

#if !defined(GLOBALS_H)
#define GLOBALS_H
    
#include "cytypes.h"


/**************************************
*           API Constants
**************************************/
#define true                        1
#define false                       0
    
#define VIRTUAL_GROUND              2048  // TODO: make variable

// Define the AMux channels
#define two_electrode_config_ch     0
#define three_electrode_config_ch   1 
#define AMux_V_VDAC_source_ch       0
#define AMux_V_DVDAC_source_ch      1
#define AMux_TIA_working_electrode_ch 1

/**************************************
*        EEPROM API Constants
**************************************/

#define VDAC_NOT_SET 0
#define VDAC_IS_VDAC 1
#define VDAC_IS_DVDAC 2
    
#define VDAC_ADDRESS 0
    
    
/**************************************
*        AMuX API Constants
**************************************/
    
#define DVDAC_channel 1
#define VDAC_channel 0
    
    
/**************************************
*        Global Variables
**************************************/   
    
//uint16 dac_ground_value = VIRTUAL_GROUND;  // initialize it to be set for the DVDAC and a 1 mV per step
    
    
#endif    
/* [] END OF FILE */
