

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
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "helperFunction.h"
#include "uart0.h"
#include "i2c0.h"
//#include "string.h"

#define MAX_CHARS 80
char str[MAX_CHARS];
#define MAX_CHARS 80
#define MAX_FIELDS 2
uint16_t hour, min, sec, month, day, year;
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
                ltoa(i,str1);
                putsUart0(str1);
                putsUart0("\r\n");
                //sprintf(str1,"%x",i);
                //putsUart0(str1);
            }
        }
        putsUart0("\r\n");
    }
    else if (isCommand("write",3)){
        add = getValue(0);
        reg = getValue(1);
        data = getValue(2);
        writeI2c0Register(add, reg, data);
        //sprintf(str1, "Writing 0x%02hhx to address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
        //putsUart0(str1);
    }
    else if (isCommand("read",2)){
        add = getValue(0);
        reg = getValue(1);
        data = readI2c0Register(add, reg);
        //sprintf(str1, "Read 0x%02hhx from address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
        //putsUart0(str1);
    }
    else if (isCommand("settime",3)){
          hour = getValue(0);
          min = getValue(1);
          sec = getValue(2);
      }
    else if (isCommand("time",0)){
        putsUart0("Time");
      }
    else if (isCommand("setdate",3)){
          month = getValue(0);
          day = getValue(1);
          year = getValue(2);
      }
    else if (isCommand("date",0)){
        putsUart0("Time");
      }
    else if (isCommand("temp",0)){
        uint16_t raw;
        float instantTemp;
        putsUart0("Temp\r\n");
        raw = readAdc0Ss3();
        // Calculate temperature in degC as follows:
        //   For the 12-bit SAR ADC with Vref+ = 3.3V and Vref- = 0V, outputing a result R:
        //   Resolution is approx 0.81mV / LSb or 0.13 degC / LSb
        //   R(Vin) = floor(Vin/3.3V * 4096) -> Vin(R) ~= 3.3V * ((R+0.5) / 4096)
        //   (~ and 0.5LSb offset in Vin(R) equation are introduced for mid-tread value of the SAR transfer function)
        //   T(Vin) = (Vin - 0.424V) / 0.00625V
        //   T(R) ~= ([3.3V * ((R+0.5) / 4096)] - 0.424V) / 0.00625V
        //   T(R) ~= (0.12890625 * R) - 67.775546875 (simplified floating point equation to save cycles)
        instantTemp = ((raw / 4096.0 * 3.3) - 0.424) / 0.00625;

        // display raw ADC value and temperatures
        sprintf(str1, "Raw ADC:        %u\r\n", raw);
        putsUart0(str1);
        sprintf(str1, "Unfiltered (C): %3.1f\r\n", instantTemp);
        putsUart0(str1);
        putsUart0("\r\n");
    }

    else if (isCommand("help",0)){
        putsUart0("poll\r\n");
        putsUart0("read ADD REG\r\n");
        putsUart0("write ADD REG DATA\r\n");
    }
    else ok = false;

    return ok;
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

    putsUart0("Data Logger Ready!");
    putsUart0("\r\n");
    putsUart0("Enter Command:");
    putsUart0("\r\n");
    while(1){
       getsUart0(str,MAX_CHARS);
       tokenizeString();
       if(!ExecuteCommand()){
           putsUart0("\r\n");
           putsUart0("Invalid Command!");
       }
       putsUart0("\r\n");
    }
    }

