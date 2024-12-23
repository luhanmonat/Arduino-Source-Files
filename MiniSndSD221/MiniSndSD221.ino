

/*
 * Send and recieve bytes via SPI or I2C.
 * 3/2021 Luhan Monat
 * (Split off for interrupt driven inputs)
 * 
 * Vers Changes
 * ---- --------------------------------------
 * 2
 * 3    delay in WaitFor makes writes work
 * 4    works on recent ONN 32GB SDHC (Walmart)
 * 5    faster cleanup on writes
 * 6    also good on Samsung 32GB EVO (Best Buy)
 * 7    ACM41=0x69? failed on Sandisk 32GB (Walmart)
 * 8    
 * 9    changed to hardware SPI data lines
 * 10   works with full speed SPI (+25us post delay)
 * 16ok onn32/sam32/pny16   (43 KB/sec) ???
 * 17f  better on all 3
 * 18   also works with mic8
 * 19   also on yel16 (one of two worked on SPI)
 * 20   dropped pny16 for slow writes between blocks
 * 
 * 100  [Recoded for ATtiny1614] works ok on TW,TR,TD,TF
 * 101  Write gap 1.1 ms, use WaitFor in ReadSD
 * 102  Convert to hardware serial
 * 103  Serial input interrupt
 * 104  Fix dump bug, input buffer edit
 * 105  Init Menu ok, Dump sector or buffer
 * 106  Download works (?)
 * 107  Works at 57600, checksum on menus
 * 108  Add Delete entry, 115200 baud ok
 * 109  
 * 110  test,initmenu,loadsector,savesector,play ok
 *      fails on tambo4
 * 111  increase SBSIZE to 400, tambo4 ok
 *      stops working when powered back up - then starts
 *      working again.
 *      download kills init
 * 112  changed SaveSector to 512
 *      112noint - input without interrrupts
 * 113  Loaded 2 songs ok
 * 114  Loads TAMBO4 ok with 32 sandisk
 * 115  Cleaned up CloseSD, TAM4 still ok
 * 116  Comm() was eating retured data
 * 117  Created CommStat() for init function
 *      Loaded full music file
 * 118  Increased Trace=2 for first read bug
 * 218  Change back to interrupt version
 * 219  bad addressing code !!!!!
 * 220  
 * 221  check timing...take out delays r/w ok
 */

//  define port and bits used on nano
//  change these for other boards

/*  Commands    (upper or lower case)
 *  ------------------------------------------  
 *  xxyy zz     any two hex digits (spaces optional)
 *  .ddd        Read ddd (decimal) bytes as hex (end with space or end of line)
 *  /           Create START condition (select)
 *  ;           Create STOP (automatic at end of line)
 *  R           Repeat command string
 *  Wnnn        Wait nnn milliseconds
 *  *ddd        Repeat last command
 *  I           Init device
 */

#define VERS  221


enum {RDMODE,WRMODE};

#define STOP  3

//  SPI Mode
#define SCK  3     // SPI CLOCK
#define MOSI 1     // MASTER OUT
#define MISO 2      // MASTER IN
#define NSEL 4      // *SELECT
#define BUG  5

//  I2C Mode
#define SCL 2       // I2C clock
#define SDA 3       // I2C data

//  Port B
#define DET   0
#define CD    1
#define TXD   2
#define RXD   3


#define IBSIZE  60
#define SBSIZE  500

#define READSD  0x52      // read multi block
#define WRITESD 0x59      // write multi block

#define BAUD      9600
#define BITDELAY  1000000/BAUD
#define TXD     5
#define RXD     7

#define RET   '\r'
#define BS    '\b'
#define ESC   0x1b
#define DEL   0x7f
#define EOF   0x1A

byte  SBRpnt,SBXpnt,serbuff[SBSIZE];
int   reverse,space,index,col;
char  ErrorFlag,block;
char  inbuff[IBSIZE];
byte  last,endline,ix,dmp,exitflag;
int   findex,fmode;

