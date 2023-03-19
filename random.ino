
int dist[256];

void setup() {

 Serial.begin(9600);

}

void loop() {
byte  x;
word i,j;

  Serial.println("\nZero Out");
  for(i=0;i<256;i++)
    dist[i]=0;

  Serial.println("\nRandom Numbers:");

  for(i=0;i<50000;i++) {
    ++dist[RandomNR()];
  }
  x=0;
  for(i=0;i<16;i++) {
    Serial.println();
    for(j=0;j<16;j++) {
      Serial.print(dist[x++]);
      Serial.print(' ');
    }
  }
  
halt: goto halt;
}

byte RandomNR() {
static byte b1,b2,b3;

  b2+=b1;
  b3+=b2;
  b1+=Rotate(b3)+7;
  return(b2);
 
}

byte  Rotate(byte val) {

  return((val<<7)|(val>>1));
}

