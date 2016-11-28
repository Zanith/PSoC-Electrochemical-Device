/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#if !defined(LUT_PROTOCOLS_H)
#define LUT_PROTOCOLS_H

#include <project.h>
#include "stdlib.h"  // remove after testing
#include "stdio.h"  // remove after testing

/***************************************
*        Function Prototypes
***************************************/    
uint16 LUT_MakeTriangleWave(uint16 start_value, uint16 end_value);


/***************************************
* Global variables external identifier
***************************************/
extern uint16 waveform_lut[];
    
#endif

/* [] END OF FILE */
