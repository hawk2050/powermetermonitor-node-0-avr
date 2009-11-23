//
// serialcommand.c
//
// Contains application level functions for validating
// and processing serial command strings. Sits above the 
// lower level UART ring buffer and interrupt service routines.
//
// Author: Richard C Clarke
// Date: March 2009
//


// includes

#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <inttypes.h>

#include "global.h"
#include "timer.h"

#include "uart.h"		// include uart function library
#include "serialcommand_rcc.h"


static uint8_t cmdLength;
uint8_t linearBuffer[12];

/*************************************************************************
Function: uart_getCmd( char *cmdBuf )
Purpose:  return command string from ringbuffer  
Arguments: pointer to the command value location into which to place the numerical payload
Returns:  number of characters in the serial command.

		  Expected serial command format is, !SC:XXXX#
where '!' marks the start of a valid command, 'SC' is a 2 character command type, ':' is the 
command type and command value seperator, and XXXX is a ascii representation of a 16 bit hex 
value, and '#' marks end of command string. Thus, including the NULL terminator that the RX
ISR inserts, a valid command should have a length of 11 bytes.

Later on may add CRC information for validation of error free reception, useful when use RF link
*/
/**************************************************************************/

#if 0
uint8_t sc_getCmd()
{
	unsigned int cmdInt;
	uint8_t index = 0;
	unsigned char *cmdBuf;

	cmdBuf = gbuffer;

	do
	{
		cmdInt = uart_getc();
		*(cmdBuf + index) = 0xFF & cmdInt;
		index++;
	}while(cmdInt != UART_NO_DATA);

	/*Reduce count by 1 as don't want the increment that occurred when cmdInt = UART_NO_DATA*/
	index--;

	cmdLength = index;

	/*Re-enable the rx uart interrupt*/
	
	UART0_CONTROL |= 1<<RXCIE;

	return index;
}

#else

uint8_t sc_getCmd()
{
	uint16_t cmdInt;
	uint8_t index = 0;

	/*Stop the receive interrupt, until we've transferred the received '\r' terminated
	 serial string out of the ring buffer into the linear command buffer*/
	UART0_CONTROL |= 0<<RXCIE;

	do
	{
		cmdInt = uart_getc();
		linearBuffer[index] = 0xFF & cmdInt;
		index++;
	}while(cmdInt != UART_NO_DATA);

	/*Reduce count by 1 as don't want the increment that occurred when cmdInt = UART_NO_DATA*/
	index--;

	/*Re-enable the rx uart interrupt*/
	UART0_CONTROL |= 1<<RXCIE;
	

	cmdLength = index;
	return index;
}

#endif

#if 1
/*Each element of defined command format corresponds to 1 state in state machine.
Elements include: length of command, start character, field seperator character and it's position,
end of command character
*/
enum cmdResult_t sc_validateCmd(uint8_t *cmdType, uint16_t *cmdValue)
{
	enum cmdState_t cmdState;
	enum cmdResult_t cmdResult;

	cmdState = CHK_CMDLEN;
	cmdResult = CMD_INVALID;

	while(cmdState != CHK_FINISH)
	{
		switch(cmdState)
		{
			case CHK_CMDLEN:
				if (cmdLength != VALID_CMD_LEN)
				{
					
					cmdState = CHK_FINISH;
					//uart_puts_P("A\r\n");
				}
				else
				{
					
					cmdState = CHK_STARTCHAR;
					//uart_puts_P("B\r\n");
				}
				break;

			case CHK_STARTCHAR:
				if(linearBuffer[START_CHAR_POS] != START_CHAR)
				{
					
					cmdState = CHK_FINISH;
					//uart_puts_P("C\r\n");
				}
				else
				{
					
					cmdState = CHK_SEPERATOR;
					//uart_puts_P("D\r\n");
				}
				break;

			case CHK_SEPERATOR:
				if(linearBuffer[SEPERATOR_CHAR_POS] != SEPERATOR_CHAR)
				{
					
					cmdState = CHK_FINISH;
					//uart_puts_P("E\r\n");
				}
				else
				{
					
					cmdState = CHK_ENDCHAR;
					//uart_puts_P("F\r\n");
				}
				break;

			case CHK_ENDCHAR:
				if(linearBuffer[END_CHAR_POS] != END_CHAR)
				{
					
					cmdState = CHK_FINISH;
					//uart_puts_P("G\r\n");
				}
				else
				{
					
					cmdResult = CMD_VALID;
					cmdState = EXTRACT_FIELDS;
					/*Make the end character a NULL so that the value field can be treated as a string
					by the function atol() when extracting the fields*/
					//linearBuffer[END_CHAR_POS] = 0x0;
					//uart_puts_P("H\r\n");
				}
				break;

			case EXTRACT_FIELDS:
				*cmdType = linearBuffer[CMD_TYPE_CHAR_1_POS];
				*(cmdType+1) = linearBuffer[CMD_TYPE_CHAR_2_POS];
			/*	*cmdValue = atol(&linearBuffer[CMD_VALUE_CHAR_1_POS]);*/
			/*	*cmdValue = (uint16_t)strtoul(&linearBuffer[CMD_VALUE_CHAR_1_POS],NULL,16);*/
			/*	*cmdValue = asciiHexToUint(&linearBuffer[CMD_VALUE_CHAR_1_POS]);
				*cmdValue = asciiHexToUint(&linearBuffer[CMD_VALUE_CHAR_3_POS]);*/


				cmdState = CHK_FINISH;
				//uart_puts_P("I\r\n");
				break;
			default:
				break;
		}
	}/*end while*/

	return cmdResult;
}

#endif


uint8_t asciiHexToUint(uint8_t *s)
{
	uint8_t i;
	uint8_t tempByte = 0;
	//uint16_t resultInt = 0;


#if 1
	for(i=0;i<2;i++)
	{
		/*Does this character represent a numeric value betwee 0 and 9?*/
		if( *(s+i) < 0x3A)
		{
			
			//resultInt = resultInt | ( (*(s+i) - 0x30) << (12 - 4*i) );
			uart_puts_P("D\r\n");
			//uart_putc(*(s+i));
			
		}
		else
		{
			if( *(s+i) > 0x40 )
			{
				/*Assume for the moment then that it must be a letter between A and F*/
				//resultInt = resultInt | ( (*(s+i) - 0x41) << (12 - 4*i) );
				uart_puts_P("H\r\n");
				//uart_putc(*(s+i));
			
			}
			else
			{
				uart_puts_P("O\r\n");
			}
		}
		//uart_putc(*(s+i));
	}
#endif
	/*uart_putc(*s);
	uart_putc(*(s+1));
	uart_putc(*(s+2));
	uart_putc(*(s+3));*/


	return tempByte;
}


		
