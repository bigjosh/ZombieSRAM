/*
 * ZombieSRAM.c
 *
 * Created: 2/10/2014 5:56:57 PM
 *  Author: josh
 */ 

/*
 * This is a framework for very low power AVR applications that spend most time
 * sleeping and do ongoing work only when woken by a WatchDog reset.
 *
 * For best power efficiency, program the SUT fuses for 0ms startup delay
 *
 * Created: 11/18/2013 10:45:43 PM
 * Author: josh
 * Website; http://ognite.wordpress.com
 */

#include <avr/io.h>
 
#include <avr/power.h>
 
#include <avr/sleep.h>
 
#include <avr/wdt.h>


#define F_CPU 8000000

#include <util/delay.h>
 
#include <avr/wdt.h>                               // Watchdog Functions



 
// Set these according to your project,
// but make sure SUT1 and SUT0 are set or you
// waste a lot of time waiting for start up delay
 
FUSES = {
         
    .low = (FUSE_SUT1 & FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL2 & FUSE_CKSEL0),                        // Startup with clock/8, no startup delay, 8Mhz internal RC
    .high = HFUSE_DEFAULT,
    .extended = EFUSE_DEFAULT
         
};



#define TEST_BLOCK_SIZE 256

unsigned char testblock[TEST_BLOCK_SIZE];
         
void init0 (void) __attribute__ ((naked)) __attribute__ ((section (".init0")));
 
// This code will be run immedeately on reset, before any initilization or main()
 
void init0(void) {
	
	unsigned char *x;
			
	asm( "eor	r1, r1" : : ); // Clear out "zero_reg" becuase c expects it to always be zero
	
	DDRB = _BV(7) | _BV(6);		// Enable diagnostic LEDs - GREEN on pin 19, RED on pin 18
	PORTB = _BV(7) | _BV(6);		// Turn them both on for a second while we send STX as test check 	
				
	x = testblock;
	                    
	unsigned char match = 1;	// Assume a match
	
	for(unsigned int r=0; r<TEST_BLOCK_SIZE;r++) {		// Loop though the bytes in each 4 byte long pattern 
		
		unsigned char b = *(x++) ; 
		
		if (b==r) {					// Light diagnostic LED
			PORTB = _BV(7);
		} else {
			PORTB = _BV(6);
			match=0;				// remember that we missed a byte
		}
		
		_delay_ms(15);
		
		PORTB = 0x00;
	}
	
	_delay_ms(250);
	
	x = testblock;
	
	for(unsigned int r=0; r<TEST_BLOCK_SIZE;r++) {		// Loop though the bytes in each 4 byte long pattern
		
		*(x++) = r;
		
	}
	
	
	if (match) {					// Light diagnostic LED for final verdict
		PORTB = _BV(7);
		} else {
		PORTB = _BV(6);
	}
		
	_delay_ms(250);
	
	// Reload the test pattern for next pass...

	
	PORTB = 0x00;
	DDRB = 0x00;
	
	MCUCR = _BV( SE ) | _BV(SM1 ) | _BV(SM0); // Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
	
	asm("sleep");
			
}

// Main() only gets run once, when we first power up
 
int main(void)
{
}
