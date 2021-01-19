#include <msp430.h>
#include <libemb/conio/conio.h>
#include <libemb/serial/serial.h>

#include "colors.h" // colors array
#include "dtc.h"


/******
 *
 *    CONSTANTS
 *
 ******/

// large number of data structures should be here
// use `const` keyword                

//=================================== DIGIT 1 ===========================================
				 //DLD1B1D*
const char digit1_pattern1[] = { 0b00001000, 0b00001001, 0b00001000 }; 
const char digit1_pattern2[] = { 0b10101101, 0b00000100, 0b00000101 }; 
				 //***L**L*
			            //R           G           B
//=======================================================================================

//=================================== DIGIT 2/3 =========================================
				   //DLD1B1D*
const char digits23_pattern1[] = { 0b00001001, 0b00001001, 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001001, 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001001, 0b00001000, 0b00001000, 0b00001000 }; 
const char digits23_pattern2[] = { 0b00000000, 0b11100001, 0b10001000, 0b11000000, 0b01100001, 0b01000100, 0b00000100, 0b11100000, 0b00000000, 0b01000000, 0b00100000, 0b00000101, 0b00001100, 0b10000001, 0b00001100, 0b00101100 }; 
				   //***L**L*
			             // 0           1           2          3          4            5          6           7           8             9           A           B          C           D           E           F 
//=======================================================================================


/******
 *
 *    GLOBALS
 *
 ******/

// counters can go here
//======================================
char cursor = 0; // which digit is it on

// For choosing which led to change brightness for.
int color_counter = 0;

// For printing color to screen.
int print_counter    = 0;
int red_print        = 0;
int green_print      = 0;
int blue_print       = 0;
int color_math 	     = 0;
//======================================

/******l
 *
 *    INITIALIZATION
 *
 ******/
int main(void)
{
	//--------------- DEFAULT ---------------------
	/* WIZARD WORDS ***************************/
	WDTCTL   = WDTPW | WDTHOLD; // Disable Watchdog
	BCSCTL1  = CALBC1_1MHZ;     // Run @ 1MHz
	DCOCTL   = CALDCO_1MHZ;
	//---------------------------------------------

	//----------------- GPIO -----------------------
	// pin initialization
	P1DIR = BIT7|BIT5|BIT1|BIT0;
	P2DIR = -1;

	P1OUT = BIT7|BIT5|BIT1;
	P2OUT = 0;

	// Use P2.6 and P2.7 as GPIO
	P2SEL  &= ~(BIT6|BIT7);
	P2SEL2 &= ~(BIT6|BIT7);
	//---------------------------------------------

	//------------ PRINTING ------------
	// Call to allow printing to console
	serial_init(9600);

	// Set P1.1 back to regular GPIO
	P1SEL  &= ~BIT1;
	P1SEL2 &= ~BIT1;
	//----------------------------------
		
	//---------------- TIMER A0 -------------------
	// timer 0 initialization
	TA0CTL   = TASSEL_2|ID_0|MC_1;
	TA0CCTL0 = CCIE;
	TA0CCR0  = 1024;
	TA0CCTL1 = OUTMOD_7; 

	P2SEL |= BIT1;
	P2SEL |= BIT4;
	P1SEL |= BIT6;
	//--------------------------------------------- 

	//---------------- TIMER A1 -------------------
	// timer 1 initialization
	TA1CTL   = TASSEL_2|ID_3|MC_1;
	TA1CCTL0 = CCIE;
	TA1CCR0  = 1024; 
	TA1CCTL1 = OUTMOD_7;
	TA1CCTL2 = OUTMOD_7;
	//---------------------------------------------

	//------------ BUTTON INTERRUPT ------------
	// Execute once, button stuff //
	// For old boards
        P1DIR = ~BIT3;//mov.b #~BIT3, &P1DIR 
	P1REN =  BIT3;//mov.b #BIT3, &P1REN
	P1OUT =  BIT3;//mov.b #BIT3, &P1OUT
	// For interrupts
	P1IE  |=  BIT3;//bis.b #BIT3, &P1IE  ; OR
	P1IES |=  BIT3;//bis.b #BIT3, &P1IES
	P1IFG &= ~BIT3;//bic.b #BIT3, &P1IFG ; clear 
	//------------------------------------------

	//------------ ADC10CTL POTENTIOMETER ----------------------
	initialize_dtc(INCH_4, &TA0CCR1); // or &TA1CCR1 or &TA1CCR2
	// value in potentiometer is stored in TA0CCR1
	// the brightness will be changed by the call to initialize_dtc()
	//----------------------------------------------------------

/******
 *
 *    MAIN LOOP (THERE ISN'T ONE)
 *
 ******/

	//---------------- SLEEP-- --------------------
	// go to sleep with interrupts enabled
	__bis_SR_register(LPM1_bits|GIE);
	//---------------------------------------------
	// Do not put code past here

	// never return
	return 0;
}

