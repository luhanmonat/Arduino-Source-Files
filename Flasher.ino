
/*  Flasher w/ motion detection
 *  4/24 Luhan Monat
 *  Util85D [Attiny85,8mhz]
 *  
 */

enum  {DET,Q1,Q2,LED,X4};

int	main() {

  DDRB  = 0b011110;             // input on B0 only

cycle:

  if(bitRead(PINB,DET))         // only use rising edge
    Flash(10);
  else
    goto cycle;
  Wait(9000);
  Sleeper();
  goto cycle;
  

}


void Flash(byte msecs) {

  bitSet(PORTB,LED);        // Turn on LED
  Wait(msecs);
  bitClear(PORTB,LED);      // turn off led
  Wait(200);
}


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
