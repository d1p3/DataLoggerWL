

/**
 * main.c
 */
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "i2c0.h"
#include "rtc.h"
//#include "string.h"

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

float selfTest[6];// holds results of gyro and accelerometer self test
////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CHARS 80
char str[MAX_CHARS];
#define MAX_CHARS 80
#define MAX_FIELDS 2
uint32_t hour, min, sec, month, day, year;
uint32_t rtcD,rtcT;
char str[MAX_CHARS];
uint8_t pos[MAX_FIELDS];
uint8_t count = 0;
uint8_t raw_accel[6];
int16_t accel[3];
int16_t acceleration[3];
uint8_t raw_gyro[6];
int16_t deg[3];
int16_t degrees[3];
uint8_t raw_compass[6];
int16_t comp[3];
int16_t compass[3];

// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;
}

void getsUart0(char str[], uint8_t size)
{
    uint8_t count = 0;
    bool end = false;
    char c;
    while(!end)
    {
        c = getcUart0();
        end = (c == 13) || (count == size);
        if (!end)
        {
            if ((c == 8 || c == 127) && count > 0)
                count--;
            if (c >= ' ' && c < 127)
                str[count++] = c;
        }
    }
    str[count] = '\0';
}
// Tokenize Input string by replacing delimeter with null character and add position, type and field count
void tokenizeString()
{
    count = 0;
    int i;
    int N = strlen(str);//Length of the input
    char p ='d';//previous
    char c;//current
    for (i = 0; i < N; i++){
        char c = str[i];
        if (c>='a' && c<='z') //if c is alphabet (a-z)
            c='a';
        else if (c>='0' && c<='9') //if c is number (0-9)
            c='n';
        else{
            c='d';
            str[i]=0;
        }
        if (p != c){
            if (p=='d' && c=='a'){      //transition from delimeter to alpha
                pos[count]=i;
                count++;
            }
            else if (p=='d' && c=='n'){         //transition from delimeter to number
                pos[count]=i;
                count++;
            }
        }
        p = c;
    }
}

char *getString(uint8_t argN){
    return &str[pos[argN+1]];
}

// Return the numerical value of argument
uint16_t getValue(uint8_t argN){
    return atoi(getString(argN));
}

// Check valid command
bool isCommand(char *cmd, uint8_t min){
    if (count > min){
        int i;
        for(i = 0; i < strlen(cmd); i++){
            if (cmd[i] != str[pos[0]+i])
                return false;
        }
        return true;
    }
    else
        return false;
}
uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' && tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}

