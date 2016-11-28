/*********************************************************************************
* File Name: main.c
* Version 3.20
*
* Description:
*  Main program to use PSoC 5LP as an electrochemcial device
*
* TODO: Add nore details
*
**********************************************************************************
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/

#include <project.h>
#include "stdio.h"
#include "stdlib.h"
// local files
#include "calibrate.h"
#include "globals.h"
#include "helper_functions.h"
#include "lut_protocols.h"
#include "USB_protocols.h"

// define how big to make the arrays for the lut for dac and how big
// to make the adc data array 
#define MAX_LUT_SIZE 4000
#define ADC_channels 4

#define Work_electrode_resistance 1400  // ohms, estimate of resistance from SC block to the working electrode pin

struct TIAMux {
    uint8 use_extra_resistor;
    uint8 user_channel;
};
struct TIAMux tia_mux = {.use_extra_resistor = false, .user_channel = 0};

union data_usb_union {
    uint8 usb[2*MAX_LUT_SIZE];
    int16 data[MAX_LUT_SIZE];
};
union data_usb_union ADC_array[ADC_channels];  // allocate space to put adc measurements

/* make buffers for the USB ENDPOINTS */
uint8 IN_Data_Buffer[MAX_NUM_BYTES];
uint8 OUT_Data_Buffer[MAX_NUM_BYTES];

char LCD_str[32];  // buffer for LCD screen, make it extra big to avoid overflow

uint8 TIA_resistor_value;  // keep track of what resistor the TIA is using
uint8 ADC_buffer_index = 0;
uint8 Input_Flag = false;  // if there is an input, set this flag to process it
uint8 AMux_channel_select = 0;  // Let the user choose to use the two electrode configuration (set to 0) or a three
// electrode configuration (set to 1) by choosing the correct AMux channel

/* Make global variables needed for the DAC/ADC interrupt service routines */
uint16 timer_period;
uint16 waveform_lut[MAX_LUT_SIZE];  // look up table for waveform values to put into msb DAC
int16 lut_index = 0;  // look up table index
uint16 lut_value;  // value need to load DAC
uint16 lut_length = 3000;  // how long the look up table is,initialize large so when starting isr the ending doesn't get triggered

/* function prototypes */
void HardwareSetup(void);
void HardwareStart(void);
void HardwareSleep(void);
void HardwareWakeup(void);
uint16 Convert2Dec(uint8 array[], uint8 len);

CY_ISR(dacInterrupt)
{
    LCD_Position(0,0);
    sprintf(LCD_str, "lut:%d|%d|", lut_index, lut_length);
    LCD_PrintString(LCD_str);
    helper_set_dac_value(lut_value);
    lut_index++;
    if (lut_index > lut_length) { // all the data points have been given
        isr_adc_Disable();
        isr_dac_Disable();
        ADC_array[0].data[lut_index] = 0xC000;  // mark that the data array is done
        HardwareSleep();
        lut_index = 0; 
        USB_Export_Data((uint8*)"Done", 5); // calls a function in an isr but only after the current isr has been disabled
    }
    lut_value = waveform_lut[lut_index];
}
CY_ISR(adcInterrupt){
    ADC_array[0].data[lut_index] = ADC_SigDel_GetResult16(); 
    //ADC_array[0].data[lut_index] = 256*lut_index+lut_value;
    // calculate voltage error to add to VDAC on dac interrupt
}

