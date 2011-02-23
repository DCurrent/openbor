/*
**		VBE (VESA BIOS Extensions) code.
**
**	Supports VBE 2.0+ linear video buffering.
*/


// VESA mode bit indicating linear video RAM used
#define		VESA_LINEAR	0x4000


// MY VESA mode info structure, with only the important stuff in it.
#pragma pack (push,4)
typedef struct{
	int	modenum;		// The actual mode number
	int	width;
	int	height;
	int	bpp;
	int	linear;			// Linear mode supported?
	int	graphic;		// Is it a graphics mode?
	void * 	vram;			// Protected-mode address
}VESA_infostruct;
#pragma pack (pop)




int VESA_init();	// Returns VESA version on success or 0 on failure
void VESA_exit();	// Never forget this!
int VESA_getRAMsize();	// Returns the size of the VESA video RAM in bytes
int VESA_getversion();	// Already provided by init, but what the hell
int VESA_getnummodes();	// Count and return the number of modes (max. 256)

	// Search for a mode with certain features.
	// Returns VESA mode number if supported, or -1 if not supported.
int VESA_findmode(int xsize, int ysize, int bpp, int allowbanking);

	// Input: the actual mode number (use VESA_findmode to find it)
	// Returns a pointer to the VRAM on success or NULL on failure.
void * VESA_setmode(int mode);

	// Input: index for mode (0-255), pointer to my info structure.
	// Returns 1 on success, 0 on failure.
int VESA_getmodeinfo(unsigned int index, VESA_infostruct *info);

	// Set active bank in banked video modes
void VESA_setbank(unsigned int bank);

	// Print all available modes on system
void VESA_listmodes(void);