byte  CMD0[6]   = {0x40,0,0,0,0,0x95};
byte  CMD1[6]   = {0x41,0,0,0,0,0xFF};
byte  CMD8[6]   = {0x48,0,0,1,0xAA,0x87};
byte  CMD12[6]  = {0x4C,0,0,0,0,0xFF};      // stop read
byte  CMD13[6]  = {0x4D,0,0,0,0,0xFF};      // stop write
byte  CMD17[6]  = {0x51,0,0,0,0,0xFF};      // single block read
byte  CMD18[6]  = {0x52,0,0,0,0,0xFF};      // multi block read
byte  CMD24[6]  = {0x58,0,0,0,0,0xFF};      // single block write (0xFE)
byte  CMD25[6]  = {0x59,0,0,0,0,0xFF};      // multi block write  (0xFC) -> (0xFD);
byte  CMD55[6]  = {0x77,0,0,0,0,0xFF};
byte  ACMD41[6] = {0x69,0x40,0,0,0,0xFF};     // *** was 0x41 !!!

word  nextAddr,nextDir,sectorCount;


struct entry {
  word  locus;
  word  leng;
  word  check;
  word  xx2;
  char  name[8];
}menu[40];            // 640 bytes

word  maxff;
byte  end;
byte  serdata,traceon;

int main() {
int i,j,dat,cmd,selflag;

reset:

  CCP=0xD8;
  CLKCTRL.MCLKCTRLB=0;          // No clock divider
  PORTA.DIR  = 0b11111011;    // bit 2 is MISO
  SERsetup(115200);
  SPIsetup();
  DTOAsetup();
  bitSet(PORTA.DIR,BUG);
  Printx("\nSD Card Verson %d",VERS);
  selflag=1;
  traceon=1;
  
rerun:
  Select(0);    // de-select  ErrorFlag=0;
  ErrorFlag=0;
  bitSet(PORTA.OUT,BUG);      // simulate CS for logic capture
  if(traceon)
    Printx("\nSDT%d>",VERS);
  else
    Printx("\nSD%d>",VERS); 
  col=4;
  FillBuff();

  next:
  exitflag=0;
  maxff=0;
  cmd=GetBuff();
  switch (cmd) {
    case ' ':
    case ',': break;
    case '/': Select(1); selflag=1; break;
    case ';': Select(0); Wait(1); break;
    case  0 :
    case '\r':
    case '\n': Printx(" ok",0); goto rerun;
    case '.': RecvHex(GetDecimal()); break;
    case 'W': Wait(GetDecimal()+1); break;
    case 'D': DumpHex(); break;
    case '*': Repeat(GetDecimal()); break;
    case 'M': DoList(); break;
    case 'L': LoadHex(); break;
    case 'I': InitSD(); break;
    case 'X': DeleteSD(); break;
    case 'T': test(); break;
    case 'A': traceon^=1; break;
    case 'P': Play(); break;
    case 'Y': InitMENU(); break;
    case 'Z': Download(); break;
    case 'C': dat=CMD(GetDecimal(),0); break;
    case  3:  goto reset;
    default:  SendHex(cmd);
              if(ErrorFlag) {
                HWSerOut(ErrorFlag);    // show bad character
                Printx("?",0);
                col+=2;
                goto rerun;                 // abort the rest
              }
  }
  goto next;

}

void  Play() {
byte  x;
word addr,leng,nr,check,gap,maxgap;
long  size,i;

  bitSet(PORTB.DIR,CD);
  bitClear(PORTB.OUT,CD);
  check=0;
  nr=GetDecimal()-1;
  LoadSector(0);
  addr=menu[nr].locus;
  size=(long)menu[nr].leng*512-512;
  Printx(" Playing %ld bytes...",size);
  OpenReadSD(addr);
  for(i=0;i<size;i++) {
    x=ReadSD();
    check+=x;
    DTOA(x);
    if(exitflag)
      break;
    delayMicroseconds(100);
  }
  CloseSD();
  Printx(" Check=%5u",check);
  Printx(" Maxgap=%5u",maxgap);
}

//  ********** Begin D/A  ***********

void  DTOAsetup() {
  bitSet(DAC0.CTRLA,6);       // Turn on d/a output
  bitSet(DAC0.CTRLA,0);       // enable d/a
  VREF.CTRLA=3;     
  TWI0.CTRLA=0b00010100;
}

