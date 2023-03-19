/*
 * Send and recieve bytes via SPI or I2C.
 * 3/2021 Luhan Monat
 * Coded for Arduino Nano
 * 
 * This program provides live interaction from the monitor window for the purpose
 * of trying out access to registers inside a target device.  All bytes are sent and
 * received as two hexidecimal digits.  By default, bytes are transmitted if they are
 * valid hexidecimal values.
 *
 *  Commands    (upper or lower case)
 *  ------------------------------------------  
 *  xxyy zz     Send bytes as any two hex digits (spaces optional)
 *  .ddd        Read ddd (decimal) bytes as hex (end with space or end of line)
 *  /           Create START condition (select)
 *  ;           Create STOP (automatic at end of line)
 *  R           Reverse bit sense
 *  M           Mode SPI, I2C 
 *  Z           Change microsecond delay factor
 *  Wnnn        Wait nnn milliseconds
 *  S           Scan for all I2C addresses
 *  *ddd        Repeat last command byte sent ddd times
 */

//  define port and bits used on nano
//  change these for other boards

#define PORT      PORTA
#define PORTIN    PINA
#define PORT_DIR  DDRA


//  I2C Mode assign bits to I/O functions

#define SCL   0       // I2C clock
#define SDA   1       // I2C data *** PULLUP RESISTOR !!! ***

// other defines used internally by program

#define IACK  1
#define SPIMODE 1
#define I2CMODE 2
#define START 2
#define STOP  3


int   dfacto,mode,reverse,space,index;
char  ErrorFlag;
char  buff[40];
byte  last,endline;



void setup() {
  
  Serial.begin(9600);

  dfacto=10;          // speed control
  mode=I2CMODE;
  DoSel(STOP);
}

void loop() {
  int i,j,dat,cmd;

  Serial.println("Prober Vers(84) 4.4");


  // main program loop starts here
  
  rerun:
  endline=0;
  ErrorFlag=0;
  Serial.println();
  Serial.print("Prb");
  if(reverse) 
    Serial.print("/r");
  Serial.print(">");
  while(Serial.available()) 
  Serial.read();            // clear out input buffer
 
  next:
  cmd=GetSerial();
  if(endline) cmd='\r';
  switch (cmd) {
    case ' ':
    case ',': break;
    case '/': DoSel(START); break;
    case ';': DoSel(STOP); delay(1); break;
    case '\r':
    case '\n': Serial.print(" ok"); goto rerun;
    case 'R': reverse^=1; break;
    case 'Z': dfacto=GetDecimal(); goto rerun;
    case 'S': ScanI2C(); break;
    case '.': RecvHex(); goto rerun;
    case 'W': delay(GetDecimal()); break;
    case '*': Repeat(); goto rerun;
    default:  SendHex(cmd);
              if(ErrorFlag) {
                Serial.write(ErrorFlag);    // show bad character
                Serial.print("?");
                delay(400);
                goto rerun;                 // abort the rest
              }
  }
  goto next;

}

// SPI: Select or Deselect chip
// I2C: Create START or STOP condition

void DoSel(int func) {

  if(func==START) {
    bitSet(DDRB,SCL); Dx();
    bitSet(PORTB,SDA); Dx();
    bitSet(PORTB,SCL); Dx();
    bitClear(PORTB,SDA); Dx(); // start condition
    bitClear(PORTB,SCL); Dx();
  }
  if(func==STOP) {
    bitSet(PORTB,SCL); Dx();
    bitSet(PORTB,SDA); Dx(); Dx(); // stop condition
  }       

}

//  Scan the entire I2C address space
//  Even numbers (write commands) only

void  ScanI2C() {
  int i,ack;

  Serial.print("\nScanning - ");
  for(i=0;i<256;i+=2) {
    delay(10);            // delay for dramatic effect
    DoSel(START);         // create start condition
    ack=I2CWbyte(i);      // anybody home?
    DoSel(STOP);          // complete operation
    if(!ack) {            // zero = ACK true
      Printx("\nWrite Address = 0x%02X",i);
      Printx("\nRead Address  = 0x%02X\n",i+1);
      delay(200);    
    }
  }
}


