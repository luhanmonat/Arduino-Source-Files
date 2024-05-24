
/*  Flasher w/ motion detection
 *  4/24 Luhan Monat
 *  Util85D [Attiny85,8mhz]
 *  
 *  Uses SB312 PIR sensor module
 *  Sleep until next detection (15 microamps)
 *  
 */

// define all the bits the easy way

enum  {DET,Q1,Q2,LED,X4};

// Use MAIN like a real C program

int	main() {

  DDRB  = 0b011110;             // input on B0 only

cycle:

  if(bitRead(PINB,DET)) {      // only use rising edge
    Flash(10);                 // for the flash
    Wait(9000);                // let voltage settle on PIR
  }
  Sleeper();                `  // back to sleep
  goto cycle;
  

}

// Flash the LED

void Flash(byte msecs) {

  bitSet(PORTB,LED);        // Turn on LED
  Wait(msecs);
  bitClear(PORTB,LED);      // turn off led
  Wait(200);
}

// ATtiny85 sleep function
// Wake on pin change connected to PIR senso

void  Sleeper() {

  bitSet(GIMSK,5);          // enable int on change
  bitSet(PCMSK,DET);        // enable pin change interrupt
  MCUCR = 0b00110000;       // sleep enable, power down
  bitSet(SREG,7);           // global int enable
  PRR   = 0b00001111;       // shut down peripherals
  asm("sleep \n");
  PRR   = 0b00000000;       // enable all peripherals

}

ISR (PCINT0_vect) {

}
  
// Replacement for delay() function

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}
