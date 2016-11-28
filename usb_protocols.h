/*********************************************************************************
* File Name: usb_protocols.h
* Version 3.20
*
* Description:
*  This file contains the function prototypes and constants used for
*  the USB protocols.
*
*
**********************************************************************************
 * Copyright Naresuan University, Phitsanulok Thailand
 * Released under Creative Commons Attribution-ShareAlike  3.0 (CC BY-SA 3.0 US)
*********************************************************************************/


#if !defined(USB_PROTOCOLS_H)
#define USB_PROTOCOLS_H
    
#include <project.h>

/**************************************
*      Constants
**************************************/
    
#define IN_ENDPOINT 0X01
#define OUT_ENDPOINT 0x02
#define MAX_BUFFER_SIZE 32
#define true 1
#define false 0
    
// Define variables for the UBS device
#define MAX_NUM_BYTES 32  // how big to make the IN and OUT ENDPOINT BUFFERS
#define MAX_DATA_BUFFER 16 // make this MAX_NUM_BYTES / 2

/* External variable of the device address located in USBFS.h */
extern uint8 USB_deviceAdress;
    
/***************************************
*        Function Prototypes
***************************************/  
    
uint8 USB_CheckInput(uint8 buffer[]);
void USB_Export_Data(uint8 array[], uint16 size);

#endif

/* [] END OF FILE */
