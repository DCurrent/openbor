/*
	A soundblaster interface.

	Last changes:
	06-25-2000	Delay + DMA buffer clearing after sound stop
*/

#include <conio.h>		// inp, outp
#include <stdio.h>		// inp, outp
#include <stdlib.h>		// getenv
#include <dos.h>		// _dos_getvect, _dos_setvect
#include "system.h"		// DPMI calls for access to base memory


#define		MAXDMABUFSIZE	0x10000

#define		MONO		0
#define		STEREO		1
#define		LOWQ		0
#define		HIGHQ		2

#define		SBDETECT	-1


// ======================== Globals ======================== //


static char *errptr = "No error or unknown";


static int SB_playstereo = 0;
static int SB_play16bits = 0;

static int dma_seg = 0, dma_sel = 0;
static unsigned char *dma_buf = NULL;
static int dma_bufsize = 0;

static int SB_port=-1, SB_dma=-1, SB_hdma=-1, SB_irq=-1, SB_int=-1;
static int SB_version=-1;


static void (*user_service)() = NULL;



//////////////////////////////// DMA memory ///////////////////////////////////


void clearDMAbuf(){
    int i;
    if(!dma_buf) return;
    for(i=0;i<dma_bufsize;i++) dma_buf[i] = 128;	// Clear for 8-bit
}


int allocDMAbuf(int size){
   int s, e;

   if(size>MAXDMABUFSIZE) return 0;

   if(dma_buf || dma_sel) return 0;
   if((dma_buf = getDOSmem(&dma_seg, &dma_sel, size<<1))==NULL) return 0;

   s = (int)dma_buf;
   e = s + size;

   s>>=16;
   e>>=16;

   if(s!=e) dma_buf = (void*)(e<<16);

   dma_bufsize = size;
   clearDMAbuf();

   return 1;
}



void freeDMAbuf(){
   if(!dma_sel) return;
   freeDOSmem(dma_sel);
   dma_sel = 0;
   dma_seg = 0;
   dma_buf = NULL;
}




/////////////////////////// DMA controllers /////////////////////////////////



#define		DMA_MODE_DEMAND		0x00
#define		DMA_MODE_SINGLE		0x40
#define		DMA_MODE_BLOCK		0x80
#define		DMA_MODE_CASCADE	0xC0
#define		DMA_MODE_REVERSE	0x20
#define		DMA_MODE_VERIFY		0x00
#define		DMA_MODE_WRITE		0x04
#define		DMA_MODE_READ		0x08
#define		DMA_MODE_CYCLE		0x10



void clearints(void);
#pragma aux clearints="cli";
void setints(void);
#pragma aux setints="sti";



int DMAstart8(int dma, void *buf, int count){

   int add, page;

   if(dma>3) return 0;

   add = (int)buf & 0xFFFF;
   page = (int)buf >> 16;
   if(page>15) return 0;

   --count;

   clearints();
   outp(0x0A,4|dma);	// Mask (disable and select) DMA channel
   outp(0x0C,0);	// Reset DMA byte ptr FF (reset the selected channel)
   outp(0x0B, dma | DMA_MODE_SINGLE|DMA_MODE_READ|DMA_MODE_CYCLE);

   outp(dma<<1, add&0xFF);	// Set the address reg, low byte
   outp(dma<<1, add>>8);	// High byte

   if(dma==0) outp(0x87,page);	// Set the page reg
   if(dma==1) outp(0x83,page);
   if(dma==2) outp(0x81,page);
   if(dma==3) outp(0x82,page);

   outp((dma<<1)+1, count&0xFF);	// Set the counter, low byte
   outp((dma<<1)+1, count>>8);		// High byte

   outp(0x0A,dma);	// Unmask DMA channel
   setints();
   return 1;
}



void DMAstop8(int dma){
   if(dma>3) return;
   clearints();
   outp(0x0A,4|dma);	// Mask (disable and select) DMA channel
   outp(0x0C,0);	// Reset DMA byte ptr FF (reset the selected channel)
   setints();
}



// Returns number of bytes remaining
unsigned int getDMApos8(char dma){
    static int DMApos;

    //Read DMA pointer from DMA controller
    DMApos = inp((dma<<1)+1);
    DMApos = DMApos|(inp((dma<<1)+1)<<8);

    return DMApos;
}



