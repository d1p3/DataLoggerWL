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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "rtc.h"
#include "uart0.h"

#define HIB_DATA_0          ((volatile unsigned long *)0x400FC030)
#define PUSH_BUTTON_MASK 1
void stop();
void initRTC(){
    HIB_CTL_R |= HIB_CTL_CLK32EN | HIB_CTL_RTCEN;
}

void setPeriodicMode()
{
    NVIC_EN1_R |= 1 << (INT_HIBERNATE - 16 - 32); // Enable Interrupts
    HIB_RTCM0_R = HIB_RTCC_R + floor(interval/1000);
//    while (!(HIB_CTL_R & HIB_CTL_WRC));
//    HIB_RTCSS_R = (uint16_t)(interval%1000)*32768/1000 << HIB_RTCSS_RTCSSM_S;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_IM_R |= HIB_IM_RTCALT0;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCT_R = 0x7FFF;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCLD_R = HIB_RTCC_R;
}
void hibernateISRHandler(){
    char str1[80];
    sample_count = 0;
    uint32_t status = HIB_MIS_R;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    if (status && HIB_MIS_RTCALT0)
    {
        sample_count++;
        sprintf(str1, "\r\nSample number = %d\r\n", sample_count);
        putsUart0(str1);

        while (!(HIB_CTL_R & HIB_CTL_WRC));
        if (sample_count == sample_set)
        {
            NVIC_EN1_R &= ~(1 << (INT_HIBERNATE - 16 - 32));
            stop();
        }

        else
        {
            HIB_RTCM0_R = HIB_RTCC_R + floor(interval/1000);
            while (!(HIB_CTL_R & HIB_CTL_WRC));
//            HIB_RTCSS_R = (uint16_t)(interval%1000)*32768/1000 << HIB_RTCSS_RTCSSM_S;
//            while (!(HIB_CTL_R & HIB_CTL_WRC));
        }
    }
    HIB_IC_R |= status;
    while (!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCLD_R = HIB_RTCC_R;
}

void initTrigger()
{
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R = PUSH_BUTTON_MASK;
    GPIO_PORTF_AMSEL_R &= ~PUSH_BUTTON_MASK;
    GPIO_PORTF_PCTL_R = 0;
    GPIO_PORTF_AFSEL_R = 0;
    GPIO_PORTF_DEN_R |= PUSH_BUTTON_MASK;
    GPIO_PORTF_PUR_R |= PUSH_BUTTON_MASK;
    GPIO_PORTF_IS_R &= ~PUSH_BUTTON_MASK;
    GPIO_PORTF_IBE_R &= ~PUSH_BUTTON_MASK;
    GPIO_PORTF_IEV_R |= PUSH_BUTTON_MASK;
    NVIC_EN0_R |= 1 << (INT_GPIOF - 16);
}



