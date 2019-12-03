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
uint32_t interval;
uint8_t sample_count;
uint8_t sample_set;

void initRTC();
void hibernateISRHandler();
void batteryBackedRAM();
void setPeriodicMode();
#endif /* RTC_H_ */