void  DTOA(byte x) {
  DAC0.DATA=x;
}

word  Download() {
int   data;
byte  result,i,store;
word  locus,dir,start,check;
word  gap,sectors,maxgap;
long  timer,bcount;

  SeekEnd();
  check=0;
  locus=nextAddr;         // starting address
  dir=nextDir;
  data=GetBuff();
  if(data!=',') {
    CloseSD();
    Printx("\nError - need ,name",0);
    Wait(100);
    return;
  }
    
  for(i=0;i<7;i++) {
    data=GetBuff();
    if(data=='\r')
      break;
    menu[nextDir].name[i]=data;
  }
  menu[nextDir].name[i]=0;
  OpenWriteSD(menu[nextDir].locus);
  start=locus;    
  Printx("\nDownload at Sector %d ",locus);
  bcount=0;
  data=HWBuffIn();     // wait for 1st byte
  bitClear(PORTA.OUT,BUG);    // Simulate cs line
  HWSerOut('!');
  timer=0;
  goto ok;

nxt:
  data=GetSBuff();      // get buffer if ready
ok:
  if(data!=-1) {
    timer=0;
    WriteSD(data);
    check+=data;
    bcount++;
  }
  delayMicroseconds(10);
  timer++;
  if(timer<100000)
    goto nxt;
  Printx(" - Update Maxff = %u",maxff);
abort:
  CloseSD();
  bitSet(PORTA.OUT,BUG);
  Wait(100);
  Printx(" - Writing %ld bytes",bcount);
  menu[nextDir].leng=sectorCount;
  menu[nextDir].check=check;
  menu[nextDir+1].locus=menu[nextDir].locus+sectorCount;
  menu[nextDir+1].leng=0;
  SaveSector(0);
  DoList();
  return; 
}



void  DeleteSD() {
  if(GetBuff()!='*') {
    Printx(" - Error *",0);
    return;
  }
  DoList();
  menu[nextDir-1].leng=0;
  SaveSector(0);
  DoList();
}

void  SeekEnd() {
byte  j;
  LoadSector(0);
  for(j=0;j<32;j++) {
    nextAddr=menu[j].locus;
    if(menu[j].leng==0)
      break;
  }
  nextDir=j;
}

void  DoList() {
word  loc,addr,len;
byte  dat,i,j;

  LoadSector(0);
  Printx("\nNr Sector   Size Check Title",0);
  for(j=0;j<32;j++) {
    addr=menu[j].locus;
    len=menu[j].leng;
    Printx("\n%2u ",j+1);
    Printx(" %5u ",addr);
    Printx(" %5u",len);
    Printx(" %5u ",menu[j].check);
    if(len==0)
      break;
    for(i=0;i<8;i++) {
      dat=menu[j].name[i];
      if(isprint(dat))
        HWSerOut(dat);
      else
        HWSerOut(' ');
    }
  }
  CloseSD();
  
  Printx("%5u Sectors Free",65535-addr);
  Wait(100);
  nextAddr=addr;
  nextDir=j;
}

void  LoadHex() {
word  sector,addr;
byte  data,*ptr;

  sector=GetDecimal();
  LoadSector(sector);
  addr=GetHex(3);
  ptr=(byte *)menu;
  Printx("\n%04X: ",addr);
  FillBuff();
more:
  data=GetHex(2);
  ptr[addr++]=data;
    
}
void  LoadSector(int n) {
int   i;
byte  *p;

   p=(byte *)menu;
  OpenReadSD(n);
  for(i=0;i<512;i++)
    p[i]=ReadSD();
  CloseSD();
}

void  SaveSector(int n) {
int i,lines;
byte  *p;

  p=(byte *)menu;
  OpenWriteSD(n);
  for(i=0;i<514;i++)    // 514,520 worked, 500,512 fails
    WriteSD(p[i]);
  CloseSD();
}

