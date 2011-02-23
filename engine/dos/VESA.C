/*
**		VBE (VESA BIOS Extensions) code.
**
**	Supports VBE 2.0+ linear video buffering.
**	Last update: 09-15-2000
**
*/

#include <stdio.h>
#include "system.h"			// DPMI system code


// Structures need to be byte-aligned
#pragma pack(1)


// VESA identifiers (strings)
#define		VBE2		0x32454256
#define		VESA		0x41534556

// Mode attribute flags
#define		MODE_SUPPORTED		1
#define		MODE_ISCOLOUR		8
#define		MODE_ISGRAPHIC		16
#define		MODE_ISLINEAR		128

#define		MODE_ENABLE_LINEAR	0x4000		// Use to set mode
#define		VBE_MAX_MODES		256




// Structure for general VESA information
typedef struct Vinfostruct{
	unsigned long	signature;	// "VBE2" on call, "VESA" on return
	unsigned short	version;	// We're hoping for 2.0+
	unsigned long	oemptr;
	unsigned long	capability_flags;
	unsigned short	modelistptr_offset;	// Points to FFFF-terminated...
	unsigned short	modelistptr_seg;	// ...list
	unsigned short	RAMblocks;		// # RAM blocks (1 block = 64K)
	unsigned short	oemsoftversion;
	unsigned long	vendorptr;
	unsigned long	productptr;
	unsigned long	revisionptr;
	char		reserved[222];
	char		oemscratch[256];
}Vinfostruct;


// Structure for mode-specific information
typedef struct Minfostruct{
	unsigned short	attributes;	// 1:suppd 8:color 16:gfx 128:linear sup
	char		windowA;	// 1:exists 2:readable 4:writable
	char		windowB;
	unsigned short	windowgrain;		// in KB
	unsigned short	windowsize;		// in KB (?)
	unsigned short	windowAstartseg;
	unsigned short	windowBstartseg;
	unsigned long	windowposfuncptr;	// (equivalent to AX=4F05h)
	unsigned short	bytesperscanline;
	unsigned short	width;
	unsigned short	height;
	char		charwidth;
	char		charheight;
	char		memplanes;
	char		bitspp;
	char		numbanks;
	char		memtype;
	char		banksize;		// in KB
	char		numpages;
	char		reservedbyte;
	// VBE 1.2+
	char		redmasksize, redfieldpos;
	char		greenmasksize, greenfieldpos;
	char		bluemasksize, bluefieldpos;
	char		reservedmasksize, reservedfieldpos;
	char		directcolor;
	// VBE 2.0+
	void * 		physaddress;		// Protected-mode address
	void *	 	offscreenaddress;
	short		offscreensize;		// in KB
	char		reserved[206];
}Minfostruct;


// MY VESA mode info structure, with only the important stuff in it.
#pragma pack (push,4)
typedef struct{
	int	modenum;		// The actual mode number
	int	width;
	int	height;
	int	bpp;
	int	linear;
	int	graphic;
	void * 	vram;			// Protected-mode address
}VESA_infostruct;
#pragma pack (pop)






static Vinfostruct *Vinfo = NULL;
static int Vseg=0, Vsel=0;
static Minfostruct *Minfo = NULL;
static int Mseg=0, Msel=0;

static unsigned short VESAmodelist[VBE_MAX_MODES+1];



// Internal: get general VBE information (using the Vinfo structure)
static unsigned int getVinfo(void){
    prepareRMint();
    RMI.EAX = 0x4F00;		// 4F00: get VESA VBE info (real-mode)
    RMI.ES  = Vseg;		// ES:DI -> allocated space for info structure
    RMI.EDI = 0;
    simRMint(0x10);
    return (RMI.EAX==0x4F);
}


// Internal: get info for specific mode (using the Minfo structure)
static unsigned int getMinfo(int mode){
    prepareRMint();
    RMI.EAX = 0x4F01;	// 4F01: get VESA VBE mode info (real-mode int)
    RMI.ES  = Mseg;	// ES:DI -> allocated space for info structure
    RMI.EDI = 0;
    RMI.ECX = mode;
    simRMint(0x10);
    return RMI.EAX;
}





void VESA_exit(){
    if(Vsel) freeDOSmem(Vsel);
    Vsel = 0;
    Vinfo = NULL;

    if(Msel) freeDOSmem(Msel);
    Msel = 0;
    Minfo = NULL;
}



// Returns VESA version on success or 0 on failure
int VESA_init(){
    int seg, offs;
    unsigned short *sptr;
    int i;

    if(Vsel) return 1;		// Already inited

    if((Vinfo = (Vinfostruct*)getDOSmem(&Vseg,&Vsel,sizeof(Vinfostruct)))==0) return 0;
    if((Minfo = (Minfostruct*)getDOSmem(&Mseg,&Msel,sizeof(Minfostruct)))==0){
	VESA_exit();
	return 0;
    }

    Vinfo->signature = VBE2;

    if(!getVinfo()){
	VESA_exit();
	return 0;
    }

    seg = Vinfo->modelistptr_seg;
    offs = Vinfo->modelistptr_offset;
    seg <<= 4;
    offs += seg;
    sptr = (unsigned short *)offs;

    // Copy the mode list, as it may be overwritten by the VESA driver later
    for(i=0;i<VBE_MAX_MODES;i++) VESAmodelist[i] = sptr[i];

    // Just as an extra precaution, append an end marker to the mode list
    VESAmodelist[VBE_MAX_MODES] = 0xFFFF;

    return Vinfo->version;
}



