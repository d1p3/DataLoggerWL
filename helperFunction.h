/*
 * helperFunction.h
 *
 *  Created on: Oct 19, 2019
 *      Author: Diptin
 */

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Keyboard Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef HELPERFUNCTION_H_
#define HELPERFUNCTION_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void tokenizeString();
bool isCommand(char *cmd, uint8_t min);

#endif /* HELPERFUNCTION_H_ */
