#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "encoder.h"
#include "rangefinder.h"

extern volatile int min_value;
extern volatile unsigned char rotary_newstate, rotary_oldstate;


ISR(PCINT0_vect)//ROTARY INTERRUPT
{   
    char input1= PINB; //get the input value
	char a,b;
	if ((input1&(1<<4))==0)
    {
	    b=0;
	}
	else 
	{
		b=1;
	}
	
	if ((input1&(1<<3))==0)
	{
		a=0;
	}
	else
	{
		a=1;
	}

	if (rotary_oldstate == 0) 
	{
        // Handle A and B inputs for state 0
		if ((a==1)&&(b==0))
		{
			rotary_newstate=1;
			min_value++;
		}
		else if ((a==0)&&(b==1))
		{
			rotary_newstate=2;
			min_value--;
		}

	}
	else if (rotary_oldstate == 1) 
	{
		
        // Handle A and B inputs for state 1
		if((b==1)&&(a==1))
		{
			rotary_newstate=3;
			min_value++;
		}
		else if((a==0)&&(b==0))
		{
			rotary_newstate=0;
			min_value--;
		}
    }
	else if (rotary_oldstate == 2) 
	{
		
        // Handle A and B inputs for state 3
		if((a==0)&&(b==0))
		{
			rotary_newstate=0;
			min_value++;
		}
		else if((b==1)&&(a==1))
		{
			rotary_newstate=3;
			min_value--;
		}
    }
	else 
	{   // old_state = 3
        // Handle A and B inputs for state 3
        if ((a==0)&&(b==1))
		{
			rotary_newstate=2;
			min_value++;
		}
		else if ((a==1)&&(b==0))
		{
			rotary_newstate=1;
			min_value--;
		}
	}
	if (min_value<0)
	{
		min_value=400;
	}
	
    if (min_value>400)
	{
		min_value=0;
	}
	
	eeprom_update_word((void*)100,min_value); //store the data into eeprom
	rotary_oldstate=rotary_newstate;          //renew the state before the loop ends
}