int main()
{
    /* Initialize all the hardware and interrupts */
    CyGlobalIntEnable; 
    LCD_Start();
    
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);  // initialize the USB
    HardwareSetup();
    while(!USBFS_bGetConfiguration());  //Wait till it the usb gets its configuration from the PC ??
    
    isr_dac_StartEx(dacInterrupt);
    isr_dac_Disable();  // disable interrupt until a voltage signal needs to be given
    isr_adc_StartEx(adcInterrupt);
    isr_adc_Disable();
    
    USBFS_EnableOutEP(OUT_ENDPOINT);  // changed
    LCD_ClearDisplay();
    LCD_Position(0,0);
    LCD_PrintString("amp build4b");
    
    // set the voltage source
    
    
    for(;;) {
        Input_Flag = USB_CheckInput(OUT_Data_Buffer);  /* check if there is a response from the computer */
        if(USBFS_IsConfigurationChanged()) {  // check if the USb coniguration has changed, can be cause by a program reattaching to the device
            while(!USBFS_GetConfiguration());

            USBFS_EnableOutEP(OUT_ENDPOINT);
        }     

        if (Input_Flag == true) {
            switch (OUT_Data_Buffer[0]) { 
                
            case 'E': ; // User wants to export the data, the user can choose what ADC array to export
                uint8 user_ch = OUT_Data_Buffer[1]-'0';
                //LCD_Position(0,0);
                //sprintf(LCD_str, "export:%d  |", lut_length);
                //LCD_PrintString(LCD_str);
                
                if (user_ch <= ADC_channels) { // check for buffer overflow
                    // 2*(lut_length+1) because the data is 2 times as long as it has to 
                    // be sent as 8-bits and the data is 16 bit, +1 is for the 0xFFFF finished signal
                    USB_Export_Data(&ADC_array[user_ch].usb[0], 2*(lut_length+2));  
                }
                else {
                    USB_Export_Data((uint8*)"Error Exporting", 16);
                }
                //LCD_Position(1,0);
                //sprintf(LCD_str, "lut 1: %d ", waveform_lut[0]);
                //LCD_PrintString(LCD_str);
                break;
            case 'B': ; // calibrate the TIA / ADC current measuring circuit
                calibrate_TIA(TIA_resistor_value, ADC_buffer_index);
                
            case 'C': ;  // change the compare value of the PWM to start the adc isr
                uint16 CMP = Convert2Dec(&OUT_Data_Buffer[2], 5);
                PWM_isr_WriteCompare(CMP);
                break;
            case 'A': ; // input a 0-7 string into the device that will change the TIA resistor
                // input is AX|Y|Z|W: where X is the TIA resistor value, Y is the adc buffer gain setting
                // Z is T or F for if an external resistor is to be used and the AMux_working_electrode should be set according
                // W is 0 or 1 for which user resistor should be selected by AMux_working_electrode
                TIA_resistor_value = OUT_Data_Buffer[1]-'0';
                TIA_SetResFB(TIA_resistor_value);  // see TIA.h for how the values work, basically 0 - 20k, 1 -30k, etc.
                ADC_buffer_index = OUT_Data_Buffer[3]-'0';
                ADC_SigDel_SetBufferGain(ADC_buffer_index);
                if (OUT_Data_Buffer[5] == 'T') {
                    tia_mux.use_extra_resistor = true;
                    tia_mux.user_channel = OUT_Data_Buffer[7]-'0';
                    OUT_Data_Buffer[5] = 0;
                }
                break;
            case 'V': ;  // check if the device should use the dithering VDAC of the VDAC
                uint8 export_array[2];
                export_array[0] = 'V';
                if (OUT_Data_Buffer[1] == 'R') {  // User wants to read status of VDAC
                    export_array[1] = helper_check_voltage_source();
                    USB_Export_Data(export_array, 2);
                }
                else if (OUT_Data_Buffer[1] == 'S') {  // User wants to set the voltage source
                    helper_set_voltage_source(OUT_Data_Buffer[2]-'0');
                }
                break;
            case 'R': ;
                if (!isr_dac_GetState()){  // enable the dac isr if it isnt already enabled
                    /* set the electrode voltage to the lowest voltage first, then start ADC and wait for it to initiate
                    before starting the interrupts  */
                    HardwareWakeup();  // start the hardware
                    LCD_Position(0,0);
                    LCD_PrintString("Running");
                    CyDelay(1);
                    VDAC_source_SetValue(lut_value);
                    CyDelay(1);  // let the electrode voltage settle
                    ADC_SigDel_StartConvert();  // start the converstion process of the delta sigma adc so it will be ready to read when needed
                    CyDelay(1);
                    PWM_isr_WriteCounter(100);  // set the pwm timer so that it will trigger adc isr first
                    //ADC_array[0].data[lut_index] = 256*lut_index+lut_value;
                    ADC_array[0].data[lut_index] = ADC_SigDel_GetResult16();  // Hack, get first adc reading, timing element doesn't reverse for some reason
                    isr_dac_Enable();  // enable the interrupts to start the dac
                    isr_adc_Enable();  // and the adc
                    LCD_Position(1,0);
                    LCD_PrintString("endRunning");
                }
                else {
                    USB_Export_Data((uint8*)"Error1", 7);
                }
                break;
            case 'X': ; // reset the device by disabbleing isrs
                isr_dac_Disable();
                isr_adc_Disable();
                lut_index = 0;  
                break;
            case 'I': ;  // identify the device and test if the usb is working properly
                USB_Export_Data((uint8*)"USB Test - v04", 15);
                break;
            case 'L': ; // User wants to change the electrode configuration
                AMux_channel_select = Convert2Dec(&OUT_Data_Buffer[2], 1) - 2; // user sends 2 or 3 for the # electrode 
                //config, map this to 0 or 1 for the channel the AMux should select
                AMux_electrode_Select(AMux_channel_select);
                break;
            case 'S': ; // getting the starting bit of the dacs, use ; after : to make an empty statement to prevent error
                /*Get the string of data that contains the start and end voltges of the
                triangle wave and produce the look up table */
                
                uint16 low_amplitude = Convert2Dec(&OUT_Data_Buffer[2], 4);
                uint16 high_amplitude = Convert2Dec(&OUT_Data_Buffer[7], 4);
                timer_period = Convert2Dec(&OUT_Data_Buffer[12], 5);
                PWM_isr_WritePeriod(timer_period);
                
                lut_length = LUT_MakeTriangleWave(low_amplitude, high_amplitude); 
                lut_value = waveform_lut[0];  // Initialize for the start of the experiment
                break; 
            }  // end of switch statment
            OUT_Data_Buffer[0] = '0';  // clear data buffer cause it has been processed
            Input_Flag = false;  // turn off input flag because it has been processed
        }
    }  // end of for loop in main
    
}  // end of main

