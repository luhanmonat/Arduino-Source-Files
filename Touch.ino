

void setup() {

DDRD  = 0xf8;
bitSet(PORTD,2);
Serial.begin(9600);
}

void loop() {
volatile byte  x;
/*
asm(
  "sbi  0x0b,3\n"
  "cbi  0x0b,3\n"
  "mov  0x1e,0x09\n"
);
*/

GPIOR0  = 123;
asm("ldi  0x1e,55\n");
Serial.println(GPIOR0);
delay(500);


}


void  Flash() {
  
  bitSet(PORTD,4);
  delay(200);
  bitClear(PORTD,4);
  delay(200);
}
