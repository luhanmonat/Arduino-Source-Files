/*
 * Forier Tranform w/no multiplies per sample
 * [Tiny85,8Mhz]
 * Take only 4 samples per cycle at
 * 0,90,180,270 phase angles where sine 
 * and cosine factors are +1 and -1
 * 
 * 11/22 Luhan Monat [ATtiny85,16mhz]
 */


// Port B
#define XBIT    0      //
#define RBIT    1
#define BUG     3

#define CLOCK   16
#define BAUD    9600
#define BITDELAY 1000000/BAUD

#define ACHAN   2
#define FREQ    2600
#define QSAMPLE 250000/FREQ-8     // adjusted for A/D delay


int main() {
byte  x;
int   fft;

  bitSet(DDRB,BUG);

looper:

  fft=FFTX(QSAMPLE,40);
  Printx("\nResult = %d",fft);
  Wait(200);
  goto looper;

}

//  Sample only +/- peaks of sine and cosine
//  Add and subtract instead of multiplies per sample
//  finish with only 2 multiplies and one square root

int   FFTX(int qsamp,byte samps) {
long  sum,sinsum,cossum;
byte  i;

  sinsum=cossum=0;
  for(i=0;i<samps;i++) {
    cossum+=GetSample(qsamp);       // cosine = 1
    sinsum+=GetSample(qsamp);       // sine = 1
    cossum-=GetSample(qsamp);       // cosine = -1
    sinsum-=GetSample(qsamp);       // sine = -1
  }
  sum=sqrt(cossum*cossum+sinsum*sinsum);
  return(sum/samps);
}

int   GetSample(int samptime) {
int   sig;

  delayMicroseconds(samptime);
  bitSet(PORTB,BUG);
  sig=FastAna(ACHAN)-127;
  bitClear(PORTB,BUG);
  return(sig);
}

// AtTiny85 Fast Analog Read for 8Mhz clock
// Returns 8 bit result.

int  FastAna(byte chan) {

  ADCSRA = 0b10000011;      // enable, clock/8
  ADMUX=0b00100000|chan;    // VCC volt ref, left just, +chan
  bitSet(ADCSRA,6);         // start conversion
  while(bitRead(ADCSRA,6))  // wait for ready
    ;
  return(ADCH);            // **** MUST READ ADCL FIRST ****
}



// Replacement for delay() function

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}


//  printf with singe value output
//  (note: adds about 1500 bytes due to 'sprintf')

void  Printx(char *fmt, int dat) {
char  ary[40],i;

  i=0;
  sprintf(ary,fmt,dat);
  while(ary[i])
    Serout(ary[i++]);
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
