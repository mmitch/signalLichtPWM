/*
 * ATmega168 code for Signal Board/Signal Licht
 *
 * Copyright (C) 2012 by  Christian Garbs <mitch@cgarbs.de>
 * licensed under the GNU GPL v3 or later
 *
 * what we do here:
 * - blue  LED is PB2 / pin 16 / OC1B which is output compare B of timer 1
 * - green LED is PB1 / pin 15 / OC1A which is output compare A of timer 1
 * - red   LED is PB0 / pin 14 which is not reachable via any timer
 *
 * OC1B and OC1A are directly controlled via hardware PWM by timer 1.
 * Timer 1 uses clock divisor 1024 so we don't need a counter variable
 * for slowdown and can do something useful on every interrupt.
 *
 * Timer 0 is used for soft-PWM for the red LED.
 * The hardware timer 0 is used for the comparison.
 * Overflow   interrupt sets PIN 14 to 0.
 * Comparison interrupt sets PIN 14 to 1.
 * This is not ideal, as on highest and lowest values there is very little
 * time to do anything.  Timer 0 uses a clock divisor to remedy this (at
 * least partially).
 *
 * == VERSION: ========================
 * This finally adds the red LED via software PWM.
 * Notice the additon of the _red variables and the red handling to the
 * existing interrupt routine of timer 1.
 * Soft PWM is realized via timer 0 which is set up in init() and has
 * two very simple interrupts routines on compare and on overflow.
 *
 */

// includes
#include <inttypes.h>
#include <avr/interrupt.h>

// variables
#define ARRAYLENGTH 8
uint8_t values[ARRAYLENGTH] = { 0, 192, 64, 255, 32, 192, 96, 128 };
uint8_t pos_blue     = 2;
uint8_t pos_green    = 0;
uint8_t pos_red      = 3;
uint8_t target_blue  = 0;
uint8_t target_green = 0;
uint8_t target_red   = 0;

/* initialize CPU, PWM, outputs and registers */
void init(void)
{
	// initial values from array
	target_green = values[pos_green];
	target_blue  = values[pos_blue];

	//// Timer 1 (16 bit) is used in 8 bit fast PWM mode
	//   output pins are set to 0 on each timer overflow
	//   and to 1 when the comparison is true
	//   PIN 16 uses output compare B, PIN 15 uses OCA

	// enable output at bottom and disenable on compare match (higher value -> brighter)
	TCCR1A |= (1<<COM1A1) | (0<<COM1A0);
	TCCR1A |= (1<<COM1B1) | (0<<COM1B0);

	// select fast 8 bit PWM mode
	TCCR1A |= (0<<WGM11) | (1<<WGM10);
	TCCR1B |= (0<<WGM13) | (1<<WGM12);
	
	// select clock divisor 1024
	TCCR1B |= (1<<CS12) | (0<<CS11) | (1<<CS10);

	// enable interrupt on overflow (after each PWM cycle)
	TIMSK1 |= (1<<TOIE1);

	// initialize output compare
	OCR1A = target_green;
	OCR1B = target_blue;

	//// Timer 0 (8 bit) is used in fast PWM mode
	//   overflow interrupt  enables red LED
	//   compare  interrupt disables red LED

	// disconnect output port OC0A (pin12), the red LED is elsewhere
	// also disconnect output port 0C0B, it is totally unused
	TCCR0A |= (0<<COM0A1) | (0<<COM0A0);
	TCCR0B |= (0<<COM0B1) | (0<<COM0B0);

	// select fast PWM mode
	TCCR0A |= (1<<WGM01) | (1<<WGM00);
	TCCR0B |= (0<<WGM02);
	
	// select clock divisor 64
	TCCR0B |= (0<<CS02) | (1<<CS01) | (0<<CS00);

	// enable interrupt on overflow (after each PWM cycle)
	TIMSK0 |= (1<<TOIE0);

	// enable interrupt on compare A
	TIMSK0 |= (1<<OCIE0A);

	// initialize output compare
	OCR1A = target_red;
	OCR1B = 0; // unused

	//// other stuff

	// configure pins for output
	DDRB  = (1<<PB2) | (1<<PB1) | (1<<PB0);

	// initialize output pins
	// PB0 = 0 -> no red
	PORTB = (0<<PB2) | (0<<PB1) | (0<<PB0);

	sei();
}

/* main loop */
int main(void)
{
	init();
	while(1);
}

/* interrupt on PWM overflow timer 1 */
ISR (TIMER1_OVF_vect)
{
	// green LED:
	if ( OCR1A < target_green ) {
		OCR1A++;
	} else if ( OCR1A > target_green ) {
		OCR1A--;
	} else {
		// get next value  (does not change the real value this cycle)
		pos_green++;
		if (pos_green == ARRAYLENGTH) {
			pos_green = 0;
		}
		target_green = values[ pos_green ];
	}

	// blue LED:
	if ( OCR1B < target_blue ) {
		OCR1B++;
	} else if ( OCR1B > target_blue ) {
		OCR1B--;
	} else {
		// get next value  (does not change the real value this cycle)
		pos_blue++;
		if (pos_blue == ARRAYLENGTH) {
			pos_blue = 0;
		}
		target_blue = values[ pos_blue ];
	}

	// red LED:
	if ( OCR0A < target_red ) {
		OCR0A++;
	} else if ( OCR0A > target_red ) {
		OCR0A--;
	} else {
		// get next value  (does not change the real value this cycle)
		pos_red++;
		if (pos_red == ARRAYLENGTH) {
			pos_red = 0;
		}
		target_red = values[ pos_red ];
	}
}


/* interrupt on PWM overflow timer 0 */
ISR (TIMER0_OVF_vect)
{
	// enable red LED
	PORTB = (1<<PB0);
}

/* interrupt on PWM compare timer 0 */
ISR (TIMER0_COMPA_vect)
{
	// disable red LED
	PORTB &= ~(1<<PB0);
}
