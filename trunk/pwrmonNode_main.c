//
// pwrmonNode_main.c
//
// The main file containing the kernel of the Smart Power Monitor 
// Remote Node.
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
#include "uartsw_tx.h"
#include "serialcommand_rcc.h"

//u08 UART_NL[] = {0x0d,0x0a,0};

#define SMALL_MEMORY 0
#define DEBUG_SERIAL 1
#define SW_UART_TX	 2
#define HW_UART_TX	 3

#define UART_MODE SW_UART_TX


// defines
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PINB
#define LED3     PINB3
#define LED4     PINB5

/*INT0 is on PORTD, PD2*/
#define PULSE_INT DDRD

#define UPDATE_RATE 10

//#define F_CPU 8000000
#define UART_BAUD_RATE      9600 


/*#####################################
** Command Flag definitions, bit position
*/
#define ON_CHANGE 0
#define ON_QUERY 1
#define SEND_TOTAL_COUNT 2




/*Prototype*/
void myTimer1IntHandler(void);
void debugInfoOut(void);
void debugCSVInfoOut(void);
void sendTotalCount();
void ports_init(void);
void processPulse();


extern volatile unsigned char serCmndReady;



/*Global volatile variables, modifiable by ISR's. Volatile variables larger than 1 byte must be
read and written in one atomic operation. Interrupts must either be disabled or a local copy made
of the volatile variable before using it in an operation*/
volatile BOOL timerRollOverFlag;
volatile BOOL externalPulseFlag; 
volatile uint16_t thisTimer1Count;
//uint16_t localTimerTicks;

/*For Debug, keep track of the minimum duration seen on PD2 between external interrupts.
This number is in terms of number of ticks of a counter being incremented at the rate of 
3600Hz*/
uint16_t minTimerTicks;

//uint16_t tickRate_Hz;
uint16_t prescaleDiv;
uint16_t timerVal;
//uint32_t pulse_interval_sum;
uint32_t localTimerTicksSum;
uint16_t localTimerTicksAvg;

/*The number of LED pulses between sending latest measurements out serial port*/
uint8_t averageWindow;


uint16_t pulse_ticker;
uint16_t minTickError;


/*Keeps track of the total number of pulses counted since last reset or variable clear command*/
uint32_t totalPulseCount;

char buffer[12];

//
// main function
//
int main(void) 
{


	
	uint16_t tickRate_Hz;
	uint8_t commandCode[2];
	uint16_t cmdValue;
	uint8_t measureDataChange;
	uint8_t commandFlags;
	uint8_t commandLength;

		

	ports_init();

	// initialize our libraries
	/*
     *  Initialize UART library, pass baudrate and AVR cpu clock
     *  with the macro 
     *  UART_BAUD_SELECT() (normal speed mode )
     *  or 
     *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
     */
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); 
#if UART_MODE==SW_UART_TX
	uartswInit_Tx();
#endif
	// initialize the timer system, enables global interrupts.
	
	//timer1Init();
	/*Timer clocked at F_CPU/1024*/
#if SMALL_MEMORY
	/* Set up timer 1, 16 bit timer, use prescaler of 1024*/
	TCCR1B |= (1 << CS12) | (0 << CS11) | (1 << CS10); 

	/* Enable Timer1 overflow interrupt*/
	TIMSK1 |= (1 << TOIE1);  
#else
	timer1SetPrescaler(TIMER_CLK_DIV1024);
	cbi(TIMSK1, TOIE1);						// disable TCNT1 overflow
	//timerAttach(1, myTimer1IntHandler );
