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

void initRTC();
void setPeriodicMode();
void HibernateISRHandler();

#endif /* RTC_H_ */
