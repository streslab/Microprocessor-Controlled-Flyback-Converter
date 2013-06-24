/*---------------------------------------------------------------------*\
 * 
 *	Names:   Reed Slaby, Kevin Stauffer
 *	
 *	Created: SS2013
 *
 *	Purpose: This program is designed to regulate and control a flyback
 *			 converter.  The output voltage should be held at a user-
 *			 defined value while the input voltage and the load varies 
 *			 randomly.
 *
 *  Design Parameters:
 *			  Input Voltage:  24V - 36V
 *			 Output Voltage:   0V - 24V
 *			 Output Current: < 2A
 *
 *	Input:   PINA0 - Feedback Input
 *			 PIND2 - Voltage Up
 *			 PIND3 - Voltage Down
 *
 *	Output:  PORTB - LCD Data Bus
 *			 PIND0 - LCD Register Select
 *			 PIND1 - LCD Read/Write
 *			 PIND5 - PWM
 *			 PIND7 - LCD Enable
 *
\*---------------------------------------------------------------------*/

//Define Constants
#define ADC_FACTOR		2.96
#define CHAR_OFFSET		0x30
#define F_CPU			8000000
#define I_COUNT_CYCLES	488
#define I_LOWER_LIMIT  -500
#define I_UPPER_LIMIT	500
#define MAX_VOLTAGE		3000 //3000 = 30.00 volts
#define ADC_TO_D		14

#define SET_VOLTAGE_INCREMENT	50
#define SWITCH_DEBOUNCE_CYCLES	100
#define SWITCH_DEBOUNCE_MAX		1000

#define P_GAIN	4  //initial 0.8
#define I_GAIN	2.5 //initial 0.05
#define D_GAIN	0  //initial 0.01
#define T		2048 //time per pid sample (us)

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <tgmath.h>
#include <util/delay.h>

//Function Prototypes
void InitializeuC();
void LCDChar(char text, unsigned char cursorPosition);
void LCDCommand(unsigned char command);
void LCDData(unsigned char data);
void LCDInit();
void LCDString(char * text, unsigned char cursorPosition);
void LCDVoltage(int number, unsigned char cursorStartPos);

//Global Variables
volatile uint16_t tenbitadc;
volatile int previouserror = 0, ierror = 0;
volatile unsigned int switchdebounce = 0, setvoltage = 100, icounter = 0;

int main(void)
{
	//Initialize the Microcontroller
	InitializeuC();
	//Set up LCD
	LCDInit();
	LCDString(" ADC       SET  ",0x80);
	LCDString("00.00V    00.00V",0xC0);
	
	//Set up PID Initial Value
	previouserror = setvoltage - tenbitadc;

	while(1)
	{
		LCDVoltage(tenbitadc * ADC_FACTOR,0xC0);
		LCDVoltage(abs(ierror) * ADC_FACTOR,0x85);
		LCDVoltage(setvoltage,0xCA);
		_delay_ms(200);
	}	
}

//-----------------------------------------------------------------------
//This interrupt at the Timer0 Overflow.  Runs the PID Code
//Params: in
//-----------------------------------------------------------------------
ISR(TIMER0_OVF_vect) 
{
	//Assign local variables
	int error, pidoutput, derror = 0;
	
	//Increment Switch Debouncer
	if(switchdebounce <= SWITCH_DEBOUNCE_MAX)
		switchdebounce++;
	//Wipe the integral error every second	
	if(icounter >= I_COUNT_CYCLES)
		ierror = 0;
		
	//Calculate Errors
	error = setvoltage/ADC_FACTOR - tenbitadc;
	ierror += (error*T/10000)* I_GAIN;
	derror = (error - previouserror);
	
	//Keep I within predefined limits
	if(ierror >= I_UPPER_LIMIT)
		ierror = I_UPPER_LIMIT;
	else if(ierror <= I_LOWER_LIMIT)
		ierror = I_LOWER_LIMIT;

	//Get output by summing up errors multiplied with their Gains
	pidoutput = ((((P_GAIN*error)+(ierror)+(D_GAIN*derror))+tenbitadc)/ADC_TO_D)+.5;
		
	OCR1A = pidoutput;	
	previouserror = error;
	icounter++;
}

//-----------------------------------------------------------------------
//This interrupt triggers when the voltage up button is pressed.
//Params: in
//-----------------------------------------------------------------------
ISR(INT0_vect){
	if((setvoltage <= (MAX_VOLTAGE - SET_VOLTAGE_INCREMENT)) && 
		(switchdebounce >= SWITCH_DEBOUNCE_CYCLES))
	{
		setvoltage += SET_VOLTAGE_INCREMENT;
		switchdebounce = 0;
	}
}