#endif
	timerRollOverFlag = 0;
	externalPulseFlag = 0;
	/*Counts the pulses from the LED pulse detector, after every 160 increment the 
	0.1 decimal KWh counter*/
	pulse_ticker = 0;

	averageWindow = UPDATE_RATE;
	measureDataChange = 0;
	commandFlags = 0;
	totalPulseCount = 0;
	minTimerTicks = 65535; /*Equiv to 65535/3600 = 18.204 sec*/
	localTimerTicksSum = 0;
	minTickError = 0;


	/*********************************************
	* Test Timer Ticks to Millisecond Calculation
	**********************************************/
	/*Result here is time in milliseconds x 10, to give 0.1ms resolution
	in the calculation, otherwise can only represent times to integer numbers
	of 1ms increments*/
	tickRate_Hz = (F_CPU/timer1GetPrescaler());

	
	// enable global interrupts
	sei();

	/*
     *  Transmit string to UART
     *  The string is buffered by the uart library in a circular buffer
     *  and one character at a time is transmitted to the UART using interrupts.
     *  uart_puts() blocks if it can not write the whole string to the circular 
     *  buffer
     */
    
    /*
     * Transmit string from program memory to UART
     */
    //uart_puts_P("PowerMeterMonitor\r\n");
	uartswSendByte("G");

	/*CSV Column headings*/
	//uart_puts_P("(totalCount,Avged Interval, Min Interval)\r\n");
    
  

	/*Main kernel loop*/
	while(1) 	    /* Forever */
	{
		/*Check to see if we've received a command character from serial port
		May need to implement a command processing state machine eventually*/
		//commandChar = uart_getc();
		if (serCmndReady)
		{
			/*Process the Command, for now just send out the total measurement counts*/
			commandLength = sc_getCmd();

			if( sc_validateCmd(&commandCode[0], &cmdValue) == CMD_VALID )
			{
				

				//ultoa( cmdValue, buffer, 10);
				//uart_puts(buffer);
				/*For the moment if we receive a command conforming to the required format then
				send a string containing current measurement results*/
				//sendTotalCount();

				/*if(commandCode[0] == 'S')
				{
					uart_putc(commandCode[1]);
				}*/

			}
			else
			{
				/*Make sure nothing happens*/
				//ultoa( commandLength, buffer, 10);
				//uart_puts(buffer);
				uart_puts_P("IV\r\n");
				commandCode[0] = 0x0;
			}
		
			serCmndReady = 0;
			
			
		}/*if (serCmndReady)*/

		if(externalPulseFlag == 1)
		{
			processPulse();
		}
		else
		{
			/*Only action serial command if we haven't just processed an external pulse event, so as to
			reduce amount of processing we have to do in a given loop. Processing of serial commands
			isn't time critical whereas processing of the external pulse event is.*/
			switch ((uint8_t)commandCode[0])
			{

				/*Reset class of command*/
				case 'R':
					switch((uint8_t)commandCode[1])
					{
						/*Reset All*/
						case 'A':
							uart_puts_P("RA\r");
							totalPulseCount = 0;
							minTimerTicks = 65535;
							localTimerTicksSum = 0;
							minTickError = 0;
							break;
						/*Reset Minimum Interval measurement only*/
						case 'M':
							uart_puts_P("RM\r");
							minTimerTicks = 65535;
							break;
						/*Reset Error counter that determines how many intervals with a 
						duration less than minimum expected have been seen*/
						case 'E':
							uart_puts_P("RE\r");
							minTickError = 0;
							break;
						
						default:
							break;
					}

					commandCode[0] = 0x0;

					break;



				/*Get class of command*/
				case 'G':
					switch((uint8_t)commandCode[1])
					{
						case 'A':
							sendTotalCount();
							break;
						case 'M':
							utoa( minTimerTicks, buffer, 10);
							uart_puts(buffer);
							uart_puts_P("\r\n");
							break;

						case 'E':
							utoa( minTickError, buffer, 10);
							uart_puts(buffer);
							uart_puts_P("\n\r");
							break;
						
						default:
							break;
					}

					commandCode[0] = 0x0;

					break;


				default:	
					break;
			}/*end switch ((uint8_t)commandCode[0])*/

		} /*if(externalPulseFlag == 1)*/
		

	
	}/*end while(1)*/

   
}



