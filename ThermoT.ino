/*     
  Thermostat Sending Unit - (test softare)
  [ATtiny85 8Mhz internal clock]
  4/10/2022

  Read relay signal from thermostat
  Send on/off codes immediately if state changed
  Send current code every 60 seconds on no change

  Protocol: 5ms Attention, 2.5ms gap
  Each bit: 0-750usec, 1-1500usec, 750usec gap
  
*/

#define PORT  PORTD
#define PIN   PIND
#define DDR   DDRD

#define XMIT    2         // RF data output
#define SPEED   750
#define HEATON  0x1234
#define HEATOFF 0x5678

int	main() {
byte  heat,last;
word  timer;

DDR    = 0b11111111;

cycle:

  HeatCode(HEATON);
  Wait(1000);
  HeatCode(HEATOFF);
  Wait(5000);
  goto cycle;

}




//  Send 2 byte on/off codes

void  HeatCode(word hdata) {

  RFAttn();
  RFXmit(highByte(hdata));
  RFXmit(lowByte(hdata));
  Wait(200);
}

void  RFAttn() {
  bitSet(PORT,XMIT);
  delayMicroseconds(5000);
  bitClear(PORT,XMIT);
  delayMicroseconds(2500);
}

void  RFXmit(byte dat) {
byte  i,mask;
  
  mask=0x80;
  for(i=0;i<8;i++) {
    bitSet(PORT,XMIT);
    if(dat&mask) delayMicroseconds(SPEED*2);
    else         delayMicroseconds(SPEED);
    bitClear(PORT,XMIT);
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
