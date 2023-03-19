/*
 
*/

#define BUFSIZE   30
#define XMITBIT    0
#define RECVBIT    1

#define CLOCK   16
#define BAUD    9600
#define BITDELAY 1000000/BAUD-2

byte  charbuffer[BUFSIZE];
byte  *ptr,endline,ErrorFlag,chars;


int main() {
int i,j,dat,cmd;

/*
PLLCSR  = 0b00000110;
GTCCR   = 0b01010000;
TCCR1   = 0b00010001;
OCR1A   = 40;
OCR1B   = 130;

bitSet(DDRB,3);
bitSet(DDRB,4);
*/

Printx("\nMonitor 85",0);
  
  // main program loop starts here
  
rerun:
  endline=0;
  ErrorFlag=0;
  Printx("\nMon:",0);
  FillBuffer();
  
next:
  cmd=GetBuffer();
  if(endline) cmd='\r';
  switch (cmd) {
    case ' ':
    case ',': break;
    case  0 : Printx(" ok",0); goto rerun;
    case '.': ReadBack(); goto next;
    case 'W': SendData(); goto next;
    case 'R': ReadData(); goto next;
    case 'D': DumpRam(); goto rerun;
    case 'T': DoTest();  break;
  }
  goto next;

}

void  DoTest() {


}

void  DumpRam() {
byte  i,data,*p;
int   check;

  check=0;
  for(i=0;i<0x80;i++) {
    if((i%16)==0)
      Printx("\n %02X",i);
    if((i%4)==0)
      Printx(" ",0);
    p=i;
    data=*p;
    check+=data;
    Printx(" %02X",data);
  }
  Printx("\nCheck = %04X",check);
}

  

void RecvHex() {
byte  data,*pnt;
  
  
    Printx(" %02X",data);

}

// Convert monitor ascii to decimal value

word  GetDecimal() {
  word result;
  char digit;

  result=0;
  while(1) {
    digit=toupper(GetBuffer());
    if(digit=='\r')
      endline=1;
    if((digit>='0')&&(digit<='9'))
      result=result*10+(digit-'0');
    else
      return(result);
  }   
}

void  SendData() {
byte  data;

    ptr=GetHex();
    data=GetHex();
    *ptr=data;
}  


void  ReadData() {
byte  data;

  ptr=GetHex();
  data=*ptr;
  Printx("->%02X",data);

}

void  ReadBack() {
byte  data;

  data=*ptr;
  Printx("%02X",data);
  
}

// output hex value to monitor

byte  GetHex() {
byte  cc,data,*ptr;

  cc=GetBuffer();
  data=Nibble(cc)<<4;
  cc=GetBuffer();
  data+=Nibble(cc);
  return(data);

}


byte Nibble(char x) {

  x=toupper(x);
  if((x>='0')&&(x<='9'))
    return(x-'0');
  if((x>='A')&&(x<='F'))
    return(10+(x-'A'));
  ErrorFlag=x;
  return(0);
}





void  Wait(word ms) {
word  i;

  for(i=0;i<ms;i++)
    delayMicroseconds(1000);
}


// Works like 'printf()' for single data byte

void  Printx(char *fmt, int dat) {
char  i,ary[40];

  sprintf(ary,fmt,dat);
  for(i=0;i<40;i++)
    if(ary[i])
      Serout(ary[i]);
    else
      return;
}


void  FillBuffer() {
char  c;

  chars=0;
  while(chars<BUFSIZE) {
    c=Serin();
    switch(c) {
      case '\r':
      case '\n': goto done;
      default  : charbuffer[chars++]=c;
    }
  }
done:
  charbuffer[chars]=0;
  chars=0;
  return;
}


char  GetBuffer() {
char  c;

    c=charbuffer[chars];
    c=toupper(c);
    if(c) {
      chars++;
      Serout(c);
    }
    return(c);
 
}


// check for keyboard hit to abort runaway operations

byte  Exout() {
  
  if(bitRead(PINB,RECVBIT))
    return(0);
  return(1);
}

// Bit Bang input on any port bit (inbit)
// may use any bit on specified port

byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRB,RECVBIT);
  while(bitRead(PINB,RECVBIT)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2-3);      // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY);    // middle of next bit
    if(bitRead(PINB,RECVBIT))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


// Outputs single byte (ch) to designated bit (outbit).
// may use any bit on specified port

void  Serout(byte ch) {
byte  i;

  if(ch=='\n') Wait(5);
  bitSet(DDRB,XMITBIT);            // set for output
  bitClear(PORTB,XMITBIT);        // create start bit
  delayMicroseconds(BITDELAY);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTB,XMITBIT);
    else
      bitClear(PORTB,XMITBIT);
    delayMicroseconds(BITDELAY);
  }
  bitSet(PORTB,XMITBIT);          // create stop bit
  delayMicroseconds(BITDELAY*2);

  
}
