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
	  
	  UCSRB = 0;
	
}

// Sends a serial byte and waits for it to be fully transmitted
// assumes any previous transmission already complete

inline void sendbyte( unsigned char b ) {
		
	UDR = b;		// Start sending....
	
	while (!(UCSRA & _BV(UDRE)));		// Wait for it to be fully transmitted
			
}


// Sends a serial byte and waits for it to be fully transmitted
// assumes any previous transmission already complete

inline void sendbytefully( unsigned char b ) {
	
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
	PINB = _BV(7) | _BV(6);		// Turn them both on for a second while we send STX as test check 	
	
	initUSART();
	
	sendbytefully(0xEE);		// STX
	
	sendbytefully(0xEE);		// STX	
	
	PINB = 0x00;
			
	// send current contents of the test block out the serial port...
		
	x = testblock;
	
	unsigned char match = 1;	// Assume a match
	
	for(unsigned int r=0; r<TEST_BLOCK_SIZE;r++) {		// Loop though the bytes in each 4 byte long pattern 
		
		unsigned char b = *(x++) ; 
		
		if (b==r) {					// Light diagnostic LED
			PINB = _BV(7);
		} else {
			PINB = _BV(6);
			match=0;				// remember that we missed a byte
		}
		
		sendbytefully( b );

		PINB = 0;		// Turn off LED (it was on long enough durring the Serial send
		
		checksum ^= b; 
		
		_delay_ms(10);			// Seems like Ardunio softserial receiver needs this to breath...
		
		
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
		PINB = _BV(7);
		} else {
		PINB = _BV(6);
	}
		
	//Done, so halt and wait to be shutdown
	
	 MCUCR = _BV( SE ) | _BV(SM1 ) | _BV(SM0); // Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
 
	 asm("sleep");
		
}


// Put any code you want to run once on power-up here....
// Any global variables set in this routine will persist to the WatchDog routine
// Note that I/O registers are initialized on every WatchDog reset the need to be updated inside userWakeRountine()
 
// This is "static inline" so The code will just be inserted directly into the main() code avoiding overhead of a call/ret
 
static inline void userStartup(void) {
 
 // Your start-up code here....
 
}
 
// Main() only gets run once, when we first power up
 
int main(void)
{
	
DDRD = 0x03;
	
while(1) {

	PORTD = 0x00;
	
	_delay_ms(200);
	
	PORTD = 0x01;
	
	_delay_ms(200);
	
	PORTD = 0x02;
	
	_delay_ms(200);
	
}


	
 userStartup();   // Do this first, because if we turned on WDT first it might reset on us
 wdt_enable(WDTO_15MS); // Could do this slightly more efficiently in ASM, but we only do it one time- when we first power up
 // The delay set here only matters for the first time we reset to get things started, so we set it to the shortest available so we don't wait to wait too long...
 // In warmstart we will set it recurring timeout.
 
 // Note that we could save a tiny ammount of time if we RJMPed right into the warmstart() here, but that
 // could introduce a little jitter since the timing would be different on the first pass. Better to
 // always enter warmstart from exactly the same reset state for consistent timing.
 
 MCUCR = _BV( SE ) | _BV(SM1 ) | _BV(SM0); // Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
 
 asm("sleep");
 
 // we should never get here
 
}
 
// Put any code you want to run on every wake up here....
// Any global variables used in this routine will persist
// All I/O Registers are reset to their initial values (per the datasheet) everytime we wake up
// If you plan to do work for longer than the WatchDog timer, then you need to do a WatchDogReset (WDR) occasionally to keep the timer from expiring on you. Note that this can add jitter if consistant timing is important.
 
// This is "static inline" so The code will just be inserted directly into the warmstart code avoiding overhead of a call/ret
 
static inline void userWakeRoutine(void) {
 
 // Just for testing, lets twiddle PORTD0 bit on and off with a little delay between...
 
 DDRD |= _BV(0);   // Note that we can't do this in startup because IO registers get cleared everytime we reset
 PORTD |= _BV(0);
 
 for( unsigned x=0; x<100;x++ ) {asm("nop");}
 
 PORTD &= ~_BV(0);
 
}
 
void __attribute__ ((naked)) warmstart(void) {
 
 // Set the timeout to the desired value. Do this first because by default right now it will be at the initial value of 16ms
 // which might not be long enough for us to do what we need to do.
 
 
 
WDTCR = _BV( WDP1) | _BV( WDP0) ; // This sets a 125ms timeout. See the table of timeout values in the datasheet. Note that you can delete this line completely if you want the default 16ms timeout.
 
// Now do whatever the user wants...
 
 userWakeRoutine();
 
 // Now it is time to get ready for bed. We should have gotten here because there was just a WDT reset, so...
 // By default after any reset, the watchdog timeout will be 16ms since the WDP bits in WDTCSR are set to zero on reset. We'd need to set the WDTCSR if we want a different timeout
 // After a WDT reset, the WatchDog should also still be on by default because the WDRF will be set after a WDT reset, and "WDE is overridden by WDRF in MCUSR. See “MCUSR – MCU Status Register” on page 45for description of WDRF. This means thatWDE is always set when WDRF is set."
 
 MCUCR = _BV( SE ) | _BV(SM1 ) | _BV(SM0); // Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
 
 asm("sleep");
 
 // we should never get here
 
 }