

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

#define COMPASS_XOUT_H      0x03
#define COMPASS_XOUT_L      0x04
#define COMPASS_YOUT_H      0x05
#define COMPASS_YOUT_L      0x06
#define COMPASS_ZOUT_H      0x07
#define COMPASS_ZOUT_L      0x08

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
        sprintf(str1,"accel: %d\r\n", accel());
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("gyro",0)){
        sprintf(str1,"gyro: %02d\r\n", gyro());
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("compass",0)){
        sprintf(str1,"compass: %02d\r\n", compass());
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

uint16_t compass()
{
    uint8_t x = readI2c0Register(COMPASS_ADDRESS,COMPASS_XOUT_H);
    uint8_t y = readI2c0Register(COMPASS_ADDRESS,COMPASS_YOUT_H);
    uint8_t z = readI2c0Register(COMPASS_ADDRESS,COMPASS_ZOUT_H);
    float raw = x*x+y*y+z*z;
    uint16_t raw1 = (uint16_t)sqrtf(raw);
    return raw1;
}

uint16_t accel()
{
    uint8_t x = readI2c0Register(MPU9250_ADDRESS,ACCEL_XOUT_H);
    uint8_t y = readI2c0Register(MPU9250_ADDRESS,ACCEL_YOUT_H);
    uint8_t z = readI2c0Register(MPU9250_ADDRESS,ACCEL_ZOUT_H);
    float raw = x*x+y*y+z*z;
    uint16_t raw1 = (uint16_t)sqrtf(raw);
    return raw1;
}

uint16_t gyro()
{
    uint8_t x = readI2c0Register(MPU9250_ADDRESS,GYRO_XOUT_H);
    uint8_t y = readI2c0Register(MPU9250_ADDRESS,GYRO_YOUT_H);
    uint8_t z = readI2c0Register(MPU9250_ADDRESS,GYRO_ZOUT_H);
    float raw = x*x+y*y+z*z;
    uint16_t raw1 = (uint16_t)sqrtf(raw);
    return raw1;
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