bool ExecuteCommand(){
    bool ok = true;
    char str1[80];
    uint8_t i;
    uint8_t add;
    uint8_t reg;
    uint8_t data;
    //char strInput[MAX_CHARS+1];
    char* token;
    if (isCommand("start",0)){
        putsUart0("All good!");
        putsUart0("\r\n");
    }

    else if (isCommand("exit",0)){
        putsUart0("Exiting!");
        putsUart0("\r\n");
    }

    else if (isCommand("poll",0)){
        putsUart0("Devices found: ");
        for (i = 4; i < 119; i++)
        {
            if (pollI2c0Address(i))
            {
                sprintf(str1,"0x%02x ",i);
                putsUart0(str1);
            }
        }
        putsUart0("\r\n");
    }

    else if (isCommand("write",3)){
        add = (uint8_t)getValue(0);
        reg = (uint8_t)getValue(1);
        data = (uint8_t)getValue(2);
        writeI2c0Register(add, reg, data);
        sprintf(str1, "Writing 0x%02hhx to address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
        putsUart0(str1);
    }

    else if (isCommand("read",2)){
        add = (uint8_t)getValue(0);
        reg = (uint8_t)getValue(1);
        data = readI2c0Register(add, reg);
        sprintf(str1, "Read 0x%02hhx from address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
        putsUart0(str1);
    }

    else if (isCommand("time",3)){
          hour = getValue(0);
          min = getValue(1);
          sec = getValue(2);
          rtcT = HIB_RTCC_R;
          sprintf(str1, "\r\n Time set to hh = %d, mm = %d, ss = %d, rtcVal = %02f\r\n",hour,min, sec,(float)hour*3600 + min*60 + sec);
          putsUart0(str1);
      }
    else if (isCommand("time",0)){
        //uint32_t setTime = hour*3600 + min*60 + sec;
        uint32_t dT = HIB_RTCC_R - rtcT;
        sprintf(str1, "\r\nTime = %d secs\r\n",dT);
        putsUart0(str1);
      }

    else if (isCommand("date",3)){
          month = getValue(0);
          day = getValue(1);
          year = getValue(2);
          rtcD = HIB_RTCC_R;
          sprintf(str1, "\r\n Date set to mm = %d, dd = %d, yy = %d, rtcVal = %.2f\r\n",month,day,year,(float)(year*365*24*60*60)+(float)(month*30*24*60*60)+(day*24*60*60));
          putsUart0(str1);
      }

    else if (isCommand("date",0)){
        uint32_t dT = HIB_RTCC_R-rtcD;
        //float setDate = (float)(year*365*24*60*60)+(float)(month*30*24*60*60)+(day*24*60*60);
        sprintf(str1, "\r\nDate = %.2f secs\r\n",(float)dT);
        putsUart0(str1);
      }

    else if (isCommand("temp",0)){
        sprintf(str1,"Temperature: %02d\r\n", temp());
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("accel",0)){
        MPU9250_Accelerometer();
        sprintf(str1,"accelx: %d, accely: %d, accelz: %d\r\n", acceleration[0],acceleration[1],acceleration[2]);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("gyro",0)){
        MPU9250_Gyroscope();
        sprintf(str1,"gyrox: %d, gyroy: %d, gyroz: %d\r\n", degrees[0],degrees[1],degrees[2]);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("compass",0)){
        MPU9250_Compass();
        sprintf(str1,"Compassx: %d, Compassy: %d, Compassz: %d\r\n", compass[0],compass[1],compass[2]);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("log",1)){
        if(strcmp(getString(0),"compass")==0)
        {
            putsUart0("Kam22222222");
        }

        if(strcmp(getString(0),"accel")==0)
        {
            putsUart0("Acc1111111111");
        }

        if(strcmp(getString(0),"gyro")==0)
        {
            putsUart0(getString(0));
        }

        if(strcmp(getString(0),"temp")==0)
        {
            putsUart0(getString(0));
        }
    }

    else if (isCommand("samples",1)){

    }

    else if (isCommand("stop",1)){
        stop();
        putsUart0("Done!");
     }


    else if (isCommand("help",0)){
        putsUart0("poll\r\n");
        putsUart0("read ADD REG\r\n");
        putsUart0("write ADD REG DATA\r\n");
        putsUart0("time hh mm ss\r\n");
        putsUart0("date mm dd yy\r\n");
        putsUart0("time \r\n");
        putsUart0("date \r\n");

    }
    else ok = false;

    return ok;
}


void test()
{
    selfTest[0] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_ACCEL); // X-axis accel self-test results
    selfTest[1] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_ACCEL); // Y-axis accel self-test results
    selfTest[2] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_ACCEL); // Z-axis accel self-test results

    selfTest[3] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_X_GYRO);  // X-axis gyro self-test results
    selfTest[4] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Y_GYRO);  // Y-axis gyro self-test results
    selfTest[5] = readI2c0Register(MPU9250_ADDRESS, SELF_TEST_Z_GYRO);  // Z-axis gyro self-test results
}

uint16_t temp()
{
    uint16_t raw;
    raw = readAdc0Ss3();
    return raw;
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
    accel[0] = (raw_accel[0] << 8) | raw_accel[1];
    accel[1] = (raw_accel[2] << 8) | raw_accel[3];
    accel[2] = (raw_accel[4] << 8) | raw_accel[5];

    acceleration[0] = (float)(2 * accel[0])/32768;
    acceleration[1] = (float)(2 * accel[1])/32768;
    acceleration[2] = (float)(2 * accel[2])/32768;
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
void stop()
{

}
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();
    initUart0();
    initI2c0();
    initRTC();
    test();
    putsUart0("Data Logger Ready! \r\n");
    putsUart0("TimeFormat hh mm ss \r\n");
    putsUart0("DateFormat mm dd yy \r\n");
    while(1){
        putsUart0("\r\nEnter Command: \r\n");
        getsUart0(str,MAX_CHARS);
        tokenizeString();
        if(!ExecuteCommand()){
           putsUart0("\r\n");
           putsUart0("Invalid Command!");
        }
        putsUart0("\r\n");
    }
}