void  InitMENU() {
  
  if(GetBuff()!='*') {
    Printx(" - Error *",0);
    return;
  }
  Clear((byte *)menu,512);
  menu[0].locus=1;
  menu[0].leng=5;
  strcpy(menu[0].name,"first");
  menu[1].locus=6;
  menu[1].leng=0;
  SaveSector(0);
}

int test() {
int  c,x,v,j,z,stat;
byte  wr,rd,n,fast,dmp,load,save,blk;
byte  *p;
word  sector,lines;

  p=(byte *)menu;
  dmp=wr=rd=fast=load=save=blk=0;
  sector=50000;

  more:
  switch(GetBuff()) {
    case 'D': dmp=1; break;
    case 'B': blk=1; break;
    case 'W': wr=1; lines=GetDecimal(); v=GetBuff(); break;
    case 'R': rd=1; break;
    case 'F': fast=1; break;
    case 'L': load=1; break;
    case 'S': save=1; break;
    default: goto oper;
  }
  goto more;

  oper:
  if(wr) {
    Printx("\nWrite %d lines ",lines);
    Printx("at sector %u",sector);
    OpenWriteSD(sector);
    for(x=0;x<lines;x++) {
      for(c='A';c<='Z';c++)
        WriteSD(c);
      WriteSD('@');
      WriteSD('@');
      WriteSD(v);
      WriteSD('@');
      WriteSD('@');
      for(c='A';c<='Z';c++)
        WriteSD(c);
      WriteSD('\n');
    }
    WriteSD('E');
    WriteSD('N');
    WriteSD('D');
    WriteSD(EOF);
    CloseSD();
  }
  
  Wait(10);

  if(load) {
    LoadSector(sector);
  }

  if(save) {
    SaveSector(sector);
  }
  
  if(rd|blk) {
    OpenReadSD(sector);
//    Wait(10);
    lines=0;
 if(rd)
  Printx("\nRead Data\n",0);
//    Wait(500);
    while(1) {
      c=ReadSD();
      if(c==EOF) break;
      if(HWSerFast()==ESC)
        break;
      if(c=='\n')
        if(rd)
          Printx(" %5u",lines++);
      if(rd)
        HWSerOut(c);
      if(exitflag) break;
     }
    CloseSD();
  }

 if(dmp) {
    lines=60;
    Printx("\nTotal=%d\n",lines);
    Printx("% 5d:",0);
    OpenReadSD(sector);
    Wait(10);
    z=0;
    for(x=0;x<lines;x++) {
      Printx("\n%5d",z);
      for(j=0;j<20;j++) { 
        c=ReadSD();
        z++;
        Printx(" %02X",c);
        if(exitflag) break;      
      }
      delayMicroseconds(800);
    }
    CloseSD();
  }

  if(fast) {
    
    OpenReadSD(sector);
    Wait(10);
    for(x=0;x<640;x++) {
      p[x]=SPI(0xff);
    }
    CloseSD();
    z=0;
    for(x=0;x<32;x++) {
      Printx("\n%5d:",z);
      for(j=0;j<20;j++) {
        if(j%4==0)
          Printx(" ",0);
        Printx(" %02X",p[z++]);
      }
    }
  }
}
  
 

void  Ready() {
int i,r;

  for(i=0;i<100;i++) {
    r=Comm(CMD13);
    HWSerOut('.');
    if(r==0)
      break;
  }
}

byte  Xdump(int n) {
int i,j,k,data;
byte  line[16];

    Select(1);
    CMD(17,n);
    Printx("\nSector %d",n);
    RecvHex(512);
}

void RecvHex(int loops) {
  int i,data;
  
  for(i=0;i<loops;i++) {
    if(i%16==0) {
      Printx("\n",0);
      col=0;
    }
    data=SwitchIn();
    Printx(" %02X",data);
  }
}


void  Clear(byte *p, int n) {
int i;

  for(i=0;i<n;i++) {
    *p++=0;
  }
}

