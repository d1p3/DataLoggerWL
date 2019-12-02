/*
 * MPU9250.c
 *
 *  Created on: Dec 1, 2019
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
#include "MPU9250.h"
#include "i2c0.h"
#include "uart0.h"
//Sensor related definitations//////////////////////////////////////////////////////
#define MPU9250_ADDRESS 0x68  // Device address when ADO = 0
#define COMPASS_ADDRESS 0x0C   //  Address of magnetometer

#define SELF_TEST_X_GYRO 0x00
#define SELF_TEST_Y_GYRO 0x01
#define SELF_TEST_Z_GYRO 0x02

#define SELF_TEST_X_ACCEL 0x0D
#define SELF_TEST_Y_ACCEL 0x0E
#define SELF_TEST_Z_ACCEL 0x0F

#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C

#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40

#define GYRO_XOUT_H      0x43
#define GYRO_XOUT_L      0x44
#define GYRO_YOUT_H      0x45
#define GYRO_YOUT_L      0x46
#define GYRO_ZOUT_H      0x47
#define GYRO_ZOUT_L      0x48

#define COMPASS_XOUT_H      0x0D
#define COMPASS_XOUT_L      0x0E
#define COMPASS_YOUT_H      0x4B
#define COMPASS_YOUT_L      0x4C
#define COMPASS_ZOUT_H      0x4D
#define COMPASS_ZOUT_L      0x4E


void test()
{
    selfTest[0] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_ACCEL); // X-axis accel self-test results
    selfTest[1] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_ACCEL); // Y-axis accel self-test results
    selfTest[2] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_ACCEL); // Z-axis accel self-test results

    selfTest[3] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_GYRO);  // X-axis gyro self-test results
    selfTest[4] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_GYRO);  // Y-axis gyro self-test results
    selfTest[5] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_GYRO);  // Z-axis gyro self-test results
}

float temp()
{
    float instantTemp;
    uint16_t raw = readAdc0Ss3();
    instantTemp = ((raw / 4096.0 * 3.3) - 0.074) / 0.00625;
    return instantTemp;
}

void MPU_ReadMagnetometer()
{
    raw_compass[0] = readI2c0Register(COMPASS_ADDRESS,COMPASS_XOUT_H);
    raw_compass[1] = readI2c0Register(COMPASS_ADDRESS,COMPASS_XOUT_L);
    raw_compass[2] = readI2c0Register(COMPASS_ADDRESS,COMPASS_YOUT_H);
    raw_compass[3] = readI2c0Register(COMPASS_ADDRESS,COMPASS_YOUT_L);
    raw_compass[4] = readI2c0Register(COMPASS_ADDRESS,COMPASS_ZOUT_H);
    raw_compass[5] = readI2c0Register(COMPASS_ADDRESS,COMPASS_ZOUT_L);

}

void MPU_ReadAccelerometer()
{
    raw_accel[0] = readI2c0Register(MPU9250_ADDRESS,ACCEL_XOUT_H);
    raw_accel[1] = readI2c0Register(MPU9250_ADDRESS,ACCEL_XOUT_L);
    raw_accel[2] = readI2c0Register(MPU9250_ADDRESS,ACCEL_YOUT_H);
    raw_accel[3] = readI2c0Register(MPU9250_ADDRESS,ACCEL_YOUT_L);
    raw_accel[4] = readI2c0Register(MPU9250_ADDRESS,ACCEL_ZOUT_H);
    raw_accel[5] = readI2c0Register(MPU9250_ADDRESS,ACCEL_ZOUT_L);
}

void MPU_ReadGyroscope()
{
    raw_gyro[0] = readI2c0Register(MPU9250_ADDRESS,GYRO_XOUT_H);
    raw_gyro[1] = readI2c0Register(MPU9250_ADDRESS,GYRO_XOUT_L);
    raw_gyro[2] = readI2c0Register(MPU9250_ADDRESS,GYRO_YOUT_H);
    raw_gyro[3] = readI2c0Register(MPU9250_ADDRESS,GYRO_YOUT_L);
    raw_gyro[4] = readI2c0Register(MPU9250_ADDRESS,GYRO_ZOUT_H);
    raw_gyro[5] = readI2c0Register(MPU9250_ADDRESS,GYRO_ZOUT_L);
}

void MPU9250_Accelerometer(){
    MPU_ReadAccelerometer();
    accel_t[0] = (raw_accel[0] << 8) | raw_accel[1];
    accel_t[1] = (raw_accel[2] << 8) | raw_accel[3];
    accel_t[2] = (raw_accel[4] << 8) | raw_accel[5];

    acceleration[0] = (float)(2 * accel_t[0])/32768;
    acceleration[1] = (float)(2 * accel_t[1])/32768;
    acceleration[2] = (float)(2 * accel_t[2])/32768;
}

void MPU9250_Gyroscope(){
    MPU_ReadGyroscope();

    deg[0] = (raw_gyro[0] << 8) | raw_gyro[1];
    deg[1] = (raw_gyro[2] << 8) | raw_gyro[3];
    deg[2] = (raw_gyro[4] << 8) | raw_gyro[5];

    degrees[0] = (float)(250 * deg[0])/32768.0;
    degrees[1] = (float)(250 * deg[1])/32768.0;
    degrees[2] = (float)(250 * deg[2])/32768.0;
}

void MPU9250_Compass(){
    MPU_ReadMagnetometer();
    comp[0] = (raw_compass[0] << 8) | raw_compass[1];
    comp[1] = (raw_compass[2] << 8) | raw_compass[3];
    comp[2] = (raw_compass[4] << 8) | raw_compass[5];

    compass[0] = (float)(10 *4912 * comp[0])/32760;
    compass[1] = (float)(10 *4912 * comp[1])/32760;
    compass[2] = (float)(10 *4812 * comp[2])/32760;
}



