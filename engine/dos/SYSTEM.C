// Functions to gain access to base memory, simulate real-mode interrupts
// and to gain write access to memory regions.
// Last update: 09-10-2000


#include <i86.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#pragma pack (1)



// ------------------------ DPMI allocate base memory ------------------------



// DPMI call 100h allocates DOS memory
void * getDOSmem(int *segment, int *selector, int size){

   union REGS regs;
   unsigned long linearaddr;

   memset(&regs,0,sizeof(regs));
   regs.x.eax = 0x0100;
   regs.x.ebx = ((size+15)>>4);
   int386(0x31, &regs, &regs);

   if(regs.x.cflag) return 0;

   if(segment) *segment = regs.x.eax;
   if(selector) *selector = regs.x.edx;

   linearaddr = regs.x.eax & 0xFFFF;
   linearaddr <<= 4;

   return (void*) linearaddr;
}



// DPMI call 101h frees DOS memory
void freeDOSmem(int selector){

   union REGS regs;

   memset(&regs,0,sizeof(regs));
   regs.w.ax = 0x0101;
   regs.w.dx = selector;
   int386(0x31, &regs, &regs);
}




// ------------------- DPMI real-mode interrupt simulation -------------------


typedef struct{
	long	EDI;
	long	ESI;
	long	EBP;
	long	reserved;
	long	EBX;
	long	EDX;
	long	ECX;
	long	EAX;
	short	flags;
	short	ES,DS,FS,GS,IP,CS,SP,SS;
}RMIstruct;

RMIstruct RMI;



void prepareRMint(){
    memset(&RMI,0,sizeof(RMI));
}



unsigned int simRMint(int intno){
    union REGS regs;
    struct SREGS sregs;

    memset(&regs,0,sizeof(regs));
    memset(&sregs,0,sizeof(sregs));

    // Use DPMI call 300h to issue the DOS interrupt
    regs.w.ax = 0x0300;		// 03: sim real-mode int
    regs.h.bl = intno;		// The requested real-mode interrupt
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es = FP_SEG(&RMI);
    regs.x.edi = FP_OFF(&RMI);

    // use DPMI int 31 to simulate the real-mode interrupt
    int386x(0x31, &regs, &regs, &sregs);

    // Return the register most likely used as a return value
    return RMI.EAX;
}





// -------- Memory re-mapping (sometimes needed to gain write access) ---------



// Used internally
static long DPMI_mapmemory(long physaddr, long limit){
    union REGS regs;

    regs.w.ax = 0x800;
    regs.w.bx = physaddr >> 16;
    regs.w.cx = physaddr & 0xFFFF;
    regs.w.si = limit >> 16;
    regs.w.di = limit & 0xFFFF;
    int386(0x31,&regs,&regs);
    if(regs.x.cflag) return 0;
    return ((long)regs.w.bx<<16) + regs.w.cx;
}



void *gainaccess(void *physaddr, unsigned int kilobytes){
    long linaddr;
    linaddr = DPMI_mapmemory((long)physaddr,1024*kilobytes);
    return (void*) linaddr;
}


