

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

        DateTime dt;
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
        char * str2 = (char*)accel();
        putsUart0(str2);
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

    else if (isCommand("periodic",1)){
        interval = getValue(0);
        setPeriodicMode();
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

    else if (isCommand("stop",1)){
        stop();
        interval = 60;
        sprintf(str1, "\r\nInterval = %d \r\n",interval);
        putsUart0(str1);
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

void stop()
{
    batteryBackedRAM();

}

char* accel(){
    char str1[32];
    MPU9250_Accelerometer();
    sprintf(str1,"x:%.2f,y:%.2f,z:%.2f,d:%d,t:%d\r\n", acceleration[0],acceleration[1],acceleration[2],HIB_RTCC_R-rtcD,HIB_RTCC_R-rtcT);
    return str1;
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

