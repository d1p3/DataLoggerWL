/*
 * MPU9250.h
 *
 *  Created on: Dec 1, 2019
 *      Author: Diptin
 */

#ifndef MPU9250_H_
#define MPU9250_H_

//Accelerometer
uint8_t raw_accel[6];
int16_t accel_t[3];
float acceleration[3];
//Gyroscope
uint8_t raw_gyro[6];
int16_t deg[3];
int16_t degrees[3];
//Magnetometer
uint8_t raw_compass[6];
int16_t comp[3];
int16_t compass[3];

float selfTest[6];// holds results of gyro and accelerometer self test
////////////////////////////////////////////////////////////////////////////////////////

void MPU9250_Compass();
void MPU9250_Gyroscope();
void MPU9250_Accelerometer();
void MPU_ReadGyroscope();
void MPU_ReadAccelerometer();
void MPU_ReadMagnetometer();
float temp();
void test();

#endif /* MPU9250_H_ */