//-----------------------------------------------------------------------
//This interrupt triggers when the voltage down button is pressed.
//Params: in
//-----------------------------------------------------------------------
ISR(INT1_vect){  
	if((setvoltage >= SET_VOLTAGE_INCREMENT) && 
		(switchdebounce >= SWITCH_DEBOUNCE_CYCLES)) 
	{
		setvoltage -= SET_VOLTAGE_INCREMENT;
		switchdebounce = 0;
	}
}

//-----------------------------------------------------------------------
//This interrupt triggers when the ADC conversion is finished.  It then
//stores the ADC value as an integer and restarts the conversion.
//Params: in
//-----------------------------------------------------------------------
ISR(ADC_vect)
{
	tenbitadc = ADCH<<8;  
	tenbitadc += ADCL;
	ADCSRA |= 1<<ADSC;  
}

//-----------------------------------------------------------------------
//This interrupt sets the registers to the correct values for our
//implementation.
//Params: none
void InitializeuC()
{
	//Disable Interrupts
	cli();
	//Set data direction for ports
	DDRA	= 0x00;
	DDRB	= 0xFF;
	DDRD	= 0xA3;
	//Set PORTA initial values
	PORTA	= 0x00;
	//Set timer options
	ICR1	= 0x090;
	OCR1A	= 0x05;
	TCCR0	= 0x03;
	TCCR1A	= 0xA2;
	TCCR1B	= 0x19;
	//Enable Interrupts
	ADCSRA	|= 1<<ADIE;
	GICR	|= 1<<INT0 | 1<<INT1;
	MCUCR	|= 1<<ISC01 | 1<<ISC00 | 1<<ISC11 | 1<<ISC10;
	TIMSK	|= 0x01;
	//Set up ADC
	ADCSRA	|= 1<<ADPS2 |  1<<ADSC |  1<<ADEN;
	ADMUX	= 0x40;
	//Reenable Interrupts
	sei();
}

//-----------------------------------------------------------------------
//This function sends a Character to the specified cursor position on the
//LCD Display.
//Params: in, in
//-----------------------------------------------------------------------
void LCDChar(char text, unsigned char cursorPosition)
{
	LCDCommand(cursorPosition);
	_delay_ms(1);
	LCDData(text);
}

//-----------------------------------------------------------------------
//This function sends a Register command to the LCD Display.
//Params: in
//-----------------------------------------------------------------------
void LCDCommand(unsigned char command)
{
	PORTB = command;
	PORTD &= ~0x03;
	PORTD |= 0x80;
	_delay_ms(1);
	PORTD &= ~0x80;
	_delay_ms(1);
}

//-----------------------------------------------------------------------
//This function sends a character to the display.
//Params: in
//-----------------------------------------------------------------------
void LCDData(unsigned char data)
{
	PORTB = data;
	PORTD &= ~0x02;
	PORTD |= 0x81;	
	_delay_ms(1);
	PORTD &= ~0x81;
}

//-----------------------------------------------------------------------
//This function initializes the LCD Display.
//Params: none
//-----------------------------------------------------------------------
void LCDInit()
{
	_delay_ms(32);
	//Function Set
	LCDCommand(0x3C);
	//Display On
	LCDCommand(0x0C);
	//Clear Display
	LCDCommand(0x01);
	_delay_ms(1);
	//Set Entry Mode
	LCDCommand(0x06);
}

//-----------------------------------------------------------------------
//This function sends a string value to the specified cursor position on 
//the LCD display.
//Params: ref, in
//-----------------------------------------------------------------------
void LCDString(char* text, unsigned char cursorPosition)
{
	LCDCommand(cursorPosition);
	while(*text)
	{
		LCDData(*text);
		text++;
	}
}

//-----------------------------------------------------------------------
//This function converts a float Voltage to characters and sends them to 
//the specified cursor position on the LCD Display.
//Params: in, in
//-----------------------------------------------------------------------
void LCDVoltage(int number, unsigned char cursorStartPos)
{
	int index;
	int digit;
	int arr[4];
	for (int i = 1; i <= 4; i++) 
	{
		 index = 4 - i;
		 digit = number / pow(10, index);
		 arr[i-1] = digit;
		 number = number - pow(10, index) * digit;
	}
	char hundredth = arr[3] + CHAR_OFFSET;
	char tenth = arr[2] + CHAR_OFFSET;
	char ones = arr[1] + CHAR_OFFSET;
	char tens = arr[0] + CHAR_OFFSET;
	
	LCDChar(tens,cursorStartPos);
	LCDChar(ones,cursorStartPos + 1);
	LCDChar(tenth,cursorStartPos + 3);
	LCDChar(hundredth,cursorStartPos + 4);
}