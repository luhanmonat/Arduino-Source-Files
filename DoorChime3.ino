/*
 *    Door Chime w/ Class-D Output
 *    [ATtiny84,8Mhz,Analog Audio]
 *    3/22  Luhan Monat
 *    
 *    Protocol: 5ms Attenstion, 2.5ms gap
 *    Each bit: 0-750usec, 1-1500usec, 750usec gap
 *    
 *    Added delay after each actuation for capacitor recharge
 *    
 */
 
#define NOP delayMicroseconds(3)

//  Port A Pins
#define DOORBUT   0       // Door bell button
#define LEDRED    2
#define LEDBLU    1
#define REEDSWIT  3       // Door open
#define MOTION    4       // Motion sensor
#define LOCKDRV   5
#define LOCKDIR   6
#define RFDATA    7

#define BUG   LEDRED

//  Port B Pins
#define SCL       0
#define SDA       1
#define PWM       2

#define NRBASE    0x10
#define WARNING   0xB0
#define DINGDNG   0xC0
#define DING      0xD0
#define FROG      0xE0



#define ICL     0
#define IRD     1
#define IWR     2
#define INAK    0
#define IACK    1
#define BPS     90

#define DLCODE  0xA751
#define PHCODE  0xA752
#define SNCODE  0xA760

byte  cur_op;

int main() {
byte  cnt,armed,arm2;
word  lockcnt,motioncnt,code;

  DDRA  = 0b01100110;
  DDRB  = 0b00000111;
  PORTA = 0b10011111;     // pull ups on inputs

  //  Setup PWM output
  TCCR0A  = 0b10000011;   
  TCCR0B  = 0b00000001;
  OCR0A   = 128;          // D6 pwm value 

  bitClear(PORTA,LEDRED);
  PlayOut(DING);
  bitSet(PORTA,LEDRED);

cycle:

  Wait(1);
 
  if(bitRead(PINA,RFDATA)) {
    bitClear(PORTA,LEDRED);
    code=GetCode();
    if(code==DLCODE) {
      bitClear(PORTA,LEDBLU);
      DoLock(0);          // got code, unlock
      lockcnt=10000;      // set timer for 10 seconds
      Wait(500);
    }
    if(code==PHCODE) {
      bitClear(PORTA,LEDBLU);
      PlayOutRep(DING,5);
    }
    if((code&0xFFF0)==SNCODE) {
      PlayOut(DING);
      PlayOut(DING);
      Wait(200);
      PlayOut(WARNING);
      Wait(220);
      PlayOut(NRBASE+(code&0x0F)*0x10);
    }
    Wait(200);
    bitSet(PORTA,LEDRED);
    bitSet(PORTA,LEDBLU);
  }

  if(lockcnt) {           // still counting?
    lockcnt--;            // yes.
    if(!lockcnt)          // hit zero now?
      DoLock(1);          // lock the door
  }    

  if(!bitRead(PINA,DOORBUT)) {
    if(arm2) {
      PlayOut(DINGDNG);
      arm2=0;
    } 
  } else {
    arm2=1;
  }
  
    
  if(bitRead(PINA,MOTION)) {
    if(motioncnt) {
      motioncnt--;
      if(!motioncnt) { 
        if(!bitRead(PINA,REEDSWIT))
          PlayOut(FROG);
        motioncnt=2000;         // 2 second timeout for next one
      }
    } 
  } else {
    motioncnt=1;
  }
  

  if(bitRead(PINA,REEDSWIT)) {
    if(armed) {
      PlayOut(DING);
      armed=0;
    }
  } else {
    armed=1;
  }
 
  goto cycle;
  
}

void  BugOut(byte val) {
byte  i,mask;

  bitClear(PORTA,BUG);
  Wait(3);
  mask=0x80;
  for(i=0;i<8;i++) {
    bitSet(PORTA,BUG);
    if(val&mask)
      Wait(2);
    else
      Wait(1);
    bitClear(PORTA,BUG);
    Wait(1);
    mask>>=1;
  }

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
    if(bitRead(PINA,RFDATA))
      goto ok;
  }
  return(0);                    // low state timeout

ok:
  delayMicroseconds(200);
  for(i=1;i<1200;i++) {
    delayMicroseconds(10);
    if(!bitRead(PINA,RFDATA)) {   // went low - return count
       return(i);
    }
  }
  return(0);                    // high state timeout  
}

//  activate push-pull actuator
//  add time to recharge capacitor for next operation

void  DoLock(byte lock) {
static byte locked;

  if(lock==locked)                // do it once only
    return;
  if(!lock) {
    bitSet(PORTA,LOCKDIR);        // reverse for unlock
    locked=0;
  } else {
    locked=1;
  }
  Wait(10);
  bitSet(PORTA,LOCKDRV);          // power to device
  Wait(200);
  bitClear(PORTA,LOCKDRV);
  Wait(10);
  bitClear(PORTA,LOCKDIR);        // always clear relay power
  Wait(2500);                     // recharge time for capacitor 
}

void  PlayOutRep(word block,byte nr) {
byte  i;

  for(i=0;i<nr;i++)
    PlayOut(block);
}

void  PlayOut(word block) {
word  i,addr,len;
byte  data;

  addr=GetEEWord(block);
  len=GetEEWord(block+2);
  I2COpenRead(addr);
  for(i=0;i<len;i++) {
    data=I2CRead();
    OCR0A=data;
    delayMicroseconds(BPS);     
  }
  I2CClose();
  

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


void  WaitEx(word ms) {
word  i;

  for(i=0;i<ms;i++) {
    delayMicroseconds(1000);
    if(!bitRead(PINA,DOORBUT))
      return;
  }
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
