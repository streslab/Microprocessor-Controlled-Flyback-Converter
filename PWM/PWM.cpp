/*---------------------------------------------------------------------*\
 * 
 *	Names:   Reed Slaby, Kevin Stauffer
 *	
 *	Created: SS2013
 *
 *	Purpose: This program is designed to regulate and control a flyback
 *			 converter.  The output voltage should be held at a user-
 *			 defined value while the input voltage varies randomly.
 *
 *	Input:   PIND2 - Voltage Up
 *			 PIND3 - Voltage Down
 *
 *	Output:  PIND5 - PWM
 *			 PORTB - LCD Data Bus
 *			 PIND0 - LCD Register Select
 *			 PIND1 - LCD Read/Write
 *			 PIND7 - LCD Enable
 *
\*---------------------------------------------------------------------*/

#define F_CPU 8000000
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

void LCDInit();
void LCDCommand(unsigned char command);
void LCDData(unsigned char data);

int main(void)
{

	//Disable Interrupts
	cli();
	DDRD = 0xA3;
	DDRB = 0xFF;
	//Set counter options
	TCCR1A = 0xA2;
	TCCR1B = 0x19;
	//Set TOP = ICR1 for 24.5kHz
	ICR1 = 0x14D;
	//Arbitrarily Set OCR1A (Duty Cycle)
	OCR1A = 0x05;
	//Enable INT0 and INT1
	GICR = 1<<INT0 | 1<<INT1;
	//Set Falling Edge Trigger for Interrupts
	MCUCR = 1<<ISC01 | 1<<ISC00 | 1<<ISC11 | 1<<ISC10;
	//set ADC prescaler to division of 16, so at a clk f of 8Mhz, ADC speed is 500kHz
	ADCSRA |= 1<<ADPS2; //see table 85 in datasheet for prescaler selection options
	//set voltage reference as AVCC, should be 5volts?  see page 208
	//we should maybe change this to a more stable reference?
	ADMUX |= 1<<REFS0;
	//enable ADC interrupt
	ADCSRA |= 1<<ADIE;
	//enable the ADC
	ADCSRA |= 1<<ADEN;
	//Re-enable Interrupts
	sei();
	
	LCDInit();
	
    while(1)
    {
		
    }
}

//This interrupt takes an input from a button and increases the duty
//cycle.
//Params: in
ISR(INT0_vect){
	if(OCR1A <= (ICR1 - 1))

		OCR1A += 0x01;

}

//This interrupt takes an input from a button and decreases the duty
//cycle.
//Params: in
ISR(INT1_vect){  
	if(OCR1A > 0x01)

		OCR1A -= 0x01;

}

ISR(ADC_vect) //ADC interrupt vector function
{
	uint8_t theLowADC = ADCL;  //assign the variable theLowADC as the value in the register ADCL
	uint16_t theTenBitResults = ADCH<<8;  //assign the variable theTenBitResults as the value in ADCH shifted 8 left.
	//Display the value on the Display
	ADCSRA |=1<<ADSC;  //start ADC conversion
}

void LCDInit()
{
	_delay_ms(32);
	LCDCommand(0x38);
	LCDCommand(0x0C);
	LCDCommand(0x01);
	_delay_ms(1);
	LCDCommand(0x04);
}

void LCDCommand(unsigned char command)
{
	PORTB = command;
	PORTD |= 1<<PIND7;
	PORTD &= ~0x03;
	_delay_ms(1);
	PORTD &= ~1<<PIND7;
	_delay_ms(1);
}

void LCDData(unsigned char data)
{
	PORTB = data;
	PORTD |= (1<<PIND7 | 1<<PIND1);
	PORTD &= ~0x01;
	_delay_ms(1);
	PORTD &= ~0x80;
	_delay_ms(1);
}