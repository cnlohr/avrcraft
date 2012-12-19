//Copyright 2011 <>< Charles Lohr, under the MIT/x11 License.
//Portions are Copyright Scott Harris
//http://scottrharris.blogspot.com/2010/12/controlling-ge-color-effects-lights.html

#include "g35.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>


void HandleLights();
unsigned char li;
unsigned char lj;
static void LightStep();
static void xmas_set_color0(uint8_t led,uint8_t intensity,uint16_t color);
static void xmas_set_color1(uint8_t led,uint8_t intensity,uint16_t color);
static void xmas_set_color3(uint8_t led,uint8_t intensity,uint16_t color);


void SetupClock()
{
	CLKPR = _BV( CLKPCE );
	CLKPR = 0; //No divide
}

int main() {
	short tf = 0;
	uint8_t frame;

	cli();
	DDRD |= _BV(0);
	DDRD |= _BV(1);
	DDRD |= _BV(3);
	setup_spi();
	SetupClock();
	sei();

	while(1) {

		for( tf = 0; tf < 50; tf++ )
		{
			xmas_set_color0(tf,(frame&1)?0xcc:0x00,0xa0a);
		}
		for( tf = 0; tf < 50; tf++ )
		{
			xmas_set_color1(tf,(frame&1)?0xcc:0x00,0x0aa);
		}

		for( tf = 0; tf < 50; tf++ )
		{
			xmas_set_color3(tf,(frame&1)?0xcc:0x00,0xaa0);
		}

			frame++;
	//	_delay_ms(1000);
		sendchr( 0 );
		sendchr( '!' );
	}
}










 #define xmas_color_t uint16_t // typedefs can cause trouble in the Arduino environment  
   
#define b1(p) {PORTD|=_BV(p);}
#define b0(p) {PORTD&=~_BV(p);}

#define xbegin(p) { b1(p); _delay_us(10); b0(p); }
#define x1(p) { b0(p); _delay_us(20-4); b1(p); _delay_us(10-2); b0(p); }  //#define x1 { b0; _delay_us(11); b1; _delay_us(7); b0; }  
#define x0(p) { b0(p); _delay_us(10-4); b1(p); _delay_us(20-2); b0(p); }  //#define x0 { b0; _delay_us(2); b1; _delay_us(17); b0; }
#define xend(p) {b0(p); _delay_us(40); }


/*
#define xbegin { b1; _delay_us(10); b0; }
#define x1 { b0; _delay_us(20-4); b1; _delay_us(10-2); b0; }  //#define x1 { b0; _delay_us(11); b1; _delay_us(7); b0; }  
#define x0 { b0; _delay_us(10-4); b1; _delay_us(20-2); b0; }  //#define x0 { b0; _delay_us(2); b1; _delay_us(17); b0; }
#define xend {b0; _delay_us(40); }
*/
/*
//We've pushed the [quick] extreme.
#define xbegin { b1; _delay_us(7); b0; }
#define x1 { b0; _delay_us(10); b1; _delay_us(3); b0; }  //#define x1 { b0; _delay_us(11); b1; _delay_us(7); b0; }  
#define x0 { b0; _delay_us(2); b1; _delay_us(11); b0; }  //#define x0 { b0; _delay_us(2); b1; _delay_us(17); b0; }
#define xend {b0; _delay_us(40); }
*/


tic void xmas_set_color0(uint8_t led,uint8_t intensity,xmas_color_t color) {  
      uint8_t i;  
	cli();
      xbegin(0); 
 
      for(i=6;i;i--,(led<<=1))
      {
           if(led&(1<<5))  
                x1(0)
           else  
                x0(0)

      }
	_delay_us(6);
      for(i=8;i;i--,(intensity<<=1))
      {
           if(intensity&(1<<7))  
                x1(0)
           else  
                x0(0)
      }

	unsigned char top = color>>8;
	unsigned char bottom = color;

for(i=4;i;i--,(top<<=1))  
      {
           if(top&(1<<3))  
                x1(0)
           else  
                x0(0)
       }
for(i=8;i;i--,(bottom<<=1))  
      {
           if(bottom&(1<<7))  
                x1(0)
           else  
                x0(0)
       }

sei();
      xend(0);
 }  
   

static void xmas_set_color1(uint8_t led,uint8_t intensity,xmas_color_t color) {  
      uint8_t i;  
	cli();
      xbegin(1); 
 
      for(i=6;i;i--,(led<<=1))
      {
           if(led&(1<<5))  
                x1(1)
           else  
                x0(1)

      }
	_delay_us(6);
      for(i=8;i;i--,(intensity<<=1))
      {
           if(intensity&(1<<7))  
                x1(1)
           else  
                x0(1)
      }

	unsigned char top = color>>8;
	unsigned char bottom = color;

for(i=4;i;i--,(top<<=1))  
      {
           if(top&(1<<3))  
                x1(1)
           else  
                x0(1)
       }
for(i=8;i;i--,(bottom<<=1))  
      {
           if(bottom&(1<<7))  
                x1(1)
           else  
                x0(1)
       }

sei();
      xend(1);
 }  
   

static void xmas_set_color3(uint8_t led,uint8_t intensity,xmas_color_t color) {  
      uint8_t i;  
	cli();
      xbegin(3); 
 
      for(i=6;i;i--,(led<<=1))
      {
           if(led&(1<<5))  
                x1(3)
           else  
                x0(3)

      }
	_delay_us(6);
      for(i=8;i;i--,(intensity<<=1))
      {
           if(intensity&(1<<7))  
                x1(3)
           else  
                x0(3)
      }

	unsigned char top = color>>8;
	unsigned char bottom = color;

for(i=4;i;i--,(top<<=1))  
      {
           if(top&(1<<3))  
                x1(3)
           else  
                x0(3)
       }
for(i=8;i;i--,(bottom<<=1))  
      {
           if(bottom&(1<<7))  
                x1(3)
           else  
                x0(3)
       }

sei();
      xend(3);
 }  
   
