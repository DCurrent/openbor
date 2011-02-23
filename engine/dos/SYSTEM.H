// Functions to gain access to base memory, simulate real-mode interrupts
// and to gain write access to memory regions.

#ifndef SYSTEM_H
#define SYSTEM_H

#ifndef NULL
#define NULL 0
#endif

// Macros for address conversion. Real-mode address is a dword.
#define		PHYS_TO_REAL(phys)	((((int)phys&0xFFFF0)<<12)|(phys&0xF))
#define		REAL_TO_PHYS(real)	(((real&0xFFFF0000)>>12)+(real&0xFFFF))


// Flags
#define		CPU_CF		0x0001		// Carry
#define		CPU_PF		0x0004		// Parity
#define		CPU_AF		0x0010		// Aux. carry
#define		CPU_ZF		0x0040		// Zero
#define		CPU_SF		0x0080		// Sign
#define		CPU_TF		0x0100		// Trace
#define		CPU_IF		0x0200		// Int
#define		CPU_DF		0x0400		// Direction
#define		CPU_OF		0x0800		// Overflow





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

extern RMIstruct RMI;



// DPMI function to allocate DOS memory.
// Return value: linear pointer to allocated memory or NULL if failed.
void *getDOSmem(int *segment, int *selector, int size);

// Free DOS memory. Use the selector obtained through getDOSmem().
void freeDOSmem(int selector);


// Function to call before the interrupt simulation.
void prepareRMint();

// Simulate real-mode interrupt using the RMI structure.
unsigned int simRMint(int intno);


// Function to gain write access on high memory regions.
// Needed in some very specific cases.
// Returns a new pointer to the requested memory, or
// NULL on failure.
void *gainaccess(void *physaddr, unsigned int kilobytes);



#endif



