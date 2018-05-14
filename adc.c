#include <avr/io.h>

#include "adc.h"

void adc_init(unsigned char channel)
{
    // Initialize the ADC
	unsigned char mask=0b00001111;
	ADMUX=0b01100000;
	ADMUX |=(channel&mask);

	ADCSRA|=0b00000111; //select 128 as the clock divisor
	ADCSRA|=(1<<ADEN); //turn ADC on;

}

unsigned char adc_sample()
{
    // Convert an analog input and return the 8-bit result
	ADCSRA|=(1<<ADSC); //start ADC
	while ((ADCSRA&(1<<ADSC))!=0) {} 
	unsigned char result=ADCH;
	return result;
}