int DMAstart16(int dma, void *buf, int count){

   int add, page;

   if(dma<4 || dma>7) return 0;

   page = (int)buf >> 16;
   if(page>15) return 0;

   add = (int)buf;
   add >>= 1;			// 16bit DMA: half adresses
   add &= 0xFFFF;

   count>>=1;			// 16bit DMA: half the size
   --count;

   clearints();
   outp(0xD4,4|dma-4);	// Mask (disable and select) DMA channel
   outp(0xD8,0);	// Reset DMA byte ptr FF (reset the selected channel)
   outp(0xD6,(dma-4) | DMA_MODE_SINGLE|DMA_MODE_READ|DMA_MODE_CYCLE);

   outp(0xC0+((dma-4)<<2), add&0xFF);	// Set the address reg, low byte
   outp(0xC0+((dma-4)<<2), add>>8);	// High byte

   if(dma==4) outp(0x8F,page);	// Set the page reg
   if(dma==5) outp(0x8B,page);
   if(dma==6) outp(0x89,page);
   if(dma==7) outp(0x8A,page);

   outp(0xC2+((dma-4)<<2), count&0xFF);	// Set the counter, low byte
   outp(0xC2+((dma-4)<<2), count>>8);		// High byte

   outp(0xD4,dma-4);	// Unmask DMA channel
   setints();
   return 1;
}



void DMAstop16(int dma){
   if(dma<4 || dma>7) return;
   clearints();
   outp(0xD4,4|dma-4);	// Mask (disable and select) DMA channel
   outp(0xD8,0);	// Reset DMA byte ptr FF (reset the selected channel)
   setints();
}



// Returns number of words remaining
unsigned int getDMApos16(char dma){
    static int DMApos;

    //Read DMA pointer from DMA controller
    DMApos = inp(0xC2+((dma-4)<<2));
    DMApos |= (inp(0xC2+((dma-4)<<2))<<8);

    return DMApos;
}




int DMAstart(int dma, void *buf, int count){
   if(dma<4) return DMAstart8(dma,buf,count);
   return DMAstart16(dma,buf,count);
}


void DMAstop(int dma){
   if(dma<4) DMAstop8(dma);
   else DMAstop16(dma);
}


// Returns the number of samples remaining
int getDMApos(char dma){
   if(dma<4) return getDMApos8(dma);
   return getDMApos16(dma);
}






///////////////////////// Soundblaster specific //////////////////////////////





int parseB_hex(char c, char *z){
   int i, num=0;
   if(c>90) c-=32;
   for(i=0;(i<128 && z[i]);i++){
      if( (z[i]>90?z[i]-32:z[i]) == c){
	 i++;
	 while(z[i]>='0' && z[i]<='9'){
	    num<<=4;
	    num+=z[i]-'0';
	    i++;
	 }
	 return num;
      }
   }
   return -1;
}



int parseB_dec(char c, char *z){
   int i, num=0;
   if(c>90) c-=32;
   for(i=0;(i<128 && z[i]);i++){
      if( (z[i]>90?z[i]-32:z[i]) == c){
	 i++;
	 while(z[i]>='0' && z[i]<='9'){
	    num*=10;
	    num+=z[i]-'0';
	    i++;
	 }
	 return num;
      }
   }
   return -1;
}




int getblastervars(int *b_baseport, int *b_dma, int *b_dma2, int *b_irq){
   char *blaster;
   blaster = getenv("BLASTER");
   if(blaster==NULL) return 0;
   if(((*b_baseport = parseB_hex('A', blaster))==-1) ||
       ((*b_dma = parseB_dec('D', blaster))==-1) ||
       ((*b_irq = parseB_dec('I', blaster))==-1)){
      return 0;
   }
   *b_dma2 = parseB_dec('H', blaster);
   return 1;
}





int writeDSP(char val){
   int timeout = 65000;
   do{
      if(!(inp(SB_port+0x0C)&0x80)){
	 outp(SB_port+0x0C,val);
	 return 1;
      }
   }while(--timeout);
   return 0;
}



int readDSP(){
   int timeout = 65000;
   do{
     if(inp(SB_port+0x0E)&0x80) return inp(SB_port+0x0A);
   }while(--timeout);
   return -1;
}



