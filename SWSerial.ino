// Luhan Monat

// Set these up for Serin(), Serout().

#define CLOCK   8
#define BAUD    9600
#define PORTX   PORTB
#define DDRX    DDRB
#define PINX    PINB
#define OUTBIT  1
#define INBIT   0
#define BITDELAY 1000000/BAUD


int main() {
int j,k,n,dat;
int  x;

  j=0;
  DDRX=0xFF;

cycle:

  Printx("\nHello...%02X",j++);
  Wait(200);

goto cycle;
}


byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRX,INBIT);
  while(bitRead(PINX,INBIT)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2-3);    // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY);    // middle of next bit
    if(bitRead(PINX,INBIT))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


// Outputs single byte to designated port/bit/baud
// [60 bytes of code]

void  Serout(byte ch) {
byte  i;

  
  bitSet(DDRX,OUTBIT);            // set for output
  bitClear(PORTX,OUTBIT);        // create start bit
  delayMicroseconds(BITDELAY);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTX,OUTBIT);
    else
      bitClear(PORTX,OUTBIT);
    delayMicroseconds(BITDELAY);
  }
  bitSet(PORTX,OUTBIT);          // create stop bit
  delayMicroseconds(BITDELAY*2);
  bitClear(DDRX,OUTBIT);
  
}


// Millisecond delay

void  Wait(word ms) {
word  i;

  for(i=0;i<ms;i++)
    delayMicroseconds(1000);

}


// printf with singe value output

void  Printx(char *fmt, int dat) {
char  ary[40],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i])
    Serout(ary[i++]);
}