void HardwareSetup(void) {
    HardwareStart();
    HardwareSleep();
    LCD_Start();
    Timer_Start();
    VDAC_source_Start();
    VDAC_source_Sleep();
    DVDAC_Start();
    DVDAC_Sleep();
    // start all the analog muxes
    AMux_electrode_Init();
    AMux_TIA_input_Init();
    AMux_TIA_resistor_bypass_Init();
    AMux_V_source_Init();
    
    /* iniatalize the analog muxes connections all the analog muxes to run the VDAC and record the current with and no extra TIA resistor */
    AMux_electrode_Select(two_electrode_config_ch);  // start with 2 electrode configuration
    AMux_TIA_input_Select(AMux_TIA_working_electrode_ch);  // Connect the working electrode
    // Keep AMuX_TIA_resistor_bypass disconnected
    AMux_V_source_Select(AMux_V_VDAC_source_ch);  // connect the 8 bit VDAC to the opamp driving the electrode
}

void HardwareStart(void){  // start all the components that have to be on for a reading
    ADC_SigDel_Start();
    TIA_Start();
    VDAC_TIA_Start();
    Opamp_Aux_Start();
    PWM_isr_Start();
}

void HardwareWakeup(void){  // wakeup all the components that have to be on for a reading
    ADC_SigDel_Wakeup();
    TIA_Wakeup();
    VDAC_TIA_Wakeup();
    Opamp_Aux_Wakeup();
    PWM_isr_Wakeup();
    helper_wakeup_voltage_source();
}

void HardwareSleep(void){  // put to sleep all the components that have to be on for a reading
    ADC_SigDel_Sleep();
    TIA_Sleep();
    VDAC_TIA_Sleep();
    Opamp_Aux_Sleep();
    PWM_isr_Sleep();
    helper_sleep_voltage_source();
}

uint16 Convert2Dec(uint8 array[], uint8 len){
    uint16 num = 0;
    for (int i = 0; i < len; i++){
        num = num * 10 + (array[i] - '0');
    }
    return num;
}

/* [] END OF FILE */