#if 1
void sendTotalCount()
{
	
	uart_puts_P("totalTicks,");
	/*totalPulseCount can be directly converted to total kWh consumed, just divide by 1600*/
	ultoa( totalPulseCount, buffer, 10);
	uart_puts(buffer);
	uart_puts_P(",");
	/*localTimerTicksAvg is the number of timer ticks (each tick currently configured to happen every 1/3600 sec),
	between rising edges of the power meter LED pulse input to the AVR, averaged over a set number of pulses, determined
	by the constant UPDATE_RATE*/
	utoa( localTimerTicksAvg, buffer, 10);
	uart_puts(buffer);
	uart_puts_P(",");
	/*minTimerTicks keeps track of the minimum interval (in integer numbers of 1/3600 sec) measured between Power Meter
	LED flashes. This would correspond to a time of maximum household power draw. Currently this value is an 'all time'
	minimum value, i.e the minimum since the last AVR reset or counter reset. This may not be particularly useful as
	the PC logging app could keep track of such things, particularly if we also output the current non averaged 
	instantaneous pulse interval measurement too*/
	utoa( minTimerTicks, buffer, 10);
	uart_puts(buffer);
	uart_puts_P("\r\n");


}
#endif

#if defined (__AVR_ATtiny2313__)
void ports_init()
{
	// interrupt on INT0 pin rising edge (sensor triggered) 
  	MCUCR = (1<<ISC01) | (1<<ISC00);
	//MCUCR = (1<<ISC01);

  	// turn on interrupt 0!
  	GIMSK  = _BV(INT0);


	// set LED pin to output and switch on LED
    LED_DDR |= _BV(LED3);
    LED_PORT &= ~_BV(LED3);

	LED_DDR |= _BV(LED4);
    LED_PORT &= ~_BV(LED4);

	/*Configure PD2 with weak pull up, helps prevent glitching from external EMI and glitches*/
	PORTD = (1 << PD2);
	DDRD = DDRD & ~_BV(PIND2);
}
#elif defined (__AVR_ATmega168__)
void ports_init()
{
	// External Interrupt Control Register A,
	//interrupt on INT0 pin rising edge (sensor triggered) 
  	EICRA = (1<<ISC01) | (1<<ISC00);
	//MCUCR = (1<<ISC01);

  	// turn on external interrupt 0!
  	EIMSK  = _BV(INT0);


	// set LED pin to output and switch on LED
    LED_DDR |= _BV(LED3);
    LED_PORT &= ~_BV(LED3);

	LED_DDR |= _BV(LED4);
    LED_PORT &= ~_BV(LED4);

	/*Configure PD2 with weak pull up, helps prevent glitching from external EMI and glitches*/
	PORTD = (1 << PD2);
	DDRD = DDRD & ~_BV(PIND2);
}
#endif		

/*External pulse interrupt on PD2*/
SIGNAL (SIG_INTERRUPT0)
{ 
    
	
	/* Disable interrupts */

	cli();
	thisTimer1Count = TCNT1;
	
	TCNT1 = 0;
	

	

	/*Toggles bit 0 in the MCU Control Register. This is responsible for determining whether INT0 is
	rising edge or falling edge triggered. By flipping between them the interrupt will be triggered on
	both rising and falling edge*/
	//MCUCR = MCUCR ^ _BV(ISC00);

	externalPulseFlag = 1;
	
	sei();

	LED_PORT = LED_PORT ^ _BV(LED4);	//toggle LED

}


/*service timer1 interrupts*/
#if 0
SIGNAL (SIG_OVERFLOW1)     /* signal handler for Timer 1 over flow*/
{
/*The timer is in free run when it rolls over time has passed.
Increment roll over counter, An interrupt comes every second.
Maintain the times structure*/
	LED_PORT = LED_PORT ^ _BV(LED);	//toggle LED

	timerRollOverFlag = 1;
}

#if 0
void myTimer1IntHandler(void)
{
/*The timer is in free run when it rolls over time has passed.
Increment roll over counter.  This timer is incrementing at a rate of F_CPU/timer1GetPrescaler(),
which makes it 3600Hz with F_CPU = 3686400Hz, and prescaler of 1024.*/
	LED_PORT = LED_PORT ^ _BV(LED3);	//toggle LED
	timerRollOverFlag = 1;

	
}
#endif
#endif

//
// end of file toggle.c
////////////////////////////////////////////////////////////
