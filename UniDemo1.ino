
/*
 *  UniCom Portable Demo Device 1
 *  [ATtiny85,8Mhz]
 *  10/22 Luhan Monat
 *  
 */

#define RBIT  1
#define XBIT  2
#define BUT   0
#define PWR   3
#define SPKR  4

#define CLOCK   8
#define BAUD    9600
#define BITDELAY 1000000/BAUD

byte  xflag;


int main() {
byte  sync;
byte  cmd;
word  val;

restart:

  bitSet(DDRB,XBIT);
  bitSet(DDRB,PWR);
  bitSet(DDRB,SPKR);
  bitSet(PORTB,BUT);      // Pull up on button input
  bitSet(PORTB,PWR);      // turn on Bluetooth device

  Beep(1000,100);
  Beep(300,300);

cycle:

  if(!bitRead(PINB,BUT)) {
    goto shutdown;
 
  }

  
  if(bitRead(PINB,RBIT))
    goto cycle;

  sync=Serin();
  if(sync!='/')              // must have slash
    goto cycle;
  cmd=Serin();              // get command char
  val=GetDec();           // val=0 key down, val>0 key up 10ths of second
  
  switch(cmd) {
    case '*': Serout(0); Wait(2); Serout('\r');  // clear the line
              Printx("T,UniCom Demo 0.1\r",0); Wait(500);
              Printx("B,1,Tone1,Tone2,Tone3,,,,,,,,Sleep,x\r",0);
              break;
    case 'A': if(!val) Beep(300,1000); break;
    case 'B': if(!val) Beep(200,1000); break;
    case 'C': if(!val) Beep(400,1000); break;
    case 'K': if(val>10) goto shutdown;
              break;
   
  }
  goto cycle;

shutdown:

    Wait(1000);
    Beep(300,300);
    Beep(1600,100);
    bitClear(PORTB,PWR);
    bitClear(DDRB,PWR);
    bitClear(DDRB,XBIT);
    bitClear(DDRB,SPKR);
    Sleeper85();
    Wait(1000);
    goto restart;
}

void  Sleeper85() {

  bitSet(GIMSK,PCIE);      // pin change port-a
  bitSet(PCMSK,PCINT0);    // pin change bit-0
  MCUCR = 0b00110000;       // enable sleep power down
  bitSet(SREG,7);
  bitRead(PINB,BUT);
  PRR   = 0b00001111;       // shut down peripherals
  asm("sleep \n");
  PRR   = 0b00000000;       // enable all peripherals

}

ISR (PCINT0_vect) {

}

void  Beep(word dla, word cnt) {
word  i;

  for(i=0;i<cnt;i++) {
    bitSet(PORTB,SPKR);
    delayMicroseconds(dla);
    bitClear(PORTB,SPKR);
    delayMicroseconds(dla);
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

  for(i=0;i<timer;i++) {
    if(Serstat()) xflag=1;
    delayMicroseconds(1000);
  }
}


//  printf with singe value output to designated pin
//  (note: adds about 1500 bytes due to 'sprintf')

void  Printx(char *fmt, long dat) {
char  ary[100],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i])
    Serout(ary[i++]);
}

//  Return TRUE on serial data detected

byte  Serstat() {
  bitClear(DDRB,RBIT);
  if(bitRead(PINB,RBIT))
    return(0);
  return(1);
}

// Bit Bang input on any port bit (RBIT)
// may use any bit on specified port

byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRB,RBIT);
  while(bitRead(PINB,RBIT)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2);      // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY);    // middle of next bit
    if(bitRead(PINB,RBIT))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


// Outputs single byte (ch) to designated bit (XBIT).
// may use any bit on specified port

void  Serout(byte ch) {
byte  i;

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
