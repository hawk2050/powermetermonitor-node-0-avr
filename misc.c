/* Miscellaneous Code snippets*/

#if 0
	/*Configure Timer 0 prescaler*/
	timer0SetPrescaler( TIMER_CLK_DIV1024 );	// set prescaler
	outb(TCNT0, 0);							// reset TCNT0

	/*Configure the Output Compare 0 unit, set mode of operation,
	COM0A1:0: Compare Match Output A Mode, set to Normal. This disconnects
	the Compare match unit from the OC0A pin*/
	cbi(TCCR0A,COM1A1);
	cbi(TCCR0A,COM1A0);

	/*Bits 1:0 – WGM01:0: Waveform Generation Mode, set to mode 2, which
	will Clear Timer on Compare match. The WGM02 bit is in the TCCR0B 
	register*/ 
	cbi(TCCR0B,WGM02);
	sbi(TCCR0A,WGM01);
	cbi(TCCR0A,WGM00);

	/*Set the value of the OCR0A register. This is the value that when matched
	by the Timer0 TCNT0 register a match occurs and an interrupt will be 
	generated. In this application we want this match to happen every 1ms.
	This is the resolution of our internal timer for marking the start of
	external pulses. The Timer0 increments at a rate of F_CPU/1024 = 3600 ticks
	per second*/
	OCR0A = 

	// enable Timer/Counter0 Output Compare Match A interrupt
	sbi(TIMSK, OCIE0A);


	/*Power Calculations*/

				/*Do all the calculations on the PC instead of the AVR*/
				KWh_count_x_10 = (totalPulseCount/(PULSES_PER_KWH/10));
				pulse_interval_ms_x_10 = (((uint32_t)localTimerTicksAvg*10000)/tickRate_Hz);
				//pulse_interval_sum = pulse_interval_sum + pulse_interval_ms_x_10;

				/*Do the calculation to extrapolate this to an instantaneous power measure*/
				pulses_per_hr_x_10 = KWH_CONST/(pulse_interval_ms_x_10);
				kw_load_x_10 = (uint16_t)(pulses_per_hr_x_10/PULSES_PER_KWH);

				
				/*Only bother sending values if they've changed since last time*/
				if( (kw_load_x_10 != prev_kw_load)|| (KWh_count_x_10 != prev_KWh_count) )
				{
					
					measureDataChange = 1;
				}


				prev_kw_load = kw_load_x_10;
				prev_KWh_count = KWh_count_x_10;

#endif