int resetDSP(){
   outp(SB_port+0x06,1);	// send 1 to reset port
   delay(5);			// actually 3 microsecs...
   outp(SB_port+0x06,0);
   delay(5);
   return (readDSP()==0xAA);
}



int getDSPversion(){
   int hi, lo;

   if(!writeDSP(0xE1)) return -1;	// E1 = version command
   hi = readDSP();
   lo = readDSP();
   if(hi==-1 || lo==-1) return -1;

   return ((hi<<8)|lo);
}



void setsamplerate(int rate){
   int tc;
   tc = 256 - (1000000/rate);
   writeDSP(0x40);			// Timeconstant command
   writeDSP(tc);			// Set timeconstant
}



void SB3stereo(){
   outp(SB_port+0x04, 0x0E);
   outp(SB_port+0x05, (inp(SB_port+0x05)|2));
}


void SB3mono(){
   outp(SB_port+0x04, 0x0E);
   outp(SB_port+0x05, (inp(SB_port+0x05)&0xFD));
}





/////////////////////////////// IRQ handler //////////////////////////////////





volatile unsigned int irq_count = 0;


void (__interrupt __far *oldSBhandler)() = NULL;

void __interrupt __far SBhandler(){

   if(user_service) user_service();

   if(SB_version<0x200){
      writeDSP(0x14);
      writeDSP(0xFF);
      writeDSP(0xFE);
   }

   if(SB_play16bits) inp(SB_port+0x0F);		// Relieve DSP (16-bit)
   else inp(SB_port+0x0E);			// Relieve DSP (8-bit)

   if(SB_irq>7) outp(0xA0, 0x20);
   else outp(0x20,0x20);			// Send EOI to int controller

   ++irq_count;
}




int installSBhandler(){

   char b,c;

   if(SB_irq<8) SB_int = SB_irq+0x08;
   else if(SB_irq<15) SB_int = SB_irq+0x70;
   else return 0;

   // Disable IRQ
   clearints();
   b = 1;
   if(SB_irq<8){
      b <<= SB_irq;
      c = inp(0x21)|b;
      outp(0x21,c);
   }
   else{
      b <<= (SB_irq-8);
      c = inp(0xA1)|b;
      outp(0xA1,c);
   }
   setints();

   oldSBhandler = _dos_getvect(SB_int);
   _dos_setvect(SB_int,SBhandler);

   // Enable IRQ
   clearints();
   b = 1;
   if(SB_irq<8){
      b <<= SB_irq;
      b = 0xFF-b;
      c = inp(0x21)&b;
      outp(0x21,c);
   }
   else{
      b <<= (SB_irq-8);
      b = 0xFF-b;
      c = inp(0xA1)&b;
      outp(0xA1,c);
   }
   setints();
   return 1;
}



void removeSBhandler(){
   char b,c;

   if(oldSBhandler==NULL) return;

   // Disable IRQ
   clearints();
   b = 1;
   if(SB_irq<8){
      b <<= SB_irq;
      c = inp(0x21)|b;
      outp(0x21,c);
   }
   else{
      b <<= (SB_irq-8);
      c = inp(0xA1)|b;
      outp(0xA1,c);
   }
   setints();
   _dos_setvect(SB_int,oldSBhandler);
}






//////////////////////// The completed interface ///////////////////////////




// Return pointer to the DMA buffer
void * SB_init(int port, int dma, int hdma, int irq, int buffersize){

   user_service = NULL;

   if(port==-1 || dma==-1 || hdma==-1 || irq==-1){
      if(!getblastervars(&SB_port, &SB_dma, &SB_hdma, &SB_irq)){
	 errptr="Unable to parse BLASTER environment";
	 return 0;
      }
   }
   if(port!=-1) SB_port = port;
   if(dma!=-1)  SB_dma  = dma;
   if(hdma!=-1) SB_hdma = hdma;
   if(irq!=-1)  SB_irq  = irq;

   if(!resetDSP()){
      errptr="Sound reset failed";
      return 0;
   }
   if(!(SB_version=getDSPversion())){
      errptr="Unable to detect soundblaster version";
      return 0;
   }
   if(!allocDMAbuf(buffersize)){
      errptr="Not enough base memory for sound buffer";
      return 0;
   }
   if(!installSBhandler()){
      errptr="Failed to install sound ISR";
      freeDMAbuf();
      return 0;
   }
   return dma_buf;
}



