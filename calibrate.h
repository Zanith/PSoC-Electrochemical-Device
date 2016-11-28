/*******************************************************************************
* File Name: calibrate.h
*
* Description:
*  This file contains the function prototypes and constants used for
*  the protocols to calibrate a TIA / delta sigma ADC with an IDAC
*
**********************************************************************************
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/

#if !defined(CALIBRATE_H)
#define CALIBRATE_H

#include "cytypes.h"
    
/**************************************
*      Constants
**************************************/

#define AMux_TIA_calibrat_ch 0
#define AMux_TIA_measure_ch 0    
#define Number_calibration_points 5
  
    
union calibrate_data_usb_union {
    uint8 usb[4*Number_calibration_points];
    int16 data[2*Number_calibration_points];
};
union calibrate_data_usb_union calibrate_array;  // allocate space to put adc measurements
/* This union will save the calibration results and the IDAC values used in the format of
calibrate_data_usb_union = [highest IDAC value, 2nd highest IDAC value, 0 IDAC input, 2nd lowest IDAC value, 
lowest IDAC value, ADC reading for highest IDAC value, ADC reading for 2ndhighest IDAC value,
ADC reading 0 IDAC input, ADC reading for 2nd lowest IDAC value, ADC reading for lowest IDAC value]
*/
    
extern char LCD_str[];


/***************************************
*        Function Prototypes
***************************************/  
void calibrate_TIA(uint8 TIA_resistor_value, uint8 ADC_buffer_index);

#endif
/* [] END OF FILE */
