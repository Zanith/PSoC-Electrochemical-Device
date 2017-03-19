/*********************************************************************************
* File Name: main.c
* Version 0.4
*
* Description:
*  Main program to use PSoC 5LP as an electrochemcial device
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
#include "DAC.h"
#include "globals.h"
#include "helper_functions.h"
#include "lut_protocols.h"
#include "USB_protocols.h"

// define how big to make the arrays for the lut for dac and how big
// to make the adc data array 
#define MAX_LUT_SIZE 5000
#define ADC_CHANNELS 4

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
union data_usb_union ADC_array[ADC_CHANNELS];  // allocate space to put adc measurements

union small_data_usb_union {
    uint8 usb[64];
    int16 data[32];
};
union small_data_usb_union amp_union;

/* make buffers for the USB ENDPOINTS */
uint8 IN_Data_Buffer[MAX_NUM_BYTES];
uint8 OUT_Data_Buffer[MAX_NUM_BYTES];

char LCD_str[32];  // buffer for LCD screen, make it extra big to avoid overflow
char usb_str[64];  // buffer for string to send to the usb

uint8 TIA_resistor_value;  // keep track of what resistor the TIA is using
uint8 ADC_buffer_index = 0;
uint8 Input_Flag = false;  // if there is an input, set this flag to process it
uint8 AMux_channel_select = 0;  // Let the user choose to use the two electrode configuration (set to 0) or a three
// electrode configuration (set to 1) by choosing the correct AMux channel

/* Make global variables needed for the DAC/ADC interrupt service routines */
uint16 timer_period;
uint16 waveform_lut[MAX_LUT_SIZE];  // look up table for waveform values to put into msb DAC
uint16 lut_index = 0;  // look up table index
uint8 adc_recording_channel = 0;
uint16 lut_value;  // value need to load DAC
uint16 lut_length = 3000;  // how long the look up table is,initialize large so when starting isr the ending doesn't get triggered
uint16 lut_hold = 0;
uint8 adc_hold;
uint8 counter = 0;
uint16 buffer_size_bytes;
uint16 buffer_size_data_pts = 4000;  // prevent the isr from firing
uint16 dac_value_hold = 0;


/* function prototypes */
void HardwareSetup(void);
void HardwareStart(void);
void HardwareSleep(void);
void HardwareWakeup(void);
uint16 Convert2Dec(uint8 array[], uint8 len);

CY_ISR(dacInterrupt)
{
    
    DAC_SetValue(lut_value);
    lut_index++;
    dac_value_hold = lut_value;
    if (lut_index >= lut_length) { // all the data points have been given
        isr_adc_Disable();
        isr_dac_Disable();
        //LCD_Position(1,0);
        //sprintf(LCD_str, "e2:%d|%d", lut_index, lut_length);
        //LCD_PrintString(LCD_str);
        ADC_array[0].data[lut_index] = 0xC000;  // mark that the data array is done
        HardwareSleep();
        lut_index = 0; 
        USB_Export_Data((uint8*)"Done", 5); // calls a function in an isr but only after the current isr has been disabled

    }
    lut_value = waveform_lut[lut_index];
}
CY_ISR(adcInterrupt){
    //ADC_array[0].data[lut_index] = ADC_SigDel_GetResult16(); 
    //ADC_array[0].data[lut_index] = lut_value;
    ADC_array[0].data[lut_index] = dac_value_hold;
}

CY_ISR(adcAmpInterrupt){
    ADC_array[adc_recording_channel].data[lut_index] = ADC_SigDel_GetResult16(); 
    lut_index++;  
    if (lut_index >= buffer_size_data_pts) {
        ADC_array[adc_recording_channel].data[lut_index] = 0xC000;
        counter += 1;
        lut_index = 0;
        adc_hold = adc_recording_channel;
        adc_recording_channel = (adc_recording_channel + 1) % ADC_CHANNELS;
        
        sprintf(usb_str, "Done%d", adc_hold);  // tell the user the data is ready to pick up and which channel its on
        USB_Export_Data((uint8*)usb_str, 6);  // use the 'F' command to retreive the data
    }
}

