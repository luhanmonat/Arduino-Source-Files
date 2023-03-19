/*     
  Floating Power Receiving Unit
  [ATtiny85 8Mhz internal clock]
  2/29/2023

  Protocol: 5ms Attenstion, 2.5ms gap
  Each bit: 0-750usec, 1-1500usec, 750usec gap
  
*/

enum  {XBIT,RBIT,OUT,RFIN,RFOUT,RESET};

#define REDLED  XBIT


#define CLOCK   8
#define BAUD    9600
#define BITDELAY 1000000/BAUD

#define SPEED   750
#define HEATON  0x1234
#define HEATOFF 0x5678

int	  main() {
byte  heat,last,cnt;
word  code;

DDRB    = 0b100111;       // bit 3 is input

cycle:
  code=GetCode();
  
  if(code==HEATON) {
    bitSet(PORTB,OUT);
    bitSet(PORTB,REDLED);
  }
  if(code==HEATOFF) {
    bitClear(PORTB,OUT);
    bitClear(PORTB,REDLED);
  }
  goto cycle;

}

word  GetCode() {
word  leng,code;

  while(!bitRead(PINB,RFIN))
    ;
  leng=BitTime10();
  if((leng<200)||(leng>800)) // look for 5ms pulse
    return(0);
  Wait(2);                    // move up to near 1st bit
  code=GetByte();             // read high byte
  code<<=8;                   // shift up 8 bits
  code+=GetByte();            // add in low byte
  return(code);
}

byte  GetByte() {
byte  i,leng,data;

  data=0;
  for(i=0;i<8;i++) {
    leng=BitTime10();
    data<<=1;           // high order 1st
    if(leng>100)        // greater than 1ms ?
      data|=1;          // yes, is 1 bit.
  }
  return(data);
}

// measure pulses up to 12ms
// in 10us units
// wait up to 8ms for pulse to start

word  BitTime10() {
int i;

  for(i=1;i<800;i++) {           // wait 8ms for start of bit
    delayMicroseconds(10);
    if(bitRead(PINB,RFIN))
      goto ok;
  }
  return(0);                    // low state timeout

ok:
  for(i=1;i<1200;i++) {
    delayMicroseconds(10);
    if(!bitRead(PINB,RFIN))    // went low - return count
      return(i);
  }
  return(0);                    // high state timeout  
}

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);     // 
}

//  printf with singe value output to designated pin
//  (note: adds about 1500 bytes due to 'sprintf')

void  Printx(char *fmt, int dat) {
char  ary[40],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i])
    Serout(ary[i++]);
}

// Outputs single byte (ch) to designated bit (XBIT).
// may use any bit on specified port

void  Serout(byte ch) {
byte  i;

  if(ch=='\n') Wait(5);
  bitSet(DDRB,XBIT);            // set for output
  bitClear(PORTB,XBIT);        // create start bit
  delayMicroseconds(BITDELAY);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTB,XBIT);
    else
      bitClear(PORTB,XBIT);
    delayMicroseconds(BITDELAY);
  }
  bitSet(PORTB,XBIT);          // create stop bit
  delayMicroseconds(BITDELAY*2);
 
}
