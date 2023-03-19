/*
 *    BirdCage
 *    [ATtiny84] (Y55 board)
 *    2/23  Luhan Monat
 *    
 *    EEprom data structure
 *    ----------------------------------------------
 *    0x10:  [addr][length][0000][0000]file1(cr);
 *    0x20:  [addr][length][0000][0000]file2(cr);
 *    0x30:  [addr][length][0000][0000]file3(cr);
 *    0x40:  [addr][0000] // end of directory
 */
 
#define NOP delayMicroseconds(3)


//  Port A Pins

#define TxD       1       // I/O bit 2
#define BUGX      4
#define BTPWR     5
#define AUPWR     7       // Power up driver

//  Port B Pins
#define SCL       0
#define SDA       1
#define PWM       2

#define ICL     0
#define IRD     1
#define IWR     2
#define INAK    0
#define IACK    1
#define BPS     90

// Set these up for Serin(), Serout().

#define CLOCK   8
#define BAUD    9600
#define PORTX   PORTA
#define DDRX    DDRA
#define PINX    PINA
#define OUTBIT  TxD
#define INBIT   RxD
#define BITDELAY 1000000/BAUD

byte  cur_op;

int main() {
byte  rando,entries,i,x;
word  leng;
int   val;

restart:

  DDRA  = 0b11111000;
  DDRB  = 0b00000111;
  

  OCR0A   = 128;          // D6 pwm value

  bitSet(PORTA,BTPWR);
  bitClear(DDRB,PWM);
  bitClear(PORTA,AUPWR);
  WDTCSR=0b00101000;              // 4 second wdt
 
  Printx("Hello",0);

  entries=0;
  for(x=0x10;x<0x100;x+=0x10) {
    leng=GetEEWord(x+2);
    if(!leng) break;
    entries++;
    asm("WDR \n");
    PlayOut(x);
  }

  Printx("\nItems=%d",entries);


cycle:

  Wait(1000);
  bitClear(PORTA,BUGX);
  asm("WDR \n");
  rando=RandomNR();
  if(rando<entries) {
    PlayOut(0x10+rando*0x10);
  } 
  goto cycle;
  
}

byte RandomNR() {
static byte b1,b2,b3;

  b2+=b1;
  b3+=b2;
  b1+=Rotate(b3)+7;
  return(b2);
 
}

byte  Rotate(byte val) {

  return((val<<7)|(val>>1));
}


void  PlayOut(word block) {
word  i,addr,len;
byte  data;

  bitSet(PORTA,AUPWR);
  TCCR0A  = 0b10000011; 
  TCCR0B  = 0b00000001;
  bitSet(DDRB,PWM);
  addr=GetEEWord(block);
  len=GetEEWord(block+2);
  I2COpenRead(addr);
  for(i=0;i<len;i++) {
    data=I2CRead();
    OCR0A=data;
    delayMicroseconds(BPS);     
  }

  I2CClose();
  TCCR0A  = 0b10000000; 
  TCCR0B  = 0b00000001;
  bitClear(PORTA,AUPWR);
}


word  GetEEWord(word addr) {
word  loc;
byte  low,high;

  I2COpenRead(addr);
  low=I2CRead();
  high=I2CRead();
  loc=(high<<8)+low;
  I2CClose();
  return(loc);
}



void  Wait(word ms) {
word  i;

  for(i=0;i<ms;i++) {
    delayMicroseconds(1000);
  }
}


//      ****** I2C Basic Operations *******
//      Use these to access EEPROM

void  I2COpenWrite(word addr) {
  I2CAddr(addr);
  cur_op=IWR; 
}


void  I2COpenRead(word addr) {

  I2CAddr(addr);
  I2CStart();
  I2CWbyte(0xA1);
  cur_op=IRD;
}


byte  I2CRead() {

    return(I2CRbyte(IACK));
}

    //  EE Primitive only below this line //

void  I2CAddr(word locus) {

    I2CStart();
    I2CWbyte(0xA0);
    I2CWbyte(locus>>8);
    I2CWbyte(locus&0xFF);
    
}

void  I2CClose() {
  
  if(cur_op==IRD) {
    I2CRbyte(INAK);           // Read w/ no ACK
    I2CStop();
  }
  if(cur_op==IWR) {
    I2CStop();    
    Wait(5);
  }
  cur_op=ICL;
}


void  I2CStart() {
  bitSet(DDRB,SCL); NOP;      // SCL always output
  bitSet(DDRB,SDA); NOP;     // SDA is output
  bitSet(PORTB,SDA); NOP;     // SDA is high
  bitSet(PORTB,SCL); NOP;     // clock is high
  bitClear(PORTB,SDA);  NOP;   // START condition
  bitClear(PORTB,SCL);  NOP;   // drop the clock line

}

void  I2CStop() {

  bitClear(PORTB,SDA); NOP;
  bitSet(PORTB,SCL);  NOP;     // Raise the clock line
  bitSet(PORTB,SDA);  NOP;    // STOP  condition
  
}


byte  I2CRbyte(byte ack) {
byte  i,x,dat;

  bitClear(DDRB,SDA);
  bitSet(PORTB,SDA);
  dat=0;
  for(i=0;i<8;i++) {
    bitSet(PORTB,SCL); NOP;
    x=bitRead(PINB,SDA); NOP;
    bitClear(PORTB,SCL); NOP;
    dat<<=1;
    bitWrite(dat,0,x); NOP;
  }
  bitSet(DDRB,SDA); NOP;
  if(ack==IACK)
    bitClear(PORTB,SDA);
  else
    bitSet(PORTB,SDA);
  NOP;
  bitSet(PORTB,SCL); NOP;
  bitClear(PORTB,SCL); NOP;
  bitClear(PORTB,SDA); NOP;
  return(dat);
}

byte  I2CWbyte(byte dat) {
volatile byte  i,x,mask;

  
  mask=0x80;
  for(i=0;i<8;i++) {
    if(dat&mask)
      bitSet(PORTB,SDA);
    else
      bitClear(PORTB,SDA);
    NOP;
    bitSet(PORTB,SCL);  NOP;
    mask>>=1;
    bitClear(PORTB,SCL); NOP;
  }
  bitClear(DDRB,SDA); NOP;   // Set for input
  bitSet(PORTB,SCL); NOP;
  x=bitRead(PINB,SDA); NOP; // Read ACK bit
  bitClear(PORTB,SCL); NOP;
  bitSet(DDRB,SDA); NOP;
  bitClear(PORTB,SDA); NOP;
  return(x);
}

//      ***** End I2C ******

// printf with singe value output

void  Printx(char *fmt, word dat) {
char  ary[40],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i]) {
   delayMicroseconds(200);
   Serout(ary[i++]);
  }
   
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
  delayMicroseconds(BITDELAY);
  delayMicroseconds(BITDELAY);
  
}