/******
 *
 *    INTERRUPTS
 *
 ******/
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer0 (void) // Timer Interrupt for the Display
{

	//-------------------------- Potentiometer value changing --------------------------
	// Turn potentiometer 10 bit value to 8 bit value
	int potentiometer_to_8bit; // potentiometer value in 8 bits

	// Potentiometer value for red led
	if (color_counter == 0) {

		potentiometer_to_8bit = TA0CCR1 >> 2;
	}

	// Potentiometer value for green led
	else if (color_counter == 1) {

		potentiometer_to_8bit = TA1CCR1 >> 2;
	}

	// Potentiometer value for blue led
	else if (color_counter == 2) {

		potentiometer_to_8bit = TA1CCR2 >> 2;
	}

	int bottom_4bit = potentiometer_to_8bit & 0b1111; // Mask to get 4 digits on right
	int top_4bit    = potentiometer_to_8bit >> 4; // Right shift to get 4 digits on left 
	//----------------------------------------------------------------------------------

	//--------------- Turn off digits in 7-segment -----------------
	// 1.) Turn off all digits
	P1OUT &= ~(BIT7|BIT5|BIT1); // bic.b BIT7|BIT5|BIT1
	//--------------------------------------------------------------

	//---------- Choose digit value in 7-segment --------------
	// 2.) Configure segment pins for correct pattern.
	// If digit 1
	if (cursor == 0) {

		P1OUT = digit1_pattern1[color_counter]; 
		P2OUT = digit1_pattern2[color_counter];
	}
	// If digit 2
	else if (cursor == 1) {

		P1OUT = digits23_pattern1[top_4bit]; 
		P2OUT = digits23_pattern2[top_4bit];
	}
	// If digit 3
	else {
	
		P1OUT = digits23_pattern1[bottom_4bit]; 
		P2OUT = digits23_pattern2[bottom_4bit];
	}
	//---------------------------------------------------------

	//--------- Choose which digit is on in 7-segment ----------
	// 3.) Turn on next digit (only 1 digit is on at a time)
	if (cursor == 0) {
		P1OUT |= BIT1; // bis.b BIT7 (P1.7) 3rd digit
	} 
	else if (cursor == 1) {
		P1OUT |= BIT5; // bis.b BIT5 (P1.5) 2nd digit
	}
	else {
		P1OUT |= BIT7; // bis.b BIT1 (P1.1) 1st digit
	}
	//----------------------------------------------------------
	
	//-------- Update digit counter ---------
	// move ahead cursor for next time
	cursor++;
	cursor %= 3;
	//---------------------------------------
	
	//------------ Print color to console ---------------
	if (print_counter == 1000) {

		color_math = (blue_print << 6) + (green_print << 3) + red_print;
		cio_printf("%s\n\r", colors[color_math]);
	}

	print_counter++;

	// Print every 1 seconds.
	if (print_counter > 1000) {

		print_counter = 0;
	}
	//---------------------------------------------------

	// 4.) leave the interrupt
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer1 (void) // Timer interrupt for printing the color to the screen
{

	//--------------------------------
	// RED LED
	if (color_counter == 0) {

		red_print = TA0CCR1 >> 7; // Turn 10 bit to 3 bit
	}

	// GREEN LED
	else if (color_counter == 1) {

		green_print = TA1CCR1 >> 7; // Turn 10 bit to 3 bit
	}

	// BLUE LED
	else if (color_counter == 2) {

		blue_print = TA1CCR2 >> 7;
	}
	//---------------------------------	
}

#pragma vector=PORT1_VECTOR
__interrupt void button (void) // Button Interrupt
{

	//------------------------
	color_counter++;

	if (color_counter > 2) {
		
		color_counter = 0;
	}
	//------------------------

	//---------------------------------------
	// Change red led
	if (color_counter == 0) {

		initialize_dtc(INCH_4, &TA0CCR1);
	}

	// Change green led
	else if (color_counter == 1) {

		initialize_dtc(INCH_4, &TA1CCR1);
	}

	// Change blue led
	else if (color_counter == 2) {

		initialize_dtc(INCH_4, &TA1CCR2);
	}
	//---------------------------------------

	//-------------------------------------------------------
	// button debounce routine
	while (!(BIT3 & P1IN)) {} // is finger off of button yet?
	__delay_cycles(32000);    // wait 32ms
	P1IFG &= ~BIT3;           // clear interrupt flag
	//-------------------------------------------------------
}
