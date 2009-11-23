/*! \file uartsw2.c \brief Interrupt-driven Software UART Driver. */
//*****************************************************************************
//
// File Name	: 'uartsw2.c'
// Title		: Interrupt-driven Software UART Driver
// Author		: Pascal Stang - Copyright (C) 2002-2004
// Created		: 7/20/2002
// Revised		: 4/27/2004
// Version		: 0.6
// Target MCU	: Atmel AVR Series (intended for the ATmega16 and ATmega32)
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "timer.h"
#include "uartsw_Tx.h"

// Program ROM constants

// Global variables

// uartsw transmit status and data variables
static volatile u08 UartswTxBusy;
static volatile u08 UartswTxData;
static volatile u08 UartswTxBitNum;

// baud rate common to transmit and receive
static volatile u08 UartswBaudRateDiv;


// functions

//! enable and initialize the Tx software uart
void uartswInit_Tx(void)
{
    // initialize the buffers
	//uartswInitBuffers();
	// initialize the ports
	sbi(UARTSW_TX_DDR, UARTSW_TX_PIN);
	
	// initialize baud rate
	uartswSetBaudRate(9600);
	
	// setup the transmitter
	UartswTxBusy = FALSE;
	// disable OC2 interrupt
	cbi(TIMSK, OCIE2);
	// attach TxBit service routine to OC2
	timerAttach(TIMER2OUTCOMPARE_INT, uartswTxBitService);
		

	// turn on interrupts
	sei();
}



//! turns off software UART
void uartswOff_Tx(void)
{
	// disable interrupts
	cbi(TIMSK, OCIE2);

	// detach the service routines
	timerDetach(TIMER2OUTCOMPARE_INT);
	
}

void uartswSetBaudRate(u32 baudrate)
{
	u16 div;

	// set timer prescaler
	if( baudrate > (F_CPU/64L*256L) ) 
	//CHANGE THE ABOVE LINE IF CLOCK FREQ OF AVRLINX BOARD DIFFERS
	//FROM AVRSAT
	{
		// if the requested baud rate is high,
		// set timer prescalers to div-by-64
		timer2SetPrescaler(TIMERRTC_CLK_DIV64);
		
		div = 64;
	}
	else
	{
		// if the requested baud rate is low,
		// set timer prescalers to div-by-256
		timer2SetPrescaler(TIMERRTC_CLK_DIV256);
		
		div = 256;
	}

	// calculate division factor for requested baud rate, and set it
	//UartswBaudRateDiv = (u08)(((F_CPU/64L)+(baudrate/2L))/(baudrate*1L));
	//UartswBaudRateDiv = (u08)(((F_CPU/256L)+(baudrate/2L))/(baudrate*1L));
	UartswBaudRateDiv = (u08)(((F_CPU/div)+(baudrate/2L))/(baudrate*1L));
}



void uartswSendByte(u08 data)
{
	// wait until uart is ready
	while(UartswTxBusy);
	// set busy flag
	UartswTxBusy = TRUE;
	// save data
	UartswTxData = data;
	// set number of bits (+1 for stop bit)
	UartswTxBitNum = 9;
	
	// set the start bit
	cbi(UARTSW_TX_PORT, UARTSW_TX_PIN);//changed to cbi -JGM
	// schedule the next bit
	outb(OCR2, inb(TCNT2) + UartswBaudRateDiv); 
	// enable OC2 interrupt
	sbi(TIMSK, OCIE2);
}



void uartswTxBitService(void)
{
	if(UartswTxBitNum)
	{
		// there are bits still waiting to be transmitted
		if(UartswTxBitNum > 1)
		{
			// transmit data bits (inverted, LSB first)
			if( !(UartswTxData & 0x01) )
				cbi(UARTSW_TX_PORT, UARTSW_TX_PIN);//changed to cbi -JGM
			else
				sbi(UARTSW_TX_PORT, UARTSW_TX_PIN);//changed to sbi -JGM
			// shift bits down
			UartswTxData = UartswTxData>>1;
		}
		else
		{
			// transmit stop bit
			sbi(UARTSW_TX_PORT, UARTSW_TX_PIN);//changed to sbi -JGM
		}
		// schedule the next bit
		outb(OCR2, inb(OCR2) + UartswBaudRateDiv); //WHAT IS INB?
		// count down
		UartswTxBitNum--;
	}
	else
	{
		// transmission is done
		// clear busy flag
		UartswTxBusy = FALSE;
		// disable OC2 interrupt
		cbi(TIMSK, OCIE2);
	}
}


