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



// Inline because we have no stack because that would overwrite some memory

inline void initUSART() 
{
	return;
	  //set the baud rate
	
	  UBRRH = 0;
	  UBRRL = 51;			/// 9600bd at 8mhz from datasheet page 144
	  
	  // enable transmitter
	  
	  UCSRB = (1<<TXEN);

	  //set 8N1	
	  
	  UCSRC = (3<<UCSZ0);
}

inline void shutdownUSART() {
	  // disable transmitter
	  return;
	  UCSRB = 0;
	
}

// Sends a serial byte and waits for it to be fully transmitted
// assumes any previous transmission already complete

inline void sendbyte( unsigned char b ) {
	return;
	UDR = b;		// Start sending....
	
	while (!(UCSRA & _BV(UDRE)));		// Wait for it to be fully transmitted
			
}


// Sends a serial byte and waits for it to be fully transmitted
// assumes any previous transmission already complete

inline void sendbytefully( unsigned char b ) {
	return;
	UCSRA ^= _BV(TXC);				// CLear the Transmit complete flag by writing a 1 to it (datasheet 14.10.2)
	
	sendbyte(b);

	while (!(UCSRA & _BV(TXC)));		// Wait for it to be fully transmitted
	
}


#define TEST_BLOCK_SIZE 256

unsigned char testblock[TEST_BLOCK_SIZE];
         
void init0 (void) __attribute__ ((naked)) __attribute__ ((section (".init0")));
 
// This code will be run immedeately on reset, before any initilization or main()
 
void init0(void) {
	
	unsigned char *x;
	
	unsigned char checksum = 0;
		
	asm( "eor	r1, r1" : : ); // Clear out "zero_reg" becuase c expects it to always be zero
	
	DDRB = _BV(7) | _BV(6);		// Enable diagnostic LEDs - GREEN on pin 19, RED on pin 18
	PORTB = _BV(7) | _BV(6);		// Turn them both on for a second while we send STX as test check 	
		
	initUSART();
	
	_delay_ms(100);		// Let serial port settle down...
		
	sendbytefully(0xEE);		// STX
	
	sendbytefully(0xEE);		// STX	
	
	PORTB = 0x00;
			
	// send current contents of the test block out the serial port...
		
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
		
		sendbytefully( b );
		
		checksum ^= b; 
		
		_delay_ms(5);			// Seems like Ardunio softserial receiver needs this to breath...
		
		PORTB = 0x00;
		
		_delay_ms(5);			// Seems like Ardunio softserial receiver needs this to breath...
		
		
	}
	
	
	sendbytefully(checksum);
			
	// Reload the test pattern for next pass...

	x = testblock;
			
	for(unsigned int r=0; r<TEST_BLOCK_SIZE;r++) {		// Loop though the bytes in each 4 byte long pattern
		
		*(x++) = r;
		
	}
	
	// Don't send ETX until we have written data for next pass so we won't get turned off too soon...

	sendbytefully(0x55);		// ETX
		
	sendbytefully(0x55);		// ETX
	
	// Controller should wait a couple of milliseconds after getting final ETX to make sure we have time to shutdown the USART
	// because it seems like if we leave it on then the SRAM expires much more quickly. 
			
	shutdownUSART();
	
	_delay_ms(500);
	
	if (match) {					// Light diagnostic LED for final verdict
		PORTB = _BV(7);
		} else {
		PORTB = _BV(6);
	}
		
	//Done, so halt and wait to be shutdown
	
	 MCUCR = _BV( SE ) | _BV(SM1 ) | _BV(SM0); // Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
 
	 asm("sleep");
		
}

// Main() only gets run once, when we first power up
 
int main(void)
{
}
