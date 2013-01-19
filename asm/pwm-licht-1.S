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
 * This does not do much yet:
 * It dims blue + green to different levels, see ((A)) and ((B)).
 *
 */

// includes
.NOLIST
.INCLUDE "m168Pdef.inc"
.LIST

/* initialize CPU, PWM, outputs and registers */
init:

	; initialize stack
	LDI R16,HIGH(RAMEND)
	OUT SPH,R16
	LDI R16,LOW(RAMEND)
	OUT SPL,R16

	//// Timer 1 (16 bit) is used in 8 bit fast PWM mode
	//   output pins are set to 0 on each timer overflow
	//   and to 1 when the comparison is true
	//   PIN 16 uses output compare B, PIN 15 uses OCA

	; read extended IO ports to registers
	LDS R16,PRR
	LDS R17,TCCR1A
	LDS R18,TCCR1B
	LDS R19,TIMSK1

	; modify the values via the registers

	SBR R16,1<<PRTIM1	; disable powersave for timer/counter 1
						; (R16 = PRR)
	;;; TODO: enable powersaving for the others?

	// enable output at bottom and disenable on compare match (higher value -> brighter)
	CBR R17,0<<COM1A0	; (R17 = TCCR1A)
	SBR R17,1<<COM1A1
	CBR R17,0<<COM1B0
	SBR R17,1<<COM1B1

	// select fast 8 bit PWM mode
	SBR R17,1<<WGM10	; (R17 = TCCR1A)
	CBR R17,0<<WGM11
	SBR R18,1<<WGM12	; (R18 = TCCR1B)
	CBR R18,0<<WGM13

	// select clock divisor 1024
	SBR R18,1<<CS10		; (R18 = TCCR1B)
	CBR R18,0<<CS11		
	CBR R18,1<<CS12

	// enable interrupt on overflow (after each PWM cycle)
	SBR R19,1<<TOIE1	; (R19 = TIMSK1)


	; write modified registers back to extended IO ports
	STS PRR,R16
	STS TCCR1A,R17
	STS TCCR1B,R18
	STS TIMSK1,R19


	// initialize output compare
	; remember: LDI only works on R16-R31, R0 won't work!
	LDI R16,140			; medium green   ((A))
	STS OCR1AL,R16
	LDI R16,32			; dark blue      ((B))
	STS OCR1BL,R16

	//// other stuff

	// configure pins for output
	SBI DDRB, DDB0
	SBI DDRB, DDB1
	SBI DDRB, DDB2

	// initialize output pins
	// PB0 = 0 -> no red
	CBI PORTB, PB0
	CBI PORTB, PB1
	CBI PORTB, PB2

	; don't enable interrupts yet
	; we have no callback set up (no interrupt table)
	; and the timer works fine (enabling/disabling pins)
	; without any special user code/interrupt routine
	; SEI


/* main loop */
main:
	RJMP main
