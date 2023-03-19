/*
 * Bluetooth remote programming device w/ isolated TX/RX
 * 2/23 Luhan Monat
 * Program any Arduino via HC-06 bluetooth
 * Create reset pulse via 1 second timeout
 */


enum  {x1,x2,HCTX,RST,INH};


int	main() {
byte  last,dat,trips;
long  cnt;

  DDRB  = 0b00111011;
  bitSet(PORTB,HCTX);       // Data from HC-12
  bitSet(PORTB,RST);
  bitSet(PORTB,INH);

cycle:
  trips=3;
  // Wait for 1 second of no data changes
  for(cnt=0;cnt<40000;cnt++) {
    delayMicroseconds(25);
    last=dat;
    dat=bitRead(PINB,HCTX);
    if(last!=dat)
      if(!trips--) {
        cnt=0;
        trips=3;
      }
  }
  
  bitSet(PORTB,INH);          // disconnect
  // Now wait for any data pulse low
  trips=3;
  while(1) {
    delayMicroseconds(10);
    last=dat;
    dat=bitRead(PINB,HCTX);
    if(last!=dat)
      if(!trips--)
        goto pulse;
  }

pulse:
  bitClear(PORTB,RST);      // Start RESET
  bitClear(PORTB,INH);      // Connect TX/RX lines
  Wait(1);
  bitSet(PORTB,RST);        // End RESET
  
  goto cycle; 

}

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}
