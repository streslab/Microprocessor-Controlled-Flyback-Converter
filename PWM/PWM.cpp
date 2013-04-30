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
 *	Input:   Voltage Up, Voltage Down, Over-Current, Feedback
 *
 *	Output:  Voltage Display, PWM
 *
\*---------------------------------------------------------------------*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
	int cnt = 0;
	
	//Disable Interrupts
	cli();
	//Set PIND5(PWM) to Output and PIN3:2(INT1:0) to Input
	DDRD = 0b00100000;
	//Set counter options
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011001;
	//Set TOP = ICRdeasflhdlb1 for 24.5kHz
	ICR1 = 0x14D;
	//Arbitrarily Set OCR1A (Duty Cycle)
	OCR1A = 0x05;
	//Enable INT0 and INT1
	GICR = 1<<INT0 | 1<<INT1;
	//Set Falling Edge Trigger for Interrupts
	MCUCR = 1<<ISC01 | 1<<ISC00 | 1<<ISC11 | 1<<ISC10;
	//Re-enable Interrupts
	sei();
	
    while(1)
    {
		_delay_us(1);
		if(cnt%2 = 0)
			OCR1A++;
		else
			OCR1A--;
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