// Returns the size of the VESA video RAM in bytes
int VESA_getRAMsize(){
    if(!Vsel) return 0;
    return (Vinfo->RAMblocks*0x10000);
}



// Already provided by init, but what the hell
int VESA_getversion(){
    if(!Vsel) return 0;
    return Vinfo->version;
}




// Count and return the number of modes (max. 256)
int VESA_getnummodes(){
    int i;
    if(!Vsel) return 0;
    for(i=0; i<VBE_MAX_MODES; i++){
        if(VESAmodelist[i]==0xFFFF) break;	// End of mode list?
    }
    return i;
}




// Search for a mode with certain features.
// Returns VESA mode number if supported, or -1 if not supported.
int VESA_findmode(int xsize, int ysize, int bpp, int allowbanking){
    int i, mode = -1;

    if(!Vsel) return -1;		// Not inited?

    // First search for a linear mode
    for(i=0; i<VBE_MAX_MODES; i++){
        if(VESAmodelist[i]==0xFFFF) break;	// End of mode list?
        getMinfo(VESAmodelist[i]);		// Get mode info

        if(Minfo->width==xsize && Minfo->height==ysize && Minfo->bitspp==bpp &&
		 (Minfo->attributes & /*MODE_SUPPORTED+MODE_ISGRAPHIC+*/ MODE_ISLINEAR)){
	    return (VESAmodelist[i] | MODE_ENABLE_LINEAR);
        }
    }

    // If we couldn't find a mode, search for a banked mode (if allowed).
    for(i=0;(i<VBE_MAX_MODES && allowbanking);i++){
        if(VESAmodelist[i]==0xFFFF) break;	// End of mode list?
        getMinfo(VESAmodelist[i]);		// Request mode info
        if(Minfo->width==xsize && Minfo->height==ysize && Minfo->bitspp==bpp &&
		 (Minfo->attributes & /*MODE_SUPPORTED+*/ MODE_ISGRAPHIC)){
	    return VESAmodelist[i];
        }
    }

    return -1;
}



// Input: the actual mode number used by VBE (use VESA_findmode to find it)
// Returns a pointer to the VRAM on success or NULL on failure.
void * VESA_setmode(int mode){

    void * LVRAM = (void*)0xA0000;

    if(!Vsel) return NULL;
    if(mode==-1) return NULL;

    getMinfo(mode);
    if(!(Minfo->attributes & MODE_SUPPORTED)) return NULL;

    if(mode & MODE_ENABLE_LINEAR){
	LVRAM = gainaccess(Minfo->physaddress, Vinfo->RAMblocks*64);
	if(!LVRAM) return NULL;
    }

    prepareRMint();
    RMI.EAX = 0x4F02;                   // 4F02h: set VESA VBE mode
    RMI.EBX = mode;
    simRMint(0x10);

    return LVRAM;
}




// Provide mode information to the external world.
// If successful, fill variables and return 1.
// Otherwise return 0.
// Input: index for mode (0-255), pointer to my info structure
int VESA_getmodeinfo(unsigned int index, VESA_infostruct *info){

    if(!Vsel) return 0;			// Not inited?
    if(index>255) return 0;		// Bad index?
    if(!info) return 0;			// Bad pointer?

    getMinfo(VESAmodelist[index]);
    if(!(Minfo->attributes&MODE_SUPPORTED)) return 0;

    info->modenum = VESAmodelist[index];
    info->width = Minfo->width;
    info->height = Minfo->height;
    info->bpp = Minfo->bitspp;
    info->linear = (Minfo->attributes & MODE_ISLINEAR);
    info->graphic = (Minfo->attributes & MODE_ISGRAPHIC);
    info->vram = Minfo->physaddress;

    return 1;
}





void setVESAbank_asm(unsigned int bank);
#pragma aux setVESAbank_asm=\
	"mov eax, 4F05h"\
	"xor ebx,ebx"\
	"int 10h"\
	parm[edx] modify[eax ebx ecx edx];

void VESA_setbank(unsigned int bank){
    if(!Vsel) return;
    setVESAbank_asm(bank);
}




// This can be handy sometimes...
void VESA_listmodes(){
    int i, mode = -1;

    if(!Vsel) return;		// Not inited?

    // First search for a linear mode
    for(i=0; i<VBE_MAX_MODES; i++){
        if(VESAmodelist[i]==0xFFFF) break;	// End of mode list?
        getMinfo(VESAmodelist[i]);		// Get mode info

	printf(
		"VESA mode %Xh (%s)\n"
		"Dimensions %u x %u, %u bpp\n\n",
		VESAmodelist[i],
		((Minfo->attributes & MODE_ISLINEAR) ? "linear" : "banked"),
		Minfo->width,
		Minfo->height,
		Minfo->bitspp
	);
    }
}






