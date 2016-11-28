/* ========================================
 *
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
 *
 * Protocols to create look up tables
 *
 * ========================================
*/
#include "lut_protocols.h"


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
    int _lut_index = 0;  // start at the beginning of the lut
    uint16 lut_length = 2*(end_value - start_value) + 1; // calculate what index is the last look up table entry

    for (int dacValue = start_value; dacValue <= end_value; dacValue++) {
        waveform_lut[_lut_index] = dacValue;
        _lut_index++;
    }
    for (int dacValue = end_value; dacValue >= start_value; dacValue--) {
        waveform_lut[_lut_index] = dacValue;
        _lut_index++;
    }
    return lut_length;
}

/* [] END OF FILE */