void  DumpHex() {
int i,j,k,data,width;
word  n,lines;
byte  line[32],*p,c,ascii;

  ascii=1;
  width=16;
  lines=32;
more:
  c=GetBuff();
  switch(c) {
    case 'S': n=GetDecimal();
              Printx(" - Loading Sector %u",n);
              LoadSector(n);
              Wait(500);
              break;
    case 'W': ascii=0; width=24; lines=22; break;
    default: goto ok;
  }
  goto more;

ok:
  Printx("\nDump Buffer...",0);
  p=(byte *)menu;
  for(i=0;i<lines;i++) {
    Printx("\n%04X:",k);
    for(j=0;j<width;j++) {
      data=*p++;
      k++;
      line[j]=data;
      Printx(" %02X",data);
    }
    if(ascii) {
      Printx("  ",0);
      for(j=0;j<16;j++) {
        data=line[j];
        if(isprint(data))
          HWSerOut(data);
        else
          HWSerOut('.');
      }
    }
  }
  CloseSD();
}


byte  CMD(byte com, byte n) {
byte  nr,rslt,i;

  SPI(com+0x40);
  SPI(0);
  SPI(0);
  SPI(0);
  SPI(n);
  SPI(0xFF);
  for(i=0;i<8;i++) {
    rslt=SPI(0xFF);
    Printx(" %02X",rslt);
    if(rslt!=0xFF)
      break;
  }
}


void  InitSD() {
byte  i,stat;

  last=0xff;
  Repeat(100);
  for(i=1;i<=8;i++) {
    stat=CommStat(CMD0);
    Printx("\nCMD0    %02X",stat); col=0;
    Select(0);
    if(stat==0x01) break;
    Wait(i*100);
    if(Escape()) return;
  }
 
  for(i=1;i<=8;i++) {
    stat=CommStat(CMD8);
    Printx("\nCMD8    %02X",stat); col=0;
    ReadSPI(6);
    Select(0);
    if(stat==0x00) break;
    if(stat==0x01) break;
    if(stat==0x05) break;
    Wait(i*100);
    if(Escape()) return;
    Wait(10);
  }
  
  for(i=1;i<=8;i++) {
    stat=CommStat(CMD55);
    Printx("\nCMD55   %02X",stat); col=0;
    stat=CommStat(ACMD41);
    Printx("\nACMD41  %02X",stat); col=0;
    Select(0);
    if(stat==0x00) return;
    Wait(i*100);
    if(Escape()) return;
    
  }
  
  for(i=0;i<8;i++) {
    stat=CommStat(CMD1);
    Printx("\nCMD1    %02X",stat); col=0;
    if(stat==0x00) {
      CloseSD();
      Select(0);
      return;
    }
    Wait(i*100);
    if(Escape()) return;
  }
  Select(0);
  Printx("\nInit Failed",0);
}

byte  OpenWriteSD(long loc) {

  ClockBytes(10);
  Select(1);
  SPI(0xFF);
  StartSPI(0x59,loc);
  Wait(10);
  WaitFor(0,200,1);
  SPI(0xFF);
  SPI(0xFF);
  SPI(0xFF);
  SPI(0xFF);
  SPI(0xFC);
  findex=0;
  sectorCount=1;
  fmode=WRMODE;
  return(0);
}

byte  OpenReadSD(long loc) {
  
  ClockBytes(10);
  Select(1);
  SPI(0xFF);
  StartSPI(0x52,loc);
  WaitFor(0xFE,700,2);     // was 1000
  findex=0;
  sectorCount=1;
  fmode=RDMODE;
  return(0);
}

word  ReadWordSD() {
word  x;
  x=ReadSD()<<8;
  x+=ReadSD();
  return(x);
}

byte  ReadSD() {
int   r;
  
readsd:
  r=SPI(0xFF);
  if(findex++ < 512)       // get exactly 512 bytes
    return(r);
  SPI(0xff);
  SPI(0xff);
  WaitFor(0xFE,40,3);
  findex=0;
  sectorCount++;
  goto readsd;
}



void  WriteWordSD(word data) {
  WriteSD(data&0xff);
  WriteSD(data<<8);
}


byte  WriteSD(byte c) {

writesd:
  if(findex++ < 512) {
    SPI(c);
    return(0);
  }
  SPI(0xFF);
  SPI(0xFF);
  ClockBytes(10);        // was 10,20
  WaitFor(0xFF,1000,5);
  SPI(0xFC);
  findex=0;
  sectorCount++;
  goto writesd;
}

