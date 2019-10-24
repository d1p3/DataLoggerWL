

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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "helperFunction.h"
#include "uart0.h"
#include "i2c0.h"
#include "stdio.h"

#define MAX_CHARS 80
char str[MAX_CHARS];
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
    uint8_t i;
    uint8_t add;
    uint8_t reg;
    uint8_t data;
    uint16_t raw;
    float instantTemp;
    char* token;
    char str[80];

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
                sprintf(str, "0x%02x ", i);
                putsUart0(str);
            }
        }
        putsUart0("\r\n");
    }
    else if (isCommand("write",3)){
        token = strtok(NULL, " ");
        putsUart0(token);
        ok = ok && token != NULL;
        add = asciiToUint8(token);
        token = strtok(NULL, " ");
        ok = ok && token != NULL;
        reg = asciiToUint8(token);
        token = strtok(NULL, " \r\n");
        ok = ok && token != NULL;
        data = asciiToUint8(token);
        if (ok)
        {
            writeI2c0Register(add, reg, data);
            sprintf(str, "Writing 0x%02hhx to address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
            putsUart0(str);
        }
        else
            putsUart0("Error in write command arguments\r\n");
    }
    else if (isCommand("read",2)){
        token = strtok(NULL, " ");
        ok = ok && token != NULL;
        add = asciiToUint8(token);
        token = strtok(NULL, " \r\n");
        ok = ok && token != NULL;
        reg = asciiToUint8(token);
        if (ok)
        {
            data = readI2c0Register(add, reg);
            sprintf(str, "Read 0x%02hhx from address 0x%02hhx, register 0x%02hhx\r\n", data, add, reg);
            putsUart0(str);
        }
        else
            putsUart0("Error in read command arguments\r\n");
    }
    else if (isCommand("help",0)){
        putsUart0("poll\r\n");
        putsUart0("read ADD REG\r\n");
        putsUart0("write ADD REG DATA\r\n");
    }
    else if (isCommand("temp",0)){
        raw = readAdc0Ss3();
        instantTemp = ((raw / 4096.0 * 3.3) - 0.424) / 0.00625;
        // display raw ADC value and temperatures
        sprintf(str, "Raw ADC:        %u\r\n", raw);
        putsUart0(str);
        sprintf(str, "Unfiltered (C): %.1f\r\n", instantTemp);
        putsUart0(str);
        putsUart0("\r\n");
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

