/*
 * rtc.c
 *
 *  Created on: Nov 5, 2019
 *      Author: Diptin
 */
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Keyboard Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "rtc.h"

#define HIB_DATA_0          ((volatile unsigned long *)0x400FC030)
void initRTC(){
    HIB_CTL_R |= 0x41;
    NVIC_EN1_R |= 1 << (INT_HIBERNATE - 16 - 32); // Enable Interrupts
}

void setPeriodicMode(){
    stop();
    HIB_RTCM0_R = HIB_RTCC_R + floor(interval/1000);
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCSS_R = (uint16_t)(interval%1000)*32768/1000 << HIB_RTCSS_RTCSSM_S;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_IM_R |= HIB_IM_RTCALT0;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCT_R = 0x7FFF;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCLD_R = HIB_RTCC_R;
}

void hibernateISRHandler(){
    char str[80];
    uint32_t status = HIB_MIS_R;

    while (!(HIB_CTL_R & HIB_CTL_WRC));

    if (status && HIB_MIS_RTCALT0){ //HIB_MIS_RTCALT0 = 1 ~ match occurred
        //sample_count++;
        sprintf(str, "\r\nSample number = %d\r\n", sample_count);
        putsUart0(str);
        // temp();
        accel();
        // gyro();
        // compass();
        if (sample_count == sample_set){
            HIB_IM_R &= ~HIB_IM_RTCALT0;
            while (!(HIB_CTL_R & HIB_CTL_WRC));
            sample_count = 0;
        }
        else{
            HIB_RTCM0_R = HIB_RTCC_R + floor(interval/1000);
            while (!(HIB_CTL_R & HIB_CTL_WRC));
            HIB_RTCSS_R = (uint16_t)(interval%1000)*32768/1000 << HIB_RTCSS_RTCSSM_S;
            while (!(HIB_CTL_R & HIB_CTL_WRC));
        }
    }
    HIB_IC_R |= status;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCLD_R = HIB_RTCC_R;
}

void batteryBackedRAM(){
    char str[80];
    *HIB_DATA_0 = 7;
    *(HIB_DATA_0+1) = 5;
    sprintf(str, "\r\nData1 = %d Data2= %d\r\n", *HIB_DATA_0,*(HIB_DATA_0+1));
    putsUart0(str);
}





