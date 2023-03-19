
/*
 * Sleep Mode for ATtiny85
 * 2/22 Luhan Monat
 */


int	main() {
int i;

DDRB  = 0b011001;     // PB2 is input
bitSet(PORTB,2);      // PB2 enable pullup

again:

  for(i=0;i<10;i++) {
    bitSet(PORTB,0);
    Wait(200);
    bitClear(PORTB,0);
    Wait(200);
  }
  Sleeper();
  bitClear(PORTB,0);
  Wait(1000);
  goto again;



}


void  Sleeper() {


  bitSet(GIMSK,5);
  bitSet(PCMSK,2);          // enable pin change interrupt
  MCUCR = 0b00110000;
  bitSet(SREG,7);
  PRR   = 0b00001111;       // shut down peripherals
  asm("sleep \n");
  PRR   = 0b00000000;       // enable all peripherals

}

ISR (PCINT0_vect) {

  bitSet(PORTB,0);
  Wait(1000);
}


void  Wait(word ms) {
word  i;

  for(i=0;i<ms;i++)
    delayMicroseconds(1000);
}
