//
// Diptin Dahal
// Samnang Chay
// Project: DataLoggerWL
//-----------------------------------------------------------------------------
// Hardware Discription
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

// I2C devices on I2C bus 0 with 2kohm pullups on SDA and SCL

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "i2c0.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

#define MAX_CHARS 80
int main(void)
{
    char strInput[MAX_CHARS+1];
    char* token;
    char str[80];
    uint8_t i;
    uint8_t add;
    uint8_t reg;
    uint8_t data;
    bool ok;
    // Initialize hardware
    initHw();
    initUart0();
    initI2c0();

    putsUart0("I2C0 R/W, Poll Utility\r\n");
    while (true)
    {
        getsUart0(strInput, MAX_CHARS);
        token = strtok(strInput, " ");
        ok = token != NULL;
        if (strcmp(token, "write") == 0)
        {
            token = strtok(NULL, " ");
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
                sprintf(str, "Writing 0x%02hhx to address 0x%02hhx, register 0x%02hhx\r", data, add, reg);
                putsUart0(str);
            }
            else
                putsUart0("Error in write command arguments\r");
        }
        if (strcmp(token, "read") == 0)
        {
            token = strtok(NULL, " ");
            ok = ok && token != NULL;
            add = asciiToUint8(token);
            token = strtok(NULL, " \r\n");
            ok = ok && token != NULL;
            reg = asciiToUint8(token);
            if (ok)
            {
                data = readI2c0Register(add, reg);
                sprintf(str, "Read 0x%02hhx from address 0x%02hhx, register 0x%02hhx\r", data, add, reg);
                putsUart0(str);
            }
            else
                putsUart0("Error in read command arguments\r");
        }
        if (strcmp(token, "poll") == 0)
        {
            putsUart0("Devices found: ");
            for (i = 4; i < 119; i++)
            {
                if (pollI2c0Address(i))
                {
                    sprintf(str, "0x%02x ", i);
                    putsUart0(str);
                }
            }
            putsUart0("\r");
        }
        if (strcmp(token, "help") == 0)
        {
            putsUart0("poll\r");
            putsUart0("read ADD REG\r");
            putsUart0("write ADD REG DATA\r");
        }
    }
    return 0;
}
