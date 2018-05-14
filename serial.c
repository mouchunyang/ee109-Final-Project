#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "serial.h"
#include "rangefinder.h"

#define BUZZ_FREQUENCY 500
#define PERIOD_US 1000000/BUZZ_FREQUENCY //2000us

extern volatile int RX_startflag;
extern volatile int RX_endflag;
extern volatile char bufff[10];
extern volatile int bufff_count;

volatile int buzz_count=0;

void play_note()
{
	TCCR0A |= (1 << WGM01);   // Set the mode for "Clear Timer on Compare" (CTC)
 
    TIMSK0 |= (1 << OCIE0A);   //Enable Time interrupt
	
	OCR0A=62; //every 1000us the buzzer turn on or off to create the sound in about the frequency of  "BUZZ_FREQUENCY"
                           
    TCCR0B|= (0b00000100); //Set the prescalar value as 16MHZ/256, so each 16 us the clock goes 1 turn
    
}
ISR(TIMER0_COMPA_vect)
{
	buzz_count++;
	if (buzz_count%2==0)
	{
		PORTD |= (1<<3);
	}
    else 
	{
		PORTD &= ~(1<<3);
	}
	if (buzz_count==(2*BUZZ_FREQUENCY))
	{
		TCCR0B&=0b11111000;
		buzz_count=0;
	}
	
}
/*
void play_note(unsigned short freq)
{
    unsigned long period;

    period = 1000000 / freq;      // Period of note in microseconds

    while (freq--) 
	{
        PORTD |= (1 << 3);         // Make PB4 high
        variable_delay_us(period/2); // Use variable_delay_us to delay for half the period
        PORTD &= ~(1 << 3);        // Make PB4 low
		variable_delay_us(period/2); // Delay for half the period again
    }
}
*/
/*
void variable_delay_us(int delay)
{
    int i = (delay + 5) / 10;

    while (i--)
        _delay_us(10);
}
*/

void serial_init(unsigned short ubrr_value)
{

    // Set up USART0 registers
	UCSR0C = (3 << UCSZ00);               // Async., no parity,
                                          // 1 stop bit, 8 data bits
    UCSR0B |= (1 << TXEN0 | 1 << RXEN0);  // Enable RX and TX
    UBRR0 = ubrr_value;	 

    UCSR0B|= (1 << RXCIE0);
    
	// Enable tri-state
	DDRC |= (1<<3);
	PORTC &= ~(1<<3);
}

void serial_txchar(char ch)
{
    while ((UCSR0A & (1<<UDRE0)) == 0);
    UDR0 = ch;
}
void serial_stringout(char *s)
{
	serial_txchar('@');
	// Call serial_txchar in loop to send a string
	int index;
	for (index=0; index<strlen(s); index++)
	{
		if(s[index]!=' ')
		{
			serial_txchar(s[index]);
		}
	}
	serial_txchar('$');
}
ISR(USART_RX_vect)
{
	char ch;
	ch=UDR0;
	if (ch=='@') //starts to receive the data if "@" is detected
	{
		RX_startflag=1;
		RX_endflag=0;
	}
	if (!RX_startflag) //if the program havent start, the rest of the code is useless, so just skip them and return
	    return;
	
    if (ch=='$') //ends receiving the data
	{
		RX_endflag=1;
		bufff[bufff_count]='\0';
		bufff_count=0;
		return;
	}
	bufff[bufff_count]=ch;
	bufff_count++;
	
	if (bufff_count>5) // if buff_count goes too large, it means there is some problem receiving "$", so just send the data we received
	{
		RX_startflag=0;
		RX_endflag=1;
		bufff[bufff_count]='\0';
		bufff_count=0;
	}
}
		


