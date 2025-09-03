/*
 * MAX7219 Driver
 * ATtiny84
 * 8/25 Luhan Monat

 Vers   Update
 ----   ----------------------------------
 101    Decimal output to display w/dp
 102    Fix single digit / no dp ok
 103    Added Hex digits
 104    
*/

#define  VERS   104

//  Pin ATtiny84 Pin Assignments
enum  {xx,TXD,RXD,DIN,NCS,CLK};

//  MAX7219 Commands
enum  {DECODE=9,INTEN,SCANLIM,SHUTDN,DTEST};

//  Hexidecimal segments (uses reverse bit order)

byte  segs[]={0x7E,0x30,0x6D,0x79,0x33,0x5B,0x5f,0x70,0x7F,0x7B,
              0x77,0x1F,0x4E,0x3D,0x4F,0x47};

#define CLOCK   8
#define BAUD    9600
#define BITDELAY 1000000/BAUD


int main() {
byte  i,j;
word  x;
long  big;

  
  MAXI_INIT();
  OutDec(VERS,3,6);
  Wait(1000);
  MAXI_BLANK(1,8);
  Wait(1000);
    
again:
  for(i=1;i<9;i++) {
    MAXI(i,segs[16-i]);
  }
 
halt: goto halt;
}

//  Output 16 bit value w/ decimal places
//  LOC is low end of number
//  Set dp to > 5 for no dp display

void  OutDec(word data, byte loc, byte dp) {
byte  i,x,act,dd;
word  divisor;

  act=0;
  divisor=10000;
  for(i=5;i>0;i--) {
    if(i==(dp+1))
      dd=0x80;
    else
      dd=0;
    x=data/divisor;
    data=data%divisor;
    if(x||(i==(dp+1))||(i==1))
      act=1;
    if(act) 
      MAXI(i+loc,segs[x]|dd);
    divisor/=10;
  }
}

void  MAXI_INIT() {
  
  bitSet(DDRA,DIN);
  bitSet(DDRA,NCS);
  bitSet(DDRA,CLK);
  MAXI(DECODE,0);
  MAXI(INTEN,11);
  MAXI(SCANLIM,7);
  MAXI(SHUTDN,1);
  MAXI_BLANK(1,8);
}

void  MAXI_BLANK(byte first, byte last) {
byte  i;
  for(i=first;i<=last;i++) {
    MAXI(i,0);
  }
}


void  MAXI(byte op, byte data) {
word  xdata;
int i;


  xdata=(op<<8)|data;
  bitClear(PORTA,NCS);
  for(i=0;i<16;i++) {
    bitClear(PORTA,CLK);
    if(xdata&0x8000)
      bitSet(PORTA,DIN);
    else
      bitClear(PORTA,DIN);
    bitSet(PORTA,CLK);
    xdata<<=1;
  }
  bitSet(PORTA,NCS);
  bitClear(PORTA,CLK);
}




// Replacement for delay() function

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}

void  Printx(char *fmt, long dat) {
char  ary[100];
byte  i;

  sprintf(ary,fmt,dat);
  for(i=0;i<40;i++) {
    if(ary[i])
      Serout(ary[i]);
    else
      break;
  }
}

// Bit Bang input on TINY84 PA2

byte  Serin() {
byte  i,data;

  data=0;

  bitClear(DDRA,RXD);
  while(bitRead(PINA,RXD)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2);      // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY);    // middle of next bit
    if(bitRead(PINA,RXD))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


// Outputs single byte (ch) tiny84 PA1

void  Serout(byte ch) {
byte  i;

  bitSet(DDRA,TXD);            // set for output
  bitClear(PORTA,TXD);        // create start bit
  delayMicroseconds(BITDELAY);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTA,TXD);
    else
      bitClear(PORTA,TXD);
    delayMicroseconds(BITDELAY);
  }
  bitSet(PORTA,TXD);          // create stop bit
  delayMicroseconds(BITDELAY*2);
 
}
