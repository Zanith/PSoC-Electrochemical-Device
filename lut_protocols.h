/*******************************************************************************
* File Name: lut_protocols.h
*
* Description:
*  This file contains the function prototypes and constants used for
*  the protocols to create look up tables
*
**********************************************************************************
 * Copyright Kyle Vitautas Lopin, Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/
#if !defined(LUT_PROTOCOLS_H)
#define LUT_PROTOCOLS_H

#include <project.h>
#include "stdlib.h"  // remove after testing
#include "stdio.h"  // remove after testing

/***************************************
*        Function Prototypes
***************************************/    
uint16 LUT_MakeTriangleWave(uint16 start_value, uint16 end_value);
void LUT_MakePulse(uint16 base, uint16 pulse);


/***************************************
* Global variables external identifier
***************************************/
extern uint16 waveform_lut[];
    
#endif

/* [] END OF FILE */
