#ifndef SERIALCOMMAND_H
#define SERIALCOMMAND_H
/************************************************************************
Title:    Serial Command validation and processing
Author:   Richard Clarke <richard@clarke.biz>   
File:     $Id:  $
Software: AVR-GCC 4.1, AVR Libc 1.4
Hardware: any AVR with built-in UART, tested on ATTiny2313 
License:  GNU General Public License 
Usage:    see Doxygen manual

LICENSE:
    Copyright (C) 2009 Richard Clarke

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
************************************************************************/
#include "global.h"

#define VALID_CMD_LEN 10

enum cmdState_t
{
	
	CHK_CMDLEN,
	CHK_STARTCHAR,
	CHK_SEPERATOR,
	CHK_ENDCHAR,
	EXTRACT_FIELDS,
	CHK_FINISH
	
};


/*Define structure of valid command string*/
#define START_CHAR_POS 			0
#define CMD_TYPE_CHAR_1_POS 	1
#define CMD_TYPE_CHAR_2_POS 	2
#define SEPERATOR_CHAR_POS		3
#define CMD_VALUE_CHAR_1_POS	4
#define CMD_VALUE_CHAR_2_POS	5
#define CMD_VALUE_CHAR_3_POS	6
#define CMD_VALUE_CHAR_4_POS	7
#define END_CHAR_POS			8


enum cmdResult_t
{
	CMD_INVALID,
	CMD_VALID
};

#define START_CHAR '!'		/* dec 33, 0x21*/
#define SEPERATOR_CHAR ':'
#define END_CHAR '#'

/*TODO:USER:RCC
**User added function for transferring a '\r' terminated serial command from the uart circular buffer
**to a linear command buffer for validation and processing
*/


/*************************************************************************
Function: sc_getCmd( char *cmdBuf )
Purpose:  return command string from ringbuffer  
Arguments: pointer to the command value location into which to place the numerical payload
Returns:  number of characters in the serial command.

		  Expected serial command format is, !SC:XXXX#
where '!' marks the start of a valid command, 'SC' is a 2 character command type, ':' is the 
command type and command value seperator, and XXXX is a ascii representation of a 16 bit hex 
value, and '#' marks end of command string. Thus, including the NULL terminator that the RX
ISR inserts, a valid command should have a length of 10 bytes.

Later on may add CRC information for validation of error free reception, useful when use RF link
*/
/**************************************************************************/
extern uint8_t sc_getCmd(void);

enum cmdResult_t sc_validateCmd(uint8_t *cmdType, uint16_t *cmdValue);

extern uint8_t asciiHexToUint(uint8_t *s);


#endif
