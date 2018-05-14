/********************************************
 *
 *  Name: Chunyang Mou
 *  Section: FRIDAY 12:30
 *  Assignment: Project - Rangefinder
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "adc.h"
#include "encoder.h"
#include "serial.h"
#include "rangefinder.h"

#define FOSC 16000000           // Clock frequency
#define BAUD 9600               // Baud rate used
#define MYUBRR FOSC/16/BAUD-1   // Value for UBRR0 register
#define ADC_CHAN 0              // Buttons use ADC channel 0


char *modes[]={"single","repeat"};
volatile unsigned int time_us=0;
volatile int distance=0;
volatile int pulse_count=0;
unsigned char pulse_flag=0;
unsigned char mode_state=0;

volatile int min_value=0;
volatile unsigned char rotary_newstate, rotary_oldstate;


volatile int RX_startflag=0;
volatile int RX_endflag=0;
volatile char bufff[10];
volatile int bufff_count=0;

int num=0; //the distance we receive from another arduino

volatile char acquire=0;

char messagebuf[10];
char senfbuf[5];
char minvaluebuff[5];
char receivebuf[10];

int main()
{
	lcd_init();              //Initialize lcd
	lcd_splashscreen();      //splash the screen to show the program is running
	adc_init(ADC_CHAN);      //Initialize adc for the encoder
    serial_init(MYUBRR);     //Initialize serial interface
	UCSR0B |= (1 << RXCIE0); // Enable receiver interrupts
	
	DDRC &= ~(1 << 1);       //set the ports to inputs for buttons
	DDRC &= ~(1 << 2);

    PORTC |= ((1<<1)|(1<<2)); //set up the pull-up resistors for ports A2 AND A1
	DDRD |= (1<<2);           //Connect D2 to trig of the censor so set it to output
	PORTC |= (1<<5);          //Set A5 to receive the signal from censor
	
	PORTB |= ((1<<PB4)|(1<<PB3)); //Enable the pull-up resistors for the rotary encoder
	DDRD |= (1<<3);            //Enable d3 to output to generate sound using the buffer

	
	PCICR |= ((1 << 1)|(1<<0)); //Enable THE interrupt for any change in ports
    PCMSK1 |= (1 << PCINT13);   //Enable A5 to receive the interrupt 
	PCMSK0 |= ((1 << PCINT3)|(1 <<PCINT4)); //Enbale B3 B4 to receive interrupts\
	
	DDRB|=(1<<5);               //enable the ports to output for red and green LEDs
	DDRC|=(1<<4);          
	
	sei();                      //Enable the global interrupt
	
	char input= PINB;
	char rotary_a,rotary_b;
	if ((input&(1<<4))==0)
	{
	    rotary_b=0;
	}
	else 
	{
		rotary_b=1;
	}
	
	if ((input&(1<<3))==0)
	{
		rotary_a=0;
	}
	else
	{
		rotary_a=1;
	}
    //define the states and find the initial state
    if (!rotary_b && !rotary_a)    //b=0, a=0
	rotary_oldstate = 0;
    else if (!rotary_b && rotary_a)//b=0, a=1
	rotary_oldstate = 1;
    else if (rotary_b && !rotary_a)//b=1, a=0
	rotary_oldstate = 2;
    else             //b=1, a=1
	rotary_oldstate = 3;
    
    rotary_newstate = rotary_oldstate;
	
	char temp=eeprom_read_word((void*)100);
	if ((temp>0)&&(temp<400))
	{
		min_value=temp;
	}
	

	while(1)
    {
		lcd_moveto(1,0);           
		snprintf(minvaluebuff,5,"%3d", min_value);
		lcd_stringout(minvaluebuff);     //print the min value
		
		if (mode_state==0)               //single mode
		{
			if ((PINC & (1<<1)) == 0)    //check if the "mode" button is pressed
		    {
				while ((PINC & (1<<1)) == 0){}
		        mode_state = 1; //set state to repeat
			    lcd_moveto(0,0);
				lcd_stringout("repeat");
				_delay_ms(100);
		    }
			else if ((PINC & (1<<2)) == 0) //check if the acquire button is pressed
			{
				acquire=1;
				_delay_ms(100);
			}
		}
		else if (mode_state==1)            //repeat mode
		{
			if ((PINC & (1<<1)) == 0)      //check if the "mode" button is pressed
		    {
				while ((PINC & (1<<1)) == 0){}
		        mode_state = 0;            //set state to single
			    lcd_moveto(0,0);
				lcd_stringout("single");
				_delay_ms(200);
		    }
		}
		if (mode_state==0)                  //single mode
		{
		    if (acquire==1)                 //acquire button is pressed
			{
			    PORTD |= (1<<2);            //censor sends the pulse to measure the distance
				_delay_us(20);
				PORTD &= ~(1<<2);
				acquire=0;                  //reset the acquire flag
				
				while(pulse_flag%2==1){}    //wait to receive the pulse
				if (distance>40)            //only prints and sends  the data when it is larger than the minimum distance and smaller than the max distance
				{
					if (distance<4000)
					{
						lcd_moveto(0,8);    //print the distance in cm
					    snprintf(messagebuf,10,"%3d.%d",distance/10,distance%10);
	                    lcd_stringout(messagebuf);
						
						snprintf(senfbuf,5,"%d", (int) distance); //send the distance in mm
					    serial_stringout(senfbuf);
						
					}
					else
					{
						lcd_moveto(0,8);    //print too far if the distance is illegal, do not send the data
						lcd_stringout("Too far");
					}
				}
			}
	    }
		else if (mode_state==1) //repeat mode
		{
			PORTD |= (1<<2);                  //censor sends the flag to measure the distance constantly
		   _delay_us(20);
			PORTD &= ~(1<<2);
			while(pulse_flag==1){}
			
			if (distance>40)                  //only prints and sends the data when it is larger than the minimum distance and smaller than the maximum distance
			{
				if (distance<4000)
				{
					lcd_moveto(0,8);          //print the distance in cm
				    snprintf(messagebuf,10,"%3d.%d",distance/10,distance%10);
	                lcd_stringout(messagebuf);
					
					snprintf(senfbuf,5,"%d", (int) distance);  //send the distance in mm
				    serial_stringout(senfbuf);
					
					pulse_flag=0;
						
				}
				else
				{
					lcd_moveto(0,8);            //print too far if distance is illegal, do not send the data
					lcd_stringout("Too far");
				}
				_delay_ms(200);                //Delay here to prevent the program refreshes too fast
			}
			
			
		}
	
		if (RX_endflag==1)                     //if there is a data coming in
		{
			sscanf(bufff,"@%d",&num);
			if(num>4000)
			{
				lcd_moveto(1,8);
				lcd_stringout(">400");
			}
			else
			{
				lcd_moveto(1,8); 
				snprintf(receivebuf,6,"%3d.%d",num/10,num%10); //print the data if it is legal
				lcd_stringout(receivebuf);
			
			}
			if (num/10<min_value)              //let the buzzer generates sound if the data received is smaller than minimum value
			{
				play_note();                   //play_note function is in serial.c
			}
			RX_endflag=0;                      //reset both flags for future data coming in
			RX_startflag=0;
		}
		if (num>distance)                      //LEDs for comparing the data measured and the data from the other arduino
		{
			PORTC |= (1<<4);
			PORTB &= ~(1<<5);
		}
		if (distance>num)
		{
			PORTB |= (1<<5);
			PORTC &= ~(1<<4);
		}
			
	}
}
	

ISR(TIMER1_COMPA_vect)                       //watch dog function to prevent the data of the censor goes too large
{
	TCCR1B &= 0b11111000;
	TCNT1=0;
	pulse_flag=1;
	lcd_moveto(0,8);
	acquire=0;
}

ISR(PCINT1_vect)
{
    pulse_flag++;                            //odd pulse flag means start counting the period; even means the timer finishes measuring the period already
	if (pulse_flag%2==1)
	{
		TCNT1=0;
		// Set the mode for "Clear Timer on Compare" (CTC)
	    TCCR1B |= (1 << WGM12);
        //Enable Time interrupt
	    TIMSK1 |= (1 << OCIE1A);
       //OCR1A=60000; //Prevent the clock to go too far
    
	    //Set the prescalar value as 16MHZ/8
	    TCCR1B |= (0b00000010);
	}
	else
	{
		TCCR1B&=0b11111000;
	    pulse_count=TCNT1;                   
		distance=pulse_count*5/58;//distance in mm		
	}
}




		
	
		
		
		
		
	






		
	
	
	