#define		BLOCKSIZE	255


int SB_playstart(int stereo, int bits, int samplerate){
   int blocksize;

   if(SB_port==-1) return 0;

   if(SB_version<0x300 && stereo){
      errptr="No stereo playback available on this sound system";
      return 0;
   }
   if(SB_version<0x400 && bits==16){
      errptr="No 16-bit playback available on this sound system";
      return 0;
   }
   SB_playstereo = stereo;
   SB_play16bits = (bits==16);

   if(bits==16){				// SB16+
      if(SB_hdma==-1){
	 errptr="Attempt to play 16bit sound with unknown DMA";
	 return 0;
      }
//      blocksize = (dma_bufsize>>3)-1;
      blocksize = BLOCKSIZE;
      DMAstart16(SB_hdma, dma_buf, dma_bufsize);   // Provide 16bit DMA access
      setsamplerate(samplerate);
      writeDSP(0xB6);			// 16bit,play,autoinit,FIFO on
      if(stereo) writeDSP(0x20);	// Stereo, unsigned
      else writeDSP(0x00);		// Mono, unsigned
      writeDSP(blocksize&0xFF);		// Low byte
      writeDSP(blocksize>>8);		// High byte
      return 1;
   }
//   blocksize = (dma_bufsize>>2)-1;
   blocksize = BLOCKSIZE;

   writeDSP(0xD1);				// Enable speaker

   DMAstart(SB_dma, dma_buf, dma_bufsize);	// Provide 8bit DMA access
   setsamplerate(samplerate);
   if(stereo){
      if(SB_version>=0x400){			// SB16+
	 writeDSP(0xC6);			// 8bit,play,autoinit,FIFO on
	 writeDSP(0x20);			// Stereo, unsigned
	 writeDSP(blocksize&0xFF);		// Low byte
	 writeDSP(blocksize>>8);		// High byte
	 return 1;
      }
      // SB version 3.XX stereo
      SB3stereo();
   }
   else if(SB_version>=0x300 && SB_version<0x400) SB3mono();

   writeDSP(0x48);			// Set block size
   writeDSP(blocksize&0xFF);		// Low byte
   writeDSP(blocksize>>8);		// High byte
   writeDSP(0x1C);			// Start 8bit autoinit mono playback
   return 1;
}



int SB_getpos(){
   if(SB_play16bits) return (dma_bufsize>>1) - getDMApos16(SB_hdma);
   else return dma_bufsize - getDMApos8(SB_dma);
}



void SB_playstop(){
   if(SB_play16bits) DMAstop(SB_hdma);
   else DMAstop(SB_dma);
   writeDSP(0xD3);		// Disable speaker
   writeDSP(0xD0);		// Halt DMA operation
   writeDSP(0xDA);		// Exit DMA operation
   resetDSP();
   clearDMAbuf();
   delay(30);			// To be on the safe side...?
   resetDSP();
   clearDMAbuf();
}



void SB_exit(){
   user_service = NULL;
   SB_playstop();
   removeSBhandler();
   freeDMAbuf();
   resetDSP();
   SB_port = -1;
}



// To install a user ISR
void SB_hook(void (*func)()){
   user_service = func;
}


// To request SB information
void getSBvars(int *a, int *i, int *d, int *h, int *type){
   if(a) *a=SB_port;
   if(i) *i=SB_irq;
   if(d) *d=SB_dma;
   if(h) *h=SB_hdma;
   if(type) *type=SB_version;
}





////////////////////////////// VOLUME CONTROLLERS ////////////////////////////



#define SB_MASTERVOL	0x22
#define SB_VOICEVOL	0x04
#define SB_CDVOL	0x28



char SB_getvolume(char dev){
   if(SB_port==-1) return 0;
   outp(SB_port+0x04, dev);
   return inp(SB_port+0x05)&0x0F;
}


void SB_setvolume(char dev, char volume){
   if(SB_port==-1) return;
   volume = (volume&0x0F) + (volume<<4);
   outp(SB_port+0x04, dev);
   outp(SB_port+0x05, volume);
}




