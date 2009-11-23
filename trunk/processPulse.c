//
// processPulse.c
//
// Function containing the external pulse processing code
//
// Author: Richard C Clarke
// Date: March 2009
//


// includes

#include <stdlib.h>
#include <avr/io.h>

#include <inttypes.h>

#include "global.h"

#define KWH_CONST (uint32_t)360e6	/*the number of 0.01ms intervals in 1 hour*/
#define PULSES_PER_KWH (uint16_t)1600

#define MAX_KW 15
#define TIMER_TICK_RATE 3600 /*per second, determined by F_CPU and TIMER_CLK_DIV1024*/


/*#define MIN_TICKS (uint16_t)((float)(TIMER_TICK_RATE/(MAX_KW*PULSES_PER_KWH) )*TIMER_TICK_RATE)*/

#define MIN_TICKS 405	/*Corresponds to 20kW instantaneous load*/

extern volatile BOOL externalPulseFlag;
extern volatile uint16_t thisTimer1Count;
extern uint16_t minTimerTicks;

extern uint32_t localTimerTicksSum;
/*Keeps track of the total number of pulses counted since last reset or variable clear command*/
extern uint32_t totalPulseCount;
extern uint16_t localTimerTicksAvg;

extern uint16_t pulse_ticker;
extern uint16_t minTickError;

/*The number of LED pulses between sending latest measurements out serial port*/
extern	uint8_t averageWindow;




void processPulse()
{

	uint16_t localTimerTicks;

	/*Avoid any interrupts of this multibyte volatile variable read/write. If any interrupts occur 
	whilst we have them disabled they will be processed after we reenable them*/
	cli();
	localTimerTicks = thisTimer1Count;
	
	sei();

	/*TODO:DEBUG:RCC
	**Track the minimum interval between external interrupts
	*/
	if(localTimerTicks < minTimerTicks)
	{
		minTimerTicks = localTimerTicks;
	}

	/*Build in some protection against the rare event of a glitch causing two interrupts in quick sucession.
	If the value of localTimerTicks is less than some reasonable value then ignore it, i.e don't use it in the
	calculation of the next average value. A reasonable minimum value would be the time interval corresponding
	20kW peak instantaneous power, which is if sustained continuously over a period of an hour would result in
	20 x 1600 = 32000 pulses, which would thus have a pulse interval of 3600(sec)/32000 = 112.50ms. In terms of 
	AVR timer ticks this is 0.1125 x 3600 = 405 ticks */

	if(localTimerTicks >= MIN_TICKS)
	{

		pulse_ticker++;
		totalPulseCount++;
		/*tickRate_Hz = (F_CPU/prescaleDiv)*/
		/*time_ms = (localTimerTicks/tickRate_Hz)*1000

		Result here is time in milliseconds x 10, to give 0.1ms resolution
		in the calculation, otherwise can only represent times to integer numbers
		of 1ms increments. In reality due to truncation only get resolution to within
		about 0.3ms, however this is sufficient.*/

		localTimerTicksSum = localTimerTicksSum + localTimerTicks;
	}
	else
	{
		minTickError++;
	} 

	/*pulse_interval_ms_x_10 = (((uint32_t)localTimerTicks*10000)/tickRate_Hz);
	pulse_interval_sum = pulse_interval_sum + pulse_interval_ms_x_10;*/

	/*Would never expect to see a pulse interval of less than 150ms in normal operation.
	This equates to a pulse rate of 6.667Hz or 24000/hr or an instantaneous loading of 15kW.
	If we do measure an interval less than this then it's likely due to erroneous short
	duration pulses on the ext interrupt input, perhaps due to noise. These should be discarded.
	The summation above effectively achieves this*/
	

	/*Output an update of current measured values to the serial port every 10
	pulse_ticker increments*/
	if( (pulse_ticker >= (uint8_t)averageWindow) )
	{
		//debugInfoOut();
		
		/*Calculate the average pulse interval over the last 'averageWindow' pulses.
		This should reduce the impace of any very short duration pulse intervals measured
		due to noise or erroneous switching on the ext interrupt pin*/
		localTimerTicksAvg = (uint16_t)(localTimerTicksSum/averageWindow);

		#if 0
		uart_puts_P("{");
		utoa( localTimerTicksAvg, buffer, 10);
		uart_puts(buffer);
		uart_puts_P(", ");
		utoa( minTimerTicks, buffer, 10);
		uart_puts(buffer);
		uart_puts_P("}\r\n");
		#endif
		//pulse_interval_ms_x_10 = pulse_interval_sum/averageWindow;
		localTimerTicksSum = 0;

		

		pulse_ticker = 0;
	}

				
	externalPulseFlag = 0;
	
	//localTimerTicks = 0;


}
