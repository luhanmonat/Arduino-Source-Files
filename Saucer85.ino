 /*
 * (non) Flying Saucer with light stip
 * 12/21 Luhan Monat
 * Using 60 LED RGB Lightstrip
 */

#define GRN 0xFF0000L
#define RED 0x00FF00L
#define BLU 0x0000FFL
#define WHT 0xFFFFFFL
#define YEL GRN+BLU
#define ORN GRN+RED
#define PUR RED+BLU

#define LEDS  60


int main() {
int i,j;

  Serial.begin(9600);
  bitSet(DDRB,3);       // output data bit to light strip
  RGBclear(LEDS);
  
  
cycle:
  
  Windup(10);
  Sparkle(1000);
  DriftFB(2000);
  WindOut(50);
  FlashAll(BLU,20,10);
  FlashAll(RED,20,10);
  FlashAll(GRN,20,10);
  Wait(2000);
  FlashAll(WHT,100,1);
  Wait(5000); 


  goto cycle;
}


void  Sparkle(int count) {
int i;

  for(i=0;i<count;i++) {
    DoLeds(Randex(60),Randex(60),Randex(60));
    Wait(10);
  }
  RGBclear(LEDS);
  Wait(1000);
}

void  DriftFB(int count) {
int  i,j,k;
char  red=1,grn=1,blu=5,rd,gd,bd;

  rd=bd=gd=1;
  for(i=0;i<count;i++) {
    red+=rd; if(red>60) red=1;
             if(red<0)  red=60;
             if(Random()<2) rd=-rd;
    grn+=gd; if(grn>60) grn=1;
             if(grn<0)  grn=60;
             if(Random()<2) gd=-gd;
    blu+=bd; if(blu>60) blu=1;
             if(blu<0)  blu=60;
             if(Random()<2) bd=-bd;
    DoLeds(red,grn,blu);
    Wait(10);
  }
  RGBclear(LEDS);
  Wait(1000);
}


void  RandomRGB(int times) {
int i,j;
byte  x;
  for(i=0;i<times;i++) {
    for(j=0;j<60;j++) {
      x=Random();
      if(x&1) RGBbyte(0xFF);
      else    RGBbyte(0);
      if(x&2) RGBbyte(0xFF);
      else    RGBbyte(0);
      if(x&4) RGBbyte(0xFF);
      else    RGBbyte(0);
    }
    Wait(50);
  } 
  RGBclear(LEDS);
  Wait(1000);
}

void  RandomRGB2(int times) {
int i,j;

  for(i=0;i<times;i++) {
    for(j=0;j<60;j++) {
      RGBbyte(Random());
      RGBbyte(Random());
      RGBbyte(Random());
    }
    Wait(50);
  } 
  RGBclear(LEDS);
  Wait(1000);
}


long  RandomColor() {
long  r;
    
    r=(long)Randex(20)|((long)Randex(200)<<8)|((long)Randex(20)<<16);
    return(r);
}


byte  Randex(byte size) {
byte  x;

more:
  x=Random();
  if(x<=size)
    return(x);
  goto more;
}

// Return a random number 0-255k
// Really fast, really good distribution

byte  Random() {
static byte  b1=1,b2,b3;

  b2+=b1;                   // add b1 to b2
  b3+=b2;                   // add b2 to b3
  b1+=(b3<<1)|(b3>>7);      // rotate bits in b3, add to b1
  return(b2);               // return (any) one byte
}

void  WindOut(int times) {
int i,j,k;
long  color;

  for(i=0;i<times;i++) {
    color=RandomColor();
    for(j=1;j<=LEDS;j++) {
      for(k=1;k<=j;k++) {
        RGBcolor(color);
      }
    }
  Wait(30);
  RGBclear(LEDS);
  Wait(60);
  }
  Wait(1000);
}

void  Windup(int start) {
int t;

  for(t=start;t>0;t--) {
    Rotate(RED,t+3);
    Rotate(GRN,t+3);
    Rotate(BLU,t+3);
  }
  RGBclear(LEDS);
  Wait(1000);
  
}

void  Rotate(long color, int timer) {
int i;

  for(i=1;i<=LEDS+1;i++) {
    SetLed(i,color);
    Wait(timer);
  }
  Wait(10);
  
}

void  FlashAll(long rgbval, int timer, int times)  {
int   i,j,k;
long  mask;

  for(k=0;k<times;k++) {
    for(j=0;j<LEDS;j++) {
      mask=0x800000L;
      for(i=0;i<24;i++) {     // now light the last one
        if(rgbval&mask) RGBbit(1);
        else            RGBbit(0);
        mask>>=1;
      } 
    }
    Wait(timer);
    RGBclear(LEDS);
    Wait(timer);
  }
  RGBclear(LEDS);
  Wait(300);
  
}

void  TestCycle() {
int i,j;
long color;

again:

  Wait(1000);
  for(j=0;j<4;j++) {
    switch(j) {
      case 0: color=0xff0000L; break;
      case 1: color=0x00ff00L; break;
      case 2: color=0x0000ffL; break;
      case 3: color=0xffffffL; break;
    }
    for(i=1;i<=61;i++) {
      SetLed(i,color);
      Wait(20);
    } Wait(17);
  }
  goto again;
  
}

void  DoLeds(byte red, byte grn, byte blu) {
int i;

  for(i=1;i<=LEDS;i++) {
    if(i==red) { RGBcolor(RED); continue; }
    if(i==grn) { RGBcolor(GRN); continue; }
    if(i==blu) { RGBcolor(BLU); continue; }
    RGBcolor(0);
  }
}

void  SetLed(int lednr,long rgbval) {
int i,j,k;
long  mask;

  for(i=0;i<(lednr-1);i++)
    for(j=0;j<24;j++)
      RGBbit(0);            // clear all but last leds
  RGBcolor(rgbval);
}

void  RGBclear(int n) {
int i,j;

  Wait(10);
  for(i=0;i<n;i++)
    for(j=0;j<24;j++)
      RGBbit(0);
}

void  RGBcolor(long color) {
long  mask;
byte  i;

  mask=0x800000L;
  for(i=0;i<24;i++) {     // now light the last one
    if(color&mask)  RGBbit(1);
    else            RGBbit(0);
    mask>>=1;
  }
}

void  RGBbyte(byte data) {
byte  j,mask;
    
    mask=0x80;
    for(j=0;j<8;j++) {     // now light the last one
      if(mask&data) RGBbit(1);
      else          RGBbit(0);
      mask>>=1;
    }
}
    
//  Generate 400ns(0) or 800ns(1) data pulse
//  coded for PORTB bit 3 on 16mhz PLL ATtiny85

void  RGBbit(byte bb) {

  if(bb)
    asm(
    "sbi  0x18,3 \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "cbi  0x18,3\n"
    );
  else 
    asm(
    "sbi  0x18,3 \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "nop \n"
    "cbi  0x18,3 \n"
    );
}



void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}