int main()
{
    /* Initialize all the hardware and interrupts */
    CyGlobalIntEnable; 
    LCD_Start();
    LCD_ClearDisplay();
    
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);  // initialize the USB
    HardwareSetup();
    while(!USBFS_bGetConfiguration());  //Wait till it the usb gets its configuration from the PC ??
    
    isr_dac_StartEx(dacInterrupt);
    isr_dac_Disable();  // disable interrupt until a voltage signal needs to be given
    isr_adc_StartEx(adcInterrupt);
    isr_adc_Disable();
    
    USBFS_EnableOutEP(OUT_ENDPOINT);  // changed
    isr_adcAmp_StartEx(adcAmpInterrupt);
    isr_adcAmp_Disable();
    
    LCD_Position(0,0);
    LCD_PrintString("amp build4b||");
    helper_Writebyte_EEPROM(0, VDAC_ADDRESS);
    
    for(;;) {
        
        if(USBFS_IsConfigurationChanged()) {  // if the configuration is changed reenable the OUT ENDPOINT
            while(!USBFS_GetConfiguration()) {  // wait for the configuration with windows / controller is updated
            }
            USBFS_EnableOutEP(OUT_ENDPOINT);  // reenable OUT ENDPOINT
        }
        if (Input_Flag == false) {  // make sure any input has already been dealt with
            Input_Flag = USB_CheckInput(OUT_Data_Buffer);  // check if there is a response from the computer
        }
        
        if (Input_Flag == true) {
            switch (OUT_Data_Buffer[0]) { 
                
            case 'F': ; // User wants to export streaming data
                LCD_Position(1,0);
                LCD_PrintString(";lll");
                
                uint8 user_ch1 = OUT_Data_Buffer[1]-'0';
                LCD_Position(1,0);
                sprintf(LCD_str, "get:%d | ", user_ch1);
                LCD_PrintString(LCD_str);
                USB_Export_Data(&ADC_array[user_ch1].usb[0], buffer_size_bytes); 
                break;
                
            case 'E': ; // User wants to export the data, the user can choose what ADC array to export
                uint8 user_ch = OUT_Data_Buffer[1]-'0';
                if (user_ch <= ADC_CHANNELS) { // check for buffer overflow
                    // 2*(lut_length+2) because the data is 2 times as long as it has to 
                    // be sent as 8-bits and the data is 16 bit, +1 is for the 0xC000 finished signal
                    //LCD_Position(1,0);
                    //sprintf(LCD_str, "Edf:%d", 2*(lut_length+2));
                    //LCD_PrintString(LCD_str);
                    USB_Export_Data(&ADC_array[user_ch].usb[0], 2*(lut_length+1));  
                    
                }
                else {
                    USB_Export_Data((uint8*)"Error Exporting", 16);
                }
                break;
            case 'B': ; // calibrate the TIA / ADC current measuring circuit
                calibrate_TIA(TIA_resistor_value, ADC_buffer_index);
                LCD_Position(0,0);
                LCD_PrintString("Done cal");
                break;
            case 'C': ;  // change the compare value of the PWM to start the adc isr
                uint16 CMP = Convert2Dec(&OUT_Data_Buffer[2], 5);
                //LCD_Position(0,0);
                //sprintf(LCD_str, "comp:%d | ", CMP);
                //LCD_PrintString(LCD_str);
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
                    tia_mux.user_channel = OUT_Data_Buffer[7]-'0';  // not used yet
                    OUT_Data_Buffer[5] = 0;
                    AMux_TIA_resistor_bypass_Connect(0);
                }
                else {
                    AMux_TIA_resistor_bypass_Disconnect(0);
                }
                break;
            case 'V': ;  // check if the device should use the dithering VDAC of the VDAC
                
                if (OUT_Data_Buffer[1] == 'R') {  // User wants to read status of VDAC
                    uint8 export_array[2];
                    export_array[0] = 'V';
                    export_array[1] = helper_check_voltage_source();
                    USB_Export_Data(export_array, 2);
                }
                else if (OUT_Data_Buffer[1] == 'S') {  // User wants to set the voltage source
                    helper_set_voltage_source(OUT_Data_Buffer[2]-'0');
                    DAC_Start();
                    DAC_Sleep();
                }
                break;
            case 'R': ;  // Start a cyclic voltammetry experiment
                if (!isr_dac_GetState()){  // enable the dac isr if it isnt already enabled
                    if (isr_adcAmp_GetState()) {  // User has started cyclic voltammetry while amp is already running so disable amperometry
                        isr_adcAmp_Disable();
                    }
                    LCD_Position(0,0);
                    LCD_PrintString("Cyclic volt running");
                    lut_index = 0;
                    lut_value = waveform_lut[0];
                    HardwareWakeup();  // start the hardware
                    DAC_SetValue(lut_value);  // TODO:  Fix this is a mess
                    //lut_index = 1;
                    //lut_value = waveform_lut[1];
                    CyDelay(1);  // let the electrode voltage settle
                    ADC_SigDel_StartConvert();  // start the converstion process of the delta sigma adc so it will be ready to read when needed
                    CyDelay(5);
                    PWM_isr_WriteCounter(100);  // set the pwm timer so that it will trigger adc isr first
                    
                    ADC_array[0].data[lut_index] = ADC_SigDel_GetResult16();  // Hack, get first adc reading, timing element doesn't reverse for some reason
                    isr_dac_Enable();  // enable the interrupts to start the dac
                    isr_adc_Enable();  // and the adc
                }
                else {
                    USB_Export_Data((uint8*)"Error1", 7);
                }
                break;
            case 'X': ; // reset the device by disabbleing isrs
                isr_dac_Disable();
                isr_adc_Disable();
                isr_adcAmp_Disable();
                LCD_Position(0,0);
                LCD_PrintString("not recording");
                lut_index = 0;  
                break;
            case 'I': ;  // identify the device and test if the usb is working properly, this is what the program sends at the beginning, so disable all interrupts
                // incase the program has restarted so the device will also reset
                isr_adcAmp_Disable();
                isr_adc_Disable();
                isr_dac_Disable();
                USB_Export_Data((uint8*)"USB Test - v04", 15);
                //LCD_Position(0,0);
                //LCD_PrintString("Got I");
                // TODO:  Put in a software reset incase something goes wrong the program can reattach
                break;
            case 'L': ; // User wants to change the electrode configuration
                AMux_channel_select = Convert2Dec(&OUT_Data_Buffer[2], 1) - 2; // user sends 2 or 3 for the # electrode 
                //config, map this to 0 or 1 for the channel the AMux should select
                AMux_electrode_Select(AMux_channel_select);
                break;
            case 'T': ; //Set the PWM timer period
                PWM_isr_Wakeup();
                timer_period = Convert2Dec(&OUT_Data_Buffer[2], 5);
                PWM_isr_WriteCompare(timer_period / 2);  // not used in amperometry run so just set in the middle
                PWM_isr_WritePeriod(timer_period);
                PWM_isr_Sleep();
                
                LCD_Position(1,0);
                sprintf(LCD_str, "PWM:%d", PWM_isr_ReadPeriod());
                LCD_PrintString(LCD_str);
                
                break;
            case 'Q': ;  // Hack to let the device to run a chronoamperometry experiment, not working properly yet
                PWM_isr_Wakeup();
                uint16 baseline = Convert2Dec(&OUT_Data_Buffer[2], 4);
                uint16 pulse = Convert2Dec(&OUT_Data_Buffer[7], 4);
                timer_period = Convert2Dec(&OUT_Data_Buffer[12], 5);
                
                PWM_isr_WritePeriod(timer_period);
                
                lut_length = 4000;
                LUT_MakePulse(baseline, pulse);
                lut_value = waveform_lut[0];
                
                PWM_isr_Sleep();
                break;
            case 'S': ; // make a look up table (lut) for a cyclic voltammetry experiment
                PWM_isr_Wakeup();
                uint16 low_amplitude = Convert2Dec(&OUT_Data_Buffer[2], 4);
                uint16 high_amplitude = Convert2Dec(&OUT_Data_Buffer[7], 4);
                timer_period = Convert2Dec(&OUT_Data_Buffer[12], 5);
                
                PWM_isr_WritePeriod(timer_period);

                lut_length = LUT_MakeTriangleWave(low_amplitude, high_amplitude); 
                lut_value = waveform_lut[0];  // Initialize for the start of the experiment
                PWM_isr_Sleep();
                break; 
            case 'D': ; // set the dac value
                uint16 dac_value1 = Convert2Dec(&OUT_Data_Buffer[2], 4);
                DAC_SetValue(dac_value1);
                break;
                
            case 'M': ; // run an amperometric experiment
                LCD_Position(0,0);
                LCD_PrintString("Ampmtry running");
                HardwareWakeup();
                if (!isr_adcAmp_GetState()) {  // enable isr if it is not already
                    if (isr_dac_GetState()) {  // User selected to run amperometry but a CV is still running 
                        isr_dac_Disable();
                        isr_adc_Disable();
                    }
                }
                uint16 dac_value = Convert2Dec(&OUT_Data_Buffer[2], 4);  // get the voltage the user wants and set the dac
                lut_index = 0;
                DAC_SetValue(dac_value);
                
                ADC_SigDel_StartConvert();
                CyDelay(5);
                
                buffer_size_data_pts = Convert2Dec(&OUT_Data_Buffer[7], 4);  // how many data points to collect in each adc channel before exporting the data
                buffer_size_bytes = 2*(buffer_size_data_pts + 1); // add 1 bit for the termination code and double size for bytes from uint16 data
                adc_recording_channel = 0;
                 
                CyDelay(10);
                isr_adcAmp_Enable();
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
    Timer_Start();
    DAC_Start();
    // start all the analog muxes
    AMux_electrode_Init();
    AMux_TIA_input_Init();
    AMux_TIA_resistor_bypass_Init();
    AMux_V_source_Init();
    DAC_Start();  // DAC has to be started after the AMux_V_source because it will set it based what DAC source is selected
    
    /* iniatalize the analog muxes connections all the analog muxes to run the VDAC and record the current with and no extra TIA resistor */
    AMux_electrode_Select(two_electrode_config_ch);  // start with 2 electrode configuration
    AMux_TIA_input_Select(AMux_TIA_working_electrode_ch);  // Connect the working electrode
    AMux_TIA_resistor_bypass_Init();
    AMux_TIA_resistor_bypass_Select(0);

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
    DAC_Wakeup();
    CyDelay(1);
    DAC_SetValue(lut_value);
    CyDelay(10);
    Opamp_Aux_Wakeup();
    
    PWM_isr_Wakeup();
    
}

void HardwareSleep(void){  // put to sleep all the components that have to be on for a reading
    ADC_SigDel_Sleep();
    DAC_Sleep();
    TIA_Sleep();
    VDAC_TIA_Sleep();
    Opamp_Aux_Sleep();
    PWM_isr_Sleep();
    
}

uint16 Convert2Dec(uint8 array[], uint8 len){
    uint16 num = 0;
    for (int i = 0; i < len; i++){
        num = num * 10 + (array[i] - '0');
    }
    return num;
}

/* [] END OF FILE */
