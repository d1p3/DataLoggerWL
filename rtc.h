/*
 * rtc.h
 *
 *  Created on: Nov 5, 2019
 *      Author: Diptin
 */

#ifndef RTC_H_
#define RTC_H_
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
uint16_t interval;
uint32_t sample_count;
uint32_t sample_set;

void initRTC();
void hibernateISRHandler();
void batteryBackedRAM();
void setPeriodicMode();
#endif /* RTC_H_ */
