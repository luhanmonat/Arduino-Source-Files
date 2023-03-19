/*     
  Thermostat Sending Unit
  [ATtiny85 8Mhz internal clock]
  4/10/2022

  Read relay signal from thermostat
  Send on/off codes immediately if state changed
  Send current code every 60 seconds on no change

  Protocol: 5ms Attenstion, 2.5ms gap
  Each bit: 0-750usec, 1-1500usec, 750usec gap

  Updated to new protocol: 10/27/22
*/


#define TSWIT   4         // thermostat input
#define XMIT    3         // RF data output
#define SPEED   750
#define HEATON  0x1234
#define HEATOFF 0x5678

int	main() {
byte  heat,last;
word  timer;

DDRB    = 0b101111;       // bit 4 is input
timer = 1;

cycle:

  heat=bitRead(PINB,TSWIT);
  if(heat!=last) {
    last=heat;
    DoHeat(heat);
  } else {
    Wait(1000);             // one second delay
    if(--timer==0) {
      timer=60;             // timer set for 1 minute
      DoHeat(heat);
    }
  }
  goto cycle;

}

//  Send on or off codes 3 times

void  DoHeat(byte on) {
byte  i;

  for(i=0;i<3;i++) {
    if(on)
      HeatCode(HEATON);
    else
      HeatCode(HEATOFF);
    Wait(1000);
  }
}


//  Send 2 byte on/off codes

void  HeatCode(word hdata) {

  RFAttn();
  RFXmit(highByte(hdata));
  RFXmit(lowByte(hdata));
  Wait(200);
}

void  RFAttn() {
  bitSet(PORTB,XMIT);
  delayMicroseconds(5000);
  bitClear(PORTB,XMIT);
  delayMicroseconds(2500);
}

void  RFXmit(byte dat) {
byte  i,mask;
  
  mask=0x80;
  for(i=0;i<8;i++) {
    bitSet(PORTB,XMIT);
    if(dat&mask) delayMicroseconds(SPEED*2);
    else         delayMicroseconds(SPEED);
    bitClear(PORTB,XMIT);
    delayMicroseconds(SPEED);
    mask>>=1;
  }
  delayMicroseconds(2000);
}

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}
