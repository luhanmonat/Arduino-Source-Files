/*
 * Dump All registers from 0x20 to 0xFF
 */

byte  *bp;

void setup() {

Serial.begin(9600); 
DDRB  = 0x55;
GPIOR2  = 0x55;
GPIOR1  = 0x44;
GPIOR0  = 0x22;
bp=5;
*bp = 0x77;

}

void loop() {
byte* p;
int i;
  
  for(i=0x00;i<=0xFF;i++) {
    if(i%16==0)
      Printx("\n%03X: ",i);
    if(i%4==0)
      Printx(" ",0);
    p=i;
    Printx("%02X ",*p);
  }
  halt: goto halt;
}

void  Printx(char *fmt, int dat) {
char  ary[40];

  sprintf(ary,fmt,dat);
  Serial.print(ary);
}
