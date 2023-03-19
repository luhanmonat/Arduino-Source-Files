/*
 *    Talking Box using Y17Mod1 board
 *    [ATtiny84]
 *    8/22  Luhan Monat
 *    
 *    Interracts with Unicom program over Bluetooth.
 *    Reads filenames form EEProm and sends them as
 *    button legend data on Unicom.
 *    
 *    EEprom data structure
 *    ----------------------------------------------
 *    0x10:  [addr][length][0000][0000]file1(cr);
 *    0x20:  [addr][length][0000][0000]file2(cr);
 *    0x30:  [addr][length][0000][0000]file3(cr);
 *    0x40:  [addr][0000] // end of directory
 *   
 *    Send out directory filenames as:
 *    B,file1,file2,file3 (etc)
 */
 
#define NOP delayMicroseconds(3)


//  Port A Pins
#define RxD       0       // Serial data from HC-06
#define TxD       3       // Serial data to HC-06
#define SERVO     4       // R/C Servo
#define OUTX2     5
#define OUTX1     6
#define PWR       7       // Power up driver

//  Port B Pins
#define SCL       0
#define SDA       1
#define PWM       2

#define DING      0x10
#define WEST      0x20

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
byte  index,entries,i,x;
int   val;

  DDRA  = 0b11111000;
  DDRB  = 0b00000111;
  
  TCCR0A  = 0b10000011; 
  TCCR0B  = 0b00000001;
  OCR0A   = 128;          // D6 pwm value 

cycle:

  x=Serin();
  if(x!='/') 
    goto cycle;
  x=Serin();
  index=x-'A';
  val=GetDec();
  
  switch(x) {
    case '*': Serout(0); Serout('\r'); Wait(500);  // clear the line
              Printx("T,Frog Box 1.1\r",0); Wait(1000);
              entries=SendDirectory();
              Wait(2000);
              Printx("D1,Entries=%d\r",entries);
              break;
    default: if(index<entries)
               PlayOut(index*0x10+0x10);
             break; 
              
  }
  
  Printx("!\r",0);
  goto cycle;
  
}

word  SendDirectory() {
word  i,j,cnt,addr,len,loc;
byte  dat;

  addr=0x10;      // base address of directory
  Printx("B,1",0);
  for(i=0;i<12;i++) {
    cnt=i;
    len=GetEEWord(addr+2);
    if(len==0) goto done;
    Serout(',');
    I2COpenRead(addr+8); // name of file
    for(j=0;j<7;j++) {
      dat=I2CRead();
      if(dat==0) break;
      Serout(dat);
    }
    I2CClose();
    Wait(100);
    addr+=0x10;     // next entry
  }
done:
  Serout('\r');
  return(cnt);
}

void  BoxOpen(byte OPEN) {
  if(OPEN)
    ServoPulse(9,25);
  else
    ServoPulse(18,25);
}

void  ServoPulse(byte tms, byte nr) {
byte  i;

  for(i=0;i<nr;i++) {
    bitSet(PORTA,SERVO);
    delayMicroseconds(tms*100);
    bitClear(PORTA,SERVO);
    Wait(20);
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


void  PlayOut(word block) {
word  i,addr,len;
byte  data;

  BoxOpen(1);
  bitSet(PORTA,PWR);
  addr=GetEEWord(block);
  len=GetEEWord(block+2);
  I2COpenRead(addr);
  for(i=0;i<len;i++) {
    data=I2CRead();
    OCR0A=data;
    delayMicroseconds(BPS);     
  }
  I2CClose();
  bitClear(PORTA,PWR);
  BoxOpen(0);
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

byte  Serin() {
byte  i,data;

  data=0;
  bitClear(DDRX,INBIT);
  while(bitRead(PINX,INBIT)) ;      // wait for start bit
  delayMicroseconds(BITDELAY/2);    // Middle of start bit
  for(i=0;i<8;i++) {
    delayMicroseconds(BITDELAY);    // middle of next bit
    if(bitRead(PINX,INBIT))
      bitSet(data,i);
  }
  delayMicroseconds(BITDELAY);      // Middle of stop bit
  return(data);
}


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