// Close read operations

byte  CloseSD() {

  if(fmode==RDMODE) {
 //   while(findex++ <512)
 //     SPI(0xff);
 //   SPI(0xFF);
 //   SPI(0xFF);
//    ClockBytes(10);
//    WaitFor(0xFF,200,6);
    Comm(CMD12);
    WaitFor(0x00,100,9);
    WaitFor(0xff,100,10);

  } else {
    while(findex++ <512)
      SPI(0x55);
    SPI(0xFF);
    SPI(0xFF);
    ClockBytes(10);      //221 was 10
    WaitFor(0xFF,200,6);
    SPI(0xFD);
    WaitFor(0x00,200,7);
    WaitFor(0xFF,500,8);
  }
  Select(0);
// ClockBytes(10);
}


void  StartSPI(byte op,long loc) {
  SPI(op);
  SPI((loc>>24)&0xff);
  SPI((loc>>16)&0xff);
  SPI((loc>>8)&0xff);
  SPI(loc&0xff);
  SPI(0xFF);
}

void  ClockBytes(int n) {
int i;
  
  for(i=0;i<n;i++) {
    SPI(0xFF);
  }
}

// Wait for a specified returned value

int WaitFor(byte val, int max, byte trace) {
int   i;
byte  r;

  for(i=0;i<max;i++) {
    r=SPI(0xFF);
    if(r==val)
      goto done;
    if(i>maxff)
      maxff=i;
  }
  if(traceon) {
    Printx("{{{ Trace = %d }}}",trace);
    Wait(4000);
    while(!exitflag)
      ;
  }
  return(-1);
  
  done:
  return(0);
}

byte  CommStat(byte dat[]) {
byte  result,i;

  Comm(dat);
  for(i=0;i<8;i++) {
    result=SPI(0xFF);
    if(!bitRead(result,7))
      break;
  }
  SPI(0xFF);
  return(result);
}

// removed looking for response; was eating returned data

byte  Comm(byte dats[]) {
byte  i;

  Select(1);
  for(i=0;i<6;i++)
    SPI(dats[i]);       // send the string of bytes
  return(0);
}

void  Select(byte n) {
  if(n) bitClear(PORTA.OUT,NSEL);
  else  bitSet(PORTA.OUT,NSEL);
}

void  ReadSPI(byte n) {
byte  i,dat;

  Select(1);
  for(i=0;i<n;i++) {
    dat=SPI(0xFF);
    Printx(" %02X",dat);
  }
  Select(0);
  
}



// Convert monitor ascii to decimal value

word  GetDecimal() {
  word result;
  char digit,i;

  Skip(' ');
  result=0;
  for(i=0;i<5;i++) {
    digit=GetBuff();
    if((digit>='0')&&(digit<='9')) 
      result=result*10+(digit-'0');
    else {
      ReBuff();           // put char back in buffer
      return(result);
    }
  }
  return(result);
}

word  GetHex(byte n) {
  word result;
  char digit,i,hit;

  Skip(' ');
  result=hit=0;
  for(i=0;i<n;i++) {
    digit=GetBuff();
    if(isxdigit(digit)) {
      hit=1;
      result<<=4;
      if(digit<='9')       
        result+=digit-'0';
      else
        result+=digit-'A'+10;
    } else {
      if(hit)
        ReBuff();           // put char back in buffer
      return(result);
    }
  }
  return(result);
}

void  Skip(char c) {
char  mat;

more:
  mat=GetBuff();
  if(mat==c)
    goto more;
  ReBuff();
}

void  Repeat(int count) {
int i;

  if(count) count--;
  for(i=0;i<count;i++)
    SPI(last);
  
}

