
/*  Shaker w/ motion detection
 *  4/24 Luhan Monat
 *  Util85D Board [Attiny85,8mhz]
 *  
 */

enum  {DET,Q1,Q2,MOT,X4};       // Port B bit assignments

int	main() {

  DDRB  = 0b011110;             // only B0 is input

cycle:

  if(bitRead(PINB,DET))         // PIR triggered?
    Shake(200,240);             // shake it up
  else
    goto cycle;
  Wait(9000);                   // must wait for PIR VCC to settle
  Sleeper();                    // go to sleep; wake on PIR signal     
  goto cycle;
  

}


void Shake(int cnt, byte pwr) {
int  i,j;

  for(j=0;j<cnt;j++) {
    bitSet(PORTB,MOT);
    for(i=0;i<256;i++) {
      delayMicroseconds(10);
      if(i==pwr)
        bitClear(PORTB,MOT);
    }
  }
  
}


void  Sleeper() {

  bitSet(GIMSK,5);          // enable int on change
  bitSet(PCMSK,DET);        // enable pin change interrupt (PIR)
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
