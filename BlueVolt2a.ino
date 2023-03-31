/*
 * Remote 4 chan voltmeter w/ 3 outputs
 * 8/22 Luhan Monat [ATtiny84]
 * Modified for power on/off button using Aux-4 output
 * [Drawing BlueVolt2a]
 */

#define ADREF 490        // 4.86 volts at vcc

//  Port A

enum  abits {Y1,Y2,Y3,Y4,X1,X2,X3,SWIT};


// Port B

#define XBIT    1       // (reverse x/r for tiny85)
#define RBIT    0
#define PWR     2

#define CLOCK   8
#define BAUD    9600
#define BITDELAY 1000000/BAUD

#define BUG     5
#define BUFSIZE 40

byte  Buffer[BUFSIZE],bufpnt;
byte  END;


int main() {
byte  index,x;
int val,adata;


  DDRA  = 0b01110000;
  PORTA = 0b10000000;     // Pull up on bit-7
  DDRB  = 0b0110;

  ADCSRA  = 0b10000100;   // B7-enable,B6-START,B5-auto,/16 clk
  ADCSRB  = 0b00000000;   // B4-left justify
  DIDR0   = 0b00001111;

sleep:

  bitClear(PORTB,PWR);      // Turn off bluetooth
  bitClear(PORTB,XBIT);     // Disable pull up
  bitClear(ADCSRA,7);       // Turn off A/D

  Wait(1000);
  Sleeper();
  bitSet(PORTB,PWR);        // turn on bluetooth
  bitSet(PORTB,XBIT);
  bitSet(ADCSRA,7);

  Wait(1000);

//  Send name of device to bluetooth serial device
//  using format for cheap, off-brand units

  Printx("AT+NAMEBlueVolt\r\n",0);    // no '=' sign for these units
  
cycle:

  if(!bitRead(PINA,SWIT))        // check push-button
    goto sleep;

  x=Serstat();
  if(!x)
    goto cycle;
    
  x=Serin();
  if(x!='/') 
    goto cycle;

  x=Serin();
  index=x-'A';
  val=GetDec();
  
  switch(x) {
    case '*': Serout(0); Serout('\r'); Wait(100);  // clear the line
              Printx("T,Blue Volt 2\r",0); Wait(1000);
              Printx("B,1,READ,,,AUX1,AUX2,AUX3,,,,,,OFF\r",0);
              break;
    case '#': ReadAnalogs(20); break;
    case 'A': if(val) ReadAnalogs(100); break;
    case 'D': 
    case 'E': 
    case 'F': if(!val) {
                bitSet(PORTA,index+1);
              } else {
                if(val>10) {
                  Printx("C,%d,YEL\r",index+1);
                } else {
                  bitClear(PORTA,index+1);
                  Printx("C,%d,BLK\r",index+1);
                }
              }
              break;
     case 'L': if(val>10)
                 goto sleep;
               break;     
  }
  goto cycle;
  
  
}


void  Sleeper() {

  bitSet(GIMSK,PCIE0);      // pin change port-a
  bitSet(PCMSK0,7);         // pin change bit-7
  MCUCR = 0b00110000;       // enable sleep power down
  bitRead(PORTA,SWIT);      // may be needed (?)
  bitSet(SREG,7);
  PRR   = 0b00001111;       // shut down peripherals
  asm("sleep \n");
  PRR   = 0b00000000;       // enable all peripherals
}

ISR (PCINT0_vect) { }


void  ReadAnalogs(byte rate) {
byte  i;
word   data;

  for(i=0;i<4;i++) {
    data=FastAnalog(i);
    data=(long)data*25/10;      //  240k/10k
    Printx("D%d,",i+1);
    PrintVal("%03d",data,2);
    Printx(" Volts\r",0);
    Wait(rate);
  }
}

// AtTiny84 Fast Analog Read for 8Mhz clock
// Returns 10 bit result.

word  FastAnalog(byte chan) {
word  vh,vl;

 

  ADMUX=0x80|chan;          // 1.1 volt ref, +chan
  Wait(1);
  bitSet(ADCSRA,6);         // start conversion
  while(bitRead(ADCSRA,6))  // wait for ready
    ;
  vl=ADCL;            // **** MUST READ ADCL FIRST ****
  vh=ADCH;
  vl+=vh<<8;
  return(vl);       // 16 bit data
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


void  PrintVal(char *fmt, int dat, byte dp) {
char  ary[40],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i]) {
    if(i==strlen(ary)-dp)
      Serout('.');
    Serout(ary[i++]);
  }
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
char  ary[40],i;

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

// Bit Bang input on port bit (RBIT)

byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRB,RBIT);
  while(bitRead(PINB,RBIT)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2-3);      // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY-2);    // middle of next bit
    if(bitRead(PINB,RBIT))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY-2);      // Middle of stop bit
  return(data);
}


// Outputs single byte (ch)

void  Serout(byte ch) {
byte  i;

  if(ch=='\n') Wait(5);
  bitSet(DDRB,XBIT);            // set for output
  bitClear(PORTB,XBIT);        // create start bit
  delayMicroseconds(BITDELAY-2);
  for(i=0;i<8;i++) {
    if(bitRead(ch,i))
      bitSet(PORTB,XBIT);
    else
      bitClear(PORTB,XBIT);
    delayMicroseconds(BITDELAY-2);
  }
  bitSet(PORTB,XBIT);          // create stop bit
  delayMicroseconds(BITDELAY*2);
 
}
