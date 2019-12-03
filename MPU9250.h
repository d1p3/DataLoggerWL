/*
 * MPU9250.h
 *
 *  Created on: Dec 1, 2019
 *      Author: Diptin
 */

#ifndef MPU9250_H_
#define MPU9250_H_

float selfTest[6];// holds results of gyro and accelerometer self test
////////////////////////////////////////////////////////////////////////////////////////
uint8_t asciiToUint8();
void MPU9250_Compass();
void MPU9250_Gyroscope();
void MPU9250_Accelerometer();
void MPU_ReadGyroscope();
void MPU_ReadAccelerometer();
void MPU_ReadMagnetometer();
void enableCompass();
void calibrate(float* accel_offset, float* gyro_offset);
float temp();
void test();

#endif /* MPU9250_H_ */