// Read some bytes from device (in hex)

void RecvHex() {
  int loops,i,data;
  
  loops=GetDecimal();
  for(i=0;i<loops;i++) {
    if(i%20==0)
      Serial.println();
    if(i==(loops-1))
      data=SwitchIn(1);     // no ack on last byte
    else
      data=SwitchIn(0);     // normal ack on each byte
    Printx(" %02X",data);
  }
}

// Convert monitor ascii to decimal value

word  GetDecimal() {
  word result;
  char digit;

  result=0;
  while(1) {
    digit=toupper(GetSerial());
    if(digit=='\r')
      endline=1;
    if((digit>='0')&&(digit<='9'))
      result=result*10+(digit-'0');
    else
      return(result);
  }   
}

// repeat last byte sent

void  Repeat() {
int i,count;

  count=GetDecimal()-1;       // one already sent
  for(i=0;i<count;i++)
    SwitchOut(last);
  
}

// output hex value to monitor

void SendHex(char cc) {
int dat,low;
  
  dat=Nibble(cc)<<4;
  low=toupper(GetSerial());
  dat+=Nibble(low);
  if(!ErrorFlag)
    SwitchOut(dat);
  last=dat;               // save for repeats
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

// Send out data to SPI or I2C

void SwitchOut(byte data) {
byte rtn;
  
  rtn=I2CWbyte(data);
  if(rtn==0) Serial.print("!");
  else    Serial.print("?");
  
}

// Read byte from SPI
// Read byte from I2C (optional no-ack)

byte  SwitchIn(byte noack) {
byte  data;

  data=I2CRbyte(noack);
  return(data);
}

// Read monitor command data

int GetSerial() {
byte  data;

  while(!Serial.available())
    ;
  data=toupper(Serial.read());
  Serial.write(data);
  return(data);
}



// Write byte to I2C interface
// Return: ACK value from target device

byte  I2CWbyte(byte dat) {
volatile byte  i,x,mask;

  mask=0x80;
  for(i=0;i<8;i++) {
    Dx();
    if(dat&mask)
      bitSet(PORTB,SDA);
    else
      bitClear(PORTB,SDA);
    Dx();
    bitSet(PORTB,SCL); Dx();
    mask>>=1;
    bitClear(PORTB,SCL);
  }
  Dx();
  bitClear(DDRB,SDA);    // Set for input
  bitSet(PORTB,SCL); Dx();
  x=bitRead(PINB,SDA); Dx();   // Read ACK bit
  bitClear(PORTB,SCL);
  bitSet(DDRB,SDA); Dx();
  bitClear(PORTB,SDA);
  return(x);
}


// read byte with ACK unless nack.

byte  I2CRbyte(byte nack) {
byte  i,x,dat;

  bitClear(DDRB,SDA);
  dat=0;
  for(i=0;i<8;i++) {
    bitSet(PORTB,SCL); Dx();
    x=bitRead(PINB,SDA); 
    bitClear(PORTB,SCL); Dx();
    dat<<=1;
    bitWrite(dat,0,x);
  }
  bitSet(DDRB,SDA);
  if(nack)
    bitSet(PORTB,SDA);
  else
    bitClear(PORTB,SDA);
  Dx();
  bitSet(PORTB,SCL); Dx();
  bitClear(PORTB,SCL); Dx();
  bitClear(PORTB,SDA); Dx();
  return(dat);
}


// This delay help make data easier to see on scope.

void Dx() {

  delayMicroseconds(dfacto);

}


// Works like 'printf()' for single data byte

void  Printx(char *fmt, int dat) {
char  ary[40];

  sprintf(ary,fmt,dat);
  Serial.print(ary);
}
