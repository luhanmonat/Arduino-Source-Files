/*
 * UniCom to 433 Mhz RF
 * 11/22 Luhan Monat [ATtiny84]
 * 
 * Protocol: 5ms Attenstion, 2.5ms gap
 * Each bit: 0-750usec, 1-1500usec, 750usec gap
 */


//  Port A

enum  abits {Y1,Y2,Y3,LED,RPWR,RDAT,XDAT,XPWR};


// Port B


#define BTX       1       // (reverse x/r for tiny85)
#define BTR       0
#define BTPWR     2

#define CLOCK   8
#define BAUD    9600
#define BITDELAY 1000000/BAUD

#define BUG     5
#define BUFSIZE 40

byte  Buffer[BUFSIZE],bufpnt;
word  xcode,rcode;
byte  okcodes,cmd,val;
byte  END;
word  clocker;


int main() {
byte  index,x;
int val,adata;

  DDRA  = 0b11011111;
  DDRB  = 0b0110;
  bitSet(PORTB,BTPWR);      // power up bluetooth
  bitSet(PORTA,RPWR);       // Power up receive
  bitSet(PORTA,RDAT);       // Receive data pull-up
  Flash(30);
 
cycle:

  x=Serstat();
  if(x) 
    goto ser;
  if(bitRead(PINA,RDAT))
    goto recv;
  goto cycle;
    
ser:    
  x=Serin();
  if(x!='/') 
    goto cycle;

  x=Serin();
  index=x-'A';
  val=GetDec();
  
  switch(x) {
    replot:
    case '*': Serout(0); Serout('\r'); Wait(100);  // clear the line
              Printx("T,RF433 X/R\r",0); Wait(200);
              Printx("B,1,XMIT,CODE,,,,,,,,,,\r",0);
              break;
    case 'A': if(!val) {
                XmitCode(xcode);
                Flash(2);
              } break;
    case 'B': xcode=HexPad();
              Printx("D1,Xmit:%04X\r",xcode);
              goto replot;
  } 
  
  goto cycle;

recv:
  rcode=GetCode();
  if(rcode) {
    ++okcodes;
  } else {
    okcodes=0;
  }
  Printx("D3,Recv:%04X\r",rcode);
  Wait(50);
  Printx("D4,Count=%d\r",okcodes);
  Flash(1);
  goto cycle;
  
}

void  Flash(byte cnt) {
byte  i;

  for(i=0;i<cnt;i++) {
    bitSet(PORTA,LED);
    Wait(20);
    bitClear(PORTA,LED);
    Wait(50);
  }
}
//  Pop Up numeric pad - return 16 bit unsigned value
//  hold .5 seconds for second character

word  HexPad() {
long  data,last;
  
  Printx("B,1,1/A,2/B,3/C,4/D,5/E,6/F,7,8,9,DEL,0,DONE\r",0);
  data=0;
  GetIRcom();
  
next:

  Printx("D2,%X\r",data);
  GetIRcom();
  if(val==0) goto next;
  last=data;
  switch(cmd) {
    case 'J': data/=16; break;
    case 'L': return(data);
    case 'K': data*=16; break;
    default : data*=16;
              data+=cmd-'A'+1;
              if((val>3)&&(cmd<'G'))
                data+=9;
  }
  if(data>0xffff)
    data=last;
  goto next;
}

word  NumPad() {
long  data,last;
  
  Printx("B,1,1,2,3,4,5,6,7,8,9,DEL,0,DONE\r",0);
  data=0;
  
next:

  Printx("D2,%u\r",data);
  GetIRcom();
  last=data;
  if(val>0) goto next;
  switch(cmd) {
    case 'J': data/=10; break;
    case 'L': return(data);
    case 'K': data*=10; break;
    default : data*=10;
              data+=cmd-'A'+1;
  }
  if(data>0xffff)
    data=last;
  goto next;
}


//  Get valid command 
//  Returns cmd and val global variables

void  GetIRcom() {
byte  ch;

retry:
  ch=Serin();
  if(ch!='/')
    goto retry;
  cmd=Serin();
  val=GetDec();
  
}

void  XmitCode(word code) {
  
  bitSet(PORTA,XPWR);
  Wait(1);
  RFAttn();
  if(code>0xff)
    RFXmit(code>>8);
  RFXmit(code&0xff);
  Wait(1);
  bitClear(PORTA,XPWR);
}
  
//  You can replace these 2 routines with whatever coding
//  scheme you want to use.

void  RFAttn() {
  bitSet(PORTA,XDAT);
  delayMicroseconds(5000);
  bitClear(PORTA,XDAT);
  delayMicroseconds(2500);
}

word  GetCode() {
word  leng,code;
int   val;

  leng=BitTime10();
  if((leng<200)||(leng>800)) // look for 5ms pulse
    return(0);
  code=GetByte();             // read high byte
  val=GetByte();
    if(val!=-1)
      code=(code<<8)+val;
  return(code);
}

int   GetByte() {
byte  i,leng,data;

  data=0;
  for(i=0;i<8;i++) {
    leng=BitTime10();
    if(leng==0)
      return(-1);
    data<<=1;           // high order 1st
    if(leng>110) {       // greater than 1ms ?
      data|=1;          // yes, is 1 bit.
    }
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
    if(bitRead(PINA,RDAT))
      goto ok;
  }
  return(0);                    // low state timeout

ok:
  delayMicroseconds(200);
  for(i=1;i<1200;i++) {
    delayMicroseconds(10);
    if(!bitRead(PINA,RDAT)) {   // went low - return count
       return(i);
    }
  }
  return(0);                    // high state timeout  
}

void  RFXmit(byte dat) {
byte  i,mask;
  
  mask=0x80;
  for(i=0;i<8;i++) {
    bitSet(PORTA,XDAT);
    if(dat&mask) delayMicroseconds(1500);
    else         delayMicroseconds(750);
    bitClear(PORTA,XDAT);
    delayMicroseconds(750); 
    mask>>=1;
  }
}


int GetDec() {
int   val;
char  c;

  val=0;
nxt:
  c=Serin();
  if(isdigit(c)) {
    val*=10;
    val+=c-'0';
  } else {
    return(val);
  }
  goto nxt;
  
}


// Replacement for delay() function

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}





//  printf with singe value output to designated pin
//  (note: adds about 1500 bytes due to 'sprintf')

void  Printx(char *fmt, int dat) {
char  ary[100],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i])
    Serout(ary[i++]);
}

//  Return TRUE on serial data detected

byte  Serstat() {
  bitClear(DDRB,BTR);
  if(bitRead(PINB,BTR))
    return(0);
  return(1);
}

// Bit Bang input on any port bit (RBIT)
// may use any bit on specified port

byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRB,BTR);
  while(bitRead(PINB,BTR)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2-3);      // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY-2);    // middle of next bit
    if(bitRead(PINB,BTR))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


// Outputs single byte (ch) to designated bit (XBIT).
// may use any bit on specified port

void  Serout(byte ch) {
byte  i;

  bitSet(DDRB,BTX);            // set for output
  bitClear(PORTB,BTX);        // create start bit
  delayMicroseconds(BITDELAY);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTB,BTX);
    else
      bitClear(PORTB,BTX);
    delayMicroseconds(BITDELAY);
  }
  bitSet(PORTB,BTX);          // create stop bit
  delayMicroseconds(BITDELAY*2);
 
}
