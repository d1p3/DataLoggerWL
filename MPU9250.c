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
#define DATA_LENGTH     6

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

#define COMPASS_XOUT_H      0x03
#define COMPASS_XOUT_L      0x04
#define COMPASS_YOUT_H      0x05
#define COMPASS_YOUT_L      0x06
#define COMPASS_ZOUT_H      0x07
#define COMPASS_ZOUT_L      0x08

#define COMPASS_ENABLE_ADDRESS 55
#define COMPASS_CONTROL_REG    10
#define COMPASS_STATUS_ONE_REG 2
#define COMPASS_STATUS_TWO_REG 9


void test(){
    selfTest[0] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_ACCEL); // X-axis accel self-test results
    selfTest[1] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_ACCEL); // Y-axis accel self-test results
    selfTest[2] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_ACCEL); // Z-axis accel self-test results

    selfTest[3] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_GYRO);  // X-axis gyro self-test results
    selfTest[4] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_GYRO);  // Y-axis gyro self-test results
    selfTest[5] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_GYRO);  // Z-axis gyro self-test results
}

float temp(){
    float instantTemp;
    uint16_t raw = readAdc0Ss3();
    instantTemp = ((raw / 4096.0 * 3.3) - 0.074) / 0.00625;
    return instantTemp;
}

void MPU_ReadAccelerometer(int16_t *accel){
    uint8_t data[6];
    readI2c0Registers(MPU9250_ADDRESS,ACCEL_XOUT_H,DATA_LENGTH,data);
    accel[0] = (data[0] << 8) | data[1];
    accel[1] = (data[2] << 8) | data[3];
    accel[2] = (data[4] << 8) | data[5];
}

void MPU_ReadGyroscope(int16_t *raw_gyro){
    uint8_t data[6];
    readI2c0Registers(MPU9250_ADDRESS,GYRO_XOUT_H,DATA_LENGTH,data);
    raw_gyro[0] = (data[0] << 8) | data[1];
    raw_gyro[1] = (data[2] << 8) | data[3];
    raw_gyro[2] = (data[4] << 8) | data[5];
}

void MPU9250_Accelerometer(float *acceleration){
    uint8_t data[6];
    int16_t accel_t[3];
    readI2c0Registers(MPU9250_ADDRESS,ACCEL_XOUT_H,DATA_LENGTH,data);
    accel_t[0] = (data[0] << 8) | data[1];
    accel_t[1] = (data[2] << 8) | data[3];
    accel_t[2] = (data[4] << 8) | data[5];

    acceleration[0] = (float)(2 * accel_t[0])/32768;
    acceleration[1] = (float)(2 * accel_t[1])/32768;
    acceleration[2] = (float)(2 * accel_t[2])/32768;
}

void MPU9250_Gyroscope(float *degrees){
    uint8_t data[6];
    int16_t gyro_temp[3];
    readI2c0Registers(MPU9250_ADDRESS, GYRO_XOUT_H, DATA_LENGTH, data);
    gyro_temp[0] = (data[0] << 8) | data[1];
    gyro_temp[1] = (data[2] << 8) | data[3];
    gyro_temp[2] = (data[4] << 8) | data[5];

    degrees[0] = (float)(250 * gyro_temp[0])/32768.0;
    degrees[1] = (float)(250 * gyro_temp[1])/32768.0;
    degrees[2] = (float)(250 * gyro_temp[2])/32768.0;
}
void enableCompass(){
    writeI2c0Register(COMPASS_ADDRESS, COMPASS_ENABLE_ADDRESS, asciiToUint8("0x02"));
    writeI2c0Register(COMPASS_ADDRESS, COMPASS_CONTROL_REG, asciiToUint8("0x16"));
}

void MPU9250_Compass(int16_t *compass){
    uint8_t data[6];
    while(! (readI2c0Register(COMPASS_ADDRESS, COMPASS_STATUS_ONE_REG) & 0x01) );
    readI2c0Registers(COMPASS_ADDRESS, COMPASS_XOUT_H, DATA_LENGTH, data);
    compass[0] = (int16_t)(data[1] << 8) | data[0];
    compass[1] = (int16_t)(data[3] << 8) | data[2];
    compass[2] = (int16_t)(data[5] << 8) | data[4];
    readI2c0Register(COMPASS_ADDRESS,COMPASS_STATUS_TWO_REG);
}

void calibrate(float* accel_offset, float* gyro_offset){
    int packets = 1024;

    float cal_accel[3];
    float cal_gyro[3];

    int16_t accel_bias[3];
    int16_t deg_bias[3];

    int64_t accel_sum[3] = {0,0,0};
    int64_t gyro_sum[3] = {0,0,0};
    int i = 0;
    for(i=0;i<packets;i++){
        MPU9250_Accelerometer(cal_accel);
        MPU9250_Gyroscope(cal_gyro);

        accel_sum[0] += cal_accel[0];
        accel_sum[1] += cal_accel[1];
        accel_sum[2] += cal_accel[2];

        gyro_sum[0] += cal_gyro[0];
        gyro_sum[1] += cal_gyro[1];
        gyro_sum[2] += cal_gyro[2];
    }

    accel_bias[0] = accel_sum[0] / packets;
    accel_bias[1] = accel_sum[1] / packets;
    accel_bias[2] = accel_sum[2] / packets;

    deg_bias[0] = gyro_sum[0] / packets;
    deg_bias[1] = gyro_sum[1] / packets;
    deg_bias[2] = gyro_sum[2] / packets;

    accel_offset[0] = (float)(2 * accel_bias[0])/32768;
    accel_offset[1] = (float)(2 * accel_bias[1])/32768;
    accel_offset[2] = (float)(2 * accel_bias[2])/32768;

    gyro_offset[0] = (float)(2 * deg_bias[0])/32768;
    gyro_offset[1] = (float)(2 * deg_bias[1])/32768;
    gyro_offset[2] = (float)(2 * deg_bias[2])/32768;

}
