/*
 * Puzzle Box
 * Coded for Arduino ATtiny84
 */

//  define port and bits used on nano
//  change these for other boards


#define SPKR    7     // Speaker drive bit


//    PORTA bit assignments

#define SCL   3       // I2C clock
#define SDA   4       // I2C data
#define INT   5       // hit detected (low)

#define RED   0
#define GRN   1
#define YEL   2
#define BLU   6

//    PORTB bit assignments

#define SERVO 2

//    Other bits

#define IACK  0
#define NACK  1


void setup() {
  Serial.begin(9600);
  delayMicroseconds(20);
  I2CStop();
  I2CWbyte(0);
  TSWrite(0,0x00);        // reset normal sensitivity
  TSWrite(0x30,0x40);     // threshholds (0x40)
  TSWrite(0x1F,0x2f);     // reduce sensitivty 0f-7f(2F)
  TSWrite(0x28,0x00);     // no auto repeats
  TSWrite(0x44,0x41);     // INT on press only
  bitSet(DDRB,SERVO);
}


// Test out the functionality of the software

void loop() {
byte  data;

xyz:
  
  Servo(20);      //ccw
  delay(300);
  Servo(10);       //cw
  delay(500);
  goto xyz;
  


 TestLites();

 Printx("\nArmed\n",0);

next: 
  data=TSButton();
  if(!data)
    goto next;
  Printx("\nSensor: %02X",data);
  goto next; 

}

void  Servo(byte timer) {
int i;

  for(i=0;i<20;i++) {
    bitSet(PORTB,SERVO);
    delayMicroseconds(timer*100);
    bitClear(PORTB,SERVO);
    delay(50);
  }
}

void  TestLites() {

  DDRA  = 0xFF;
xyz:
  bitClear(PORTA,RED); delay(500);
  bitSet(PORTA,RED); delay(500);
 
  bitClear(PORTA,GRN); delay(500);
  bitSet(PORTA,GRN); delay(500);

  bitClear(PORTA,YEL); delay(500);
  bitSet(PORTA,YEL); delay(500);
  
  bitClear(PORTA,BLU); delay(1000);
  bitSet(PORTA,BLU); delay(1000);

  goto  xyz;

  
}

//  running on ATmega328 at 16 Mhz
//  ok to 4000 msecs and 2000 Hz

void  Frequency(word freq, word msecs) {
word  j,sum,timer;

    timer=msecs*16;
    for(j=0;j<timer;j++) {
      delayMicroseconds(63);
      sum+=freq;
      if(sum&0x2000)
        bitSet(PORTA,SPKR);
      else
        bitClear(PORTA,SPKR);
    }
    bitClear(PORTA,SPKR);
}


// Substitute for Arduino not having printf()
// Takes only 1 variable

void  Printx(char *fmt, int dat) {
char  ary[40];

  sprintf(ary,fmt,dat);
  Serial.print(ary);
}


// Read Touch Sensors 1-8
// Return 0 for none hit

byte  TSButton() {
byte  data,i;

  if(bitRead(PINA,INT))     // read the harware ALERT/INT line
    return(0);              // return if none detected
  TSWrite(0,0x40);          // Clear the results
  data=TSRead(3);           // (finger is still there) get all 8 sensor bits
  for(i=0;i<8;i++) {
    if(bitRead(data,i))     // scan for activ bit nr.
      return(i+1);          // range = 1-8
  }
  return(9);                // this should not ever happen
}


//  Read specified touch switch register using I2C protocol

byte  TSRead(byte reg) {
byte data;

  I2CStart();           // must always have START
  I2CWbyte(0x50);       // write mode
  I2CWbyte(reg);        // uses single byte for register address
  I2CStart();           // another START
  I2CWbyte(0x51);       // now go into read mode
  data=I2CRbyte(NACK);  // read just 1 byte w/no ACK
  I2CStop();            // always end with STOP
  return(data);
}

// Write specified data to specified register

byte  TSWrite(byte reg,byte dat) {

    I2CStart();         // always have START
    I2CWbyte(0x50);     // write mode
    I2CWbyte(reg);      // uses single byte for register address
    I2CWbyte(dat);      // write to register
    I2CStop();          // must always end with STOP
}


//  ******************************************
//  ******** Standard I2C functions **********
//  ******************************************

void I2CStart() {
  
  bitSet(DDRA,SCL); x();         // clock is always output
  bitSet(PORTA,SDA); x();        // data line to output mode
  bitSet(PORTA,SCL); x();        // set clock high
  bitClear(PORTA,SDA); x();      // lower data line for start condition
  bitClear(PORTA,SCL); x();      // now set clock low
}

void I2CStop() {

  bitSet(PORTA,SCL); x();         // raise clock line first
  bitSet(PORTA,SDA); x();         // then raise data line for stop condition
}

// Write one byte via I2C

byte  I2CWbyte(byte dat) {
volatile byte  i,k,mask;

  mask=0x80;
  for(i=0;i<8;i++) {
    if(dat&mask)                // check current data bit
      bitSet(PORTA,SDA);        // transfer to data line if high
    else
      bitClear(PORTA,SDA);      // ... or low
    x();
    bitSet(PORTA,SCL); x();     // raise clock line
    mask>>=1;                   // shift the mask bit
    bitClear(PORTA,SCL); x();   // lower clock line
  }
 
  bitClear(DDRA,SDA); x();      // Set for input
  bitSet(PORTA,SCL); x();       // clock line high
  k=bitRead(PINA,SDA); x();     // Read ACK bit
  bitClear(PORTA,SCL); x();     // lower the clock
  bitSet(DDRA,SDA); x();        // default to data output
  bitClear(PORTA,SDA); x();     // leave with data line low
  return(k);                    // return with ACK bit
}


// read byte with ACK unless nack.

byte  I2CRbyte(byte nack) {
byte  i,k,dat;

  bitClear(DDRA,SDA); x();    // set data line for input
  dat=0;                      // z out initial data value
  for(i=0;i<8;i++) {          // set for 8 count
    x();
    bitSet(PORTA,SCL); x();   // raise clock line
    k=bitRead(PINA,SDA); x(); // then read the data line
    bitClear(PORTA,SCL); x(); // lower clock line
    dat<<=1;                  // data is MSB first so shift left
    bitWrite(dat,0,k);        // put the next bit into value
  }
  bitSet(DDRA,SDA); x();      // switch to data output
  if(nack)                    // do we ACK this time?
    bitSet(PORTA,SDA);        // high = no ACK
  else
    bitClear(PORTA,SDA);      // low = ACK
  x();
  bitSet(PORTA,SCL); x();     // start of clock pulse
  bitClear(PORTA,SCL); x();   // end clock pulse
  bitClear(PORTA,SDA); x();   // return with data line low
  return(dat);                // return data
}

// just a small delay to keep clocks under 400khz

void  x() {
volatile byte z;

 delayMicroseconds(100);

}
