/*******************************************************************************
* File Name: lut_protocols.c
*
* Description:
*  This file contains the protocols to create look up tables
*
**********************************************************************************
 * Copyright Kyle Vitautas Lopin, Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/

#include "lut_protocols.h"
#include "globals.h"
extern char LCD_str[];  // for debug

// local function prototype
uint16 LUT_make_side(uint16 start, uint16 end, uint16 index);


/******************************************************************************
* Function Name: LUT_MakeTriangleWave
*******************************************************************************
*
* Summary:
*  Fill in the look up table (waveform_lut) for the DACs to perform a cyclic voltammetry experiment.
*
* Parameters:
*  uint16 start_value: first value to put in the dac
*  uint16 end_value: the value for the middle of the CV experiment
*
* Return:
*  uint16: how long the look up table is
*
* Global variables:
*  waveform_lut: Array the look up table is stored in
*
*******************************************************************************/

uint16 LUT_MakeTriangleWave(uint16 start_value, uint16 end_value){
    uint16 _lut_index = 0;  // start at the beginning of the lut
    //uint16 lut_length = 2*(end_value - start_value) + 1; // calculate what index is the last look up table entry
    
    LCD_Position(1,0);
    LCD_PrintDecUint16(start_value);
    
    _lut_index = LUT_make_side(dac_ground_value, start_value, 0);
    
    _lut_index = LUT_make_side(start_value, end_value, _lut_index-1);
    _lut_index = LUT_make_side(end_value, dac_ground_value, _lut_index-1);
    waveform_lut[_lut_index] = dac_ground_value;  // the DAC is changed before the value is checked in the isr so it will go 1 over so make it stay at virtual ground
    _lut_index++;
    LCD_PutChar('|');
    LCD_PrintDecUint16(_lut_index);
    LCD_PutChar('|');
    return _lut_index;  
}

/******************************************************************************
* Function Name: LUT_make_side
*******************************************************************************
*
* Summary:
*  Make a ramp from start to end in waveform_lut starting at index
*  Does not matter if start or end is higher
*
* Parameters:
*  uint16 start: first value to put in the look up table
*  uint16 end: end value to put in the look up table
*  uint16 index: the place to start putting in numbers in the look up table
*
* Return:
*  uint16: first place after the filled in area of the look up table
*
* Global variables:
*  waveform_lut: Array the look up table is stored in
*
*******************************************************************************/

uint16 LUT_make_side(uint16 start, uint16 end, uint16 index) {
    if (start < end) {
        for (uint16 value = start; value <= end; value++) {
            waveform_lut[index] = value;
            index ++;
        }
    }
    else {
        for (uint16 value = start; value >= end; value--) {
            waveform_lut[index] = value;
            index ++;
        }
    }
    return index;
}

/******************************************************************************
* Function Name: LUT_MakePulse
*******************************************************************************
*
* Summary:
*  Make a look up table that stores a square pulse sequence
*  Quick hack for chronoamperomerty
*  TODO: THIS SHOULD BE REPLACED
*
* Parameters:
*  uint16 base: value to be placed in the DAC to maintain the baseline potential
*  uint16 pulse: value to put in the DAC for the voltage pulse
*
* Global variables:
*  waveform_lut: Array the look up table is stored in
*
*******************************************************************************/

void LUT_MakePulse(uint16 base, uint16 pulse) {
    int _lut_index = 0;
    while (_lut_index < 1000) {
        waveform_lut[_lut_index] = base;
        _lut_index++;
    }
    while (_lut_index < 2000) {
        waveform_lut[_lut_index] = pulse;
        _lut_index++;
    }
    while (_lut_index < 4000) {
        waveform_lut[_lut_index] = base;
        _lut_index++;
    }
}

/* [] END OF FILE */