void SendHex(char cc) {
int dat,low;
  
  dat=Nibble(cc)<<4;
  low=toupper(GetBuff());
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



void SwitchOut(byte data) {
  SPI(data);
}



byte  SwitchIn() {
byte  data;

  data=SPI(0xFF);
  return(data);
}

// Fill buffer from command line
// mark end with zero

void  FillBuff() {
byte  c;

  ix=0;
  next:
  c=toupper(GetSerial());
  switch (c) {
 
    case RET:   inbuff[ix]=c; ix=0; return;
    case DEL:   inbuff[0]=0; ix=0; return;
    case BS:    if(ix) {
                  ix--;
                  HWSerOut(BS);
                  HWSerOut(' ');
                  HWSerOut(BS);
                }
                break;
    default:    inbuff[ix++]=c; HWSerOut(c);
  }
  goto next;

}

void  EchoBuff() {
int i;

    while(inbuff[i]!='\r')
    HWSerOut(inbuff[i++]);
}

void  ReBuff() {
  if(ix)
    ix--;
}

byte  GetBuff() {
char  c;
  c=inbuff[ix++];
  if(c=='\r')
    ix--;
  block=0;
  return(c);
}


int GetSerial() {
char c;

  c=HWBuffIn();
  return(c);
}


// Send & Recieve SPI data via hardware
// 80k bytes/sec w/16 mhz clock
//  ********* Begin SPI ***********************

#define SPIWR   2
#define SPIRD   3
#define SPISS   4

void  SPIsetup() {
  bitSet(PORTA.DIR,3);              // /SS TO OUTPUT
  bitSet(PORTA.DIR,1);
  bitSet(PORTA.DIR,4);
  SPI0.CTRLA=0b00100101;        // master mode,div64,enable (312KHZ)
  SPI0.CTRLB=0;
}

byte  SPI(byte xdata) {
  SPI0.DATA=xdata;
  while(!bitRead(SPI0.INTFLAGS,7)) ;
  return(SPI0.DATA);
}


//  Check for keyboard 2-3 CR in a row

int Escape() {

  if(bitRead(USART0.STATUS,7))
    return(1);
  return(0);
}


//  ************* Begin Serial ************

void  SERsetup(long baud) {
word  factor;

  factor=80000000L/baud;
  USART0.BAUDL=factor&0xff;            // 9600 baud (208C)
  USART0.BAUDH=factor>>8;
  USART0.CTRLC=0b00001011;
  bitSet(PORTB.DIR,2);
  USART0.CTRLB=0b11000000;
  bitSet(USART0.CTRLA,7);       // enable RX interrupt
  SBXpnt=SBRpnt=0;
}


ISR (USART0_RXC_vect) {

  serdata=USART0.RXDATAL;
  PutSBuff(serdata);
  exitflag=1;

}

void  PutSBuff(byte x) {

  serbuff[SBXpnt++]=x;
  if(SBXpnt>=SBSIZE)
    SBXpnt=0;
}

// Send out data if buffer not full

void  HWBuffOut(byte x) {

  PutSBuff(x);
  while(SBXpnt==SBRpnt)       // next one whould over write
    ;
}

// Return serial data when ready

byte  HWBuffIn() {
int   r;

wait:
  cli();
  r=GetSBuff();
  sei();
  if(r== -1)
    goto wait;
  return(r);
}


// return serial data or -1

int   GetSBuff() {
int data;

  if(SBRpnt!=SBXpnt) {
    data=serbuff[SBRpnt++];
    if(SBRpnt>=SBSIZE)
      SBRpnt=0;
    return(data);
  }
  return(-1);
}

void  HWSerOut(byte data) {
  
  while(!bitRead(USART0.STATUS,5)) ;
  USART0.TXDATAL=data;
}

int   HWSerFast() {
  if(bitRead(USART0.STATUS,7))
    return(USART0.RXDATAL);
  else
    return(-1);
}

byte  HWSerIn() {

  while(!bitRead(USART0.STATUS,7)) ;
  return(USART0.RXDATAL);
}

void  Printx(char const fmt[], long dat) {
char  ary[100];
byte  i;

  sprintf(ary,fmt,dat);
  for(i=0;i<40;i++) {
    if(ary[i])
      HWSerOut(ary[i]);
    else
      break;
  }
}

// Replacement for delay() function

void  Wait(word timer) {
word i;

  for(i=0;i<timer;i++)
    delayMicroseconds(1000);
}
