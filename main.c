

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
#include "MPU9250.h"
//#include "string.h"




#define MAX_CHARS 80
char str[MAX_CHARS];
#define MAX_CHARS 80
#define MAX_FIELDS 2
uint8_t hour, min, sec, month, day;
uint32_t rtcD,rtcT;
char str[MAX_CHARS];
uint8_t pos[MAX_FIELDS];
uint8_t count = 0;
//[0]LowTemperature,[1]HighTemperature,[2]LowAccel,[3]HighAccel
int gating_param[4];
int log_mask[4] = {-1,-1,-1,-1};
int hysteresis_mask[4] = {-1,-1,-1,-1};
float accel_offset[3];
float gyro_offset[3];
float acceleration[3];
float gyroscope_readings[3];
int16_t magnetic_readings[3];

typedef struct _DateTime {
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} DateTime;

uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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
void reset(){
    NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}

void stop()
{
    putsUart0("\r\nSTOP\r\n");
    interval = 0;
}


bool ExecuteCommand(){
    bool ok = true;
    char str1[80];
    if (isCommand("time",3)){
          hour = getValue(0);
          min = getValue(1);
          sec = getValue(2);
          rtcT = HIB_RTCC_R;
      }

    else if (isCommand("time",0)){
        //uint32_t setTime = hour*3600 + min*60 + sec;
        uint32_t dT = HIB_RTCC_R - rtcT;
        uint32_t hh = (uint32_t) dT / 3600;
        uint32_t remainder = (uint32_t) dT - hh * 3600;
        uint32_t mm = remainder / 60;
        remainder = remainder - mm * 60;
        uint32_t ss = remainder;
        sprintf(str1, "\r\nhh:mm:ss = %d:%d:%d \r\n",hh + hour ,mm + min, ss+ sec);
        putsUart0(str1);
      }

    else if (isCommand("date",2)){
          month = getValue(0);
          day = getValue(1);
      }

    else if (isCommand("date",0)){
        //float setDate = (float)(year*365*24*60*60)+(float)(month*30*24*60*60)+(day*24*60*60);
        int8_t deltaT = HIB_RTCC_R - rtcT;
        int8_t hh = (uint32_t) deltaT / 3600;
        int8_t remainder = (uint32_t) deltaT - hh * 3600;
        int8_t mm = remainder / 60;
        remainder = remainder - mm * 60;
        uint8_t ss = remainder;

        int8_t i;
        int8_t days_in_month;
        int8_t n_month = 0;
        int8_t n_day= 0;

        if((hh+hour) >= 24){
            n_day = day + (int8_t)((hh+hour)/24);
            mm = min + (hh+hour) % 24;
        }

        if(n_day >= 28){
            for(i=0;i<12;i++){
                days_in_month = monthDays[i];
                if(month == 0)
                    break;
                if(i==month-1){
                    break;
                }
            }
            n_month = (int8_t)n_day/days_in_month;
            n_day = n_day % days_in_month;
        }

        sprintf(str1, "\r\nmonth/day = %d/%d   hh:mm:ss = %d:%d:%d \r\n", n_month + month , n_day + day , hh + hour , mm + min, ss + sec);
        putsUart0(str1);
      }

    else if (isCommand("temp",0)){
        sprintf(str1,"Temperature: %3.1f\r\n", temp());
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("accel",0)){
        sprintf(str1,"x:%.2f,y:%.2f,z:%.2f,d:%d,t:%d\r\n", acceleration[0],acceleration[1],acceleration[2],HIB_RTCC_R-rtcD,HIB_RTCC_R-rtcT);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("gyro",0)){
        sprintf(str1,"gyrox: %.2f, gyroy: %.2f, gyroz: %.2f\r\n", gyroscope_readings[0],gyroscope_readings[1],gyroscope_readings[2]);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("compass",0)){
        sprintf(str1,"Compassx: %d, Compassy: %d, Compassz: %d\r\n", magnetic_readings[0],magnetic_readings[1],magnetic_readings[2]);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("periodic",1)){
        interval = getValue(0);
        setPeriodicMode(interval);
        putsUart0("Periodic mode set!");
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
        sample_set = getValue(0);
    }

    else if (isCommand("trigger",0)){
        GPIO_PORTF_IM_R |= 1;   // enable interrupts for Push Button 2
     }

    else if(isCommand("gating",3)){
        if(strcmp(getString(0),"temp") == 0){
            //gating for Upper Temperature value;
            gating_param[1] = getValue(1);
            //gating for Lower Temperature value;
            gating_param[0] = getValue(2);
        }

        if(strcmp(getString(0),"temp") == 0){
             //gating for Upper Acceleration value;
             gating_param[3] = getValue(1);
             //gating for Lower Acceleration value;
             gating_param[2] = getValue(2);
         }
    }

    else if (isCommand("reset",0)){
        reset();
    }

    else if (isCommand("stop",0)){
        stop();
        putsUart0("Done!");
     }

    else if (isCommand("save",0)){
        putsUart0("Sam's implementation");
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

void singleValues(float *accel, float *gyroscope_readings, int16_t *magnetic_readings,float *offset_accel, float *offset_gyro)
{
    float divider = 0;
    MPU9250_Accelerometer(accel);
    MPU9250_Gyroscope(gyroscope_readings);
    MPU9250_Compass(magnetic_readings);

    int i =0;
    for(i=0;i<3;i++)
    {
       accel[i] = accel[i] - offset_accel[i];
       divider += accel[i]*accel[i];
       gyroscope_readings[i] = gyroscope_readings[i] - offset_gyro[i];
    }

    divider = sqrt(divider);
    accel[0] = accel[0] / divider;
    accel[1] = accel[1] / divider;
    accel[2] = accel[2] / divider;
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
    enableCompass();
    calibrate(accel_offset,gyro_offset);
    singleValues(acceleration,gyroscope_readings,magnetic_readings,accel_offset,gyro_offset);
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

