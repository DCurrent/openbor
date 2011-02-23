// Gameport code. Automatically calibrates.
// Last update: 08-21-2000


#define		JOY_1		0x0010
#define		JOY_2		0x0020
#define		JOY_3		0x0040
#define		JOY_4		0x0080
#define		JOY_UP		0x0100
#define		JOY_DOWN	0x0200
#define		JOY_LEFT	0x0400
#define		JOY_RIGHT	0x0800
#define		JOY_5		0x1000
#define		JOY_6		0x2000




static unsigned int rawjoybuttons=0;
static unsigned int joyXaxis=0, joyYaxis=0;
static unsigned int joyLaxis=0, joyRaxis=0;

static unsigned int joyXmax=0, joyXmin=0;
static unsigned int joyYmax=0, joyYmin=0;
static unsigned int joyLmax=0, joyLmin=0;
static unsigned int joyRmax=0, joyRmin=0;

static int numjoybuttons = 0;




// Attempt to find BOTH joystick axes, times out after 65535 loops.
// Returns the estimated number of buttons (0,4,6).

int JOYdetect(void);
#pragma aux JOYdetect=\
	"cli"\
	"mov ecx, 0FFFFh"\
	"mov edx, 00201h"\
	"out dx, al"\
	"notfound4:"\
	"in al, dx"\
	"and al, 3"\
	"jz found4"\
	"loop notfound4"\
	"xor eax, eax"\
	"jmp end"\
	"found4:"\
	"mov ecx, 0FFFFh"\
	"out dx, al"\
	"notfound6:"\
	"in al, dx"\
	"and al, 12"\
	"jz found6"\
	"loop notfound6"\
	"mov eax, 4"\
	"jmp end"\
	"found6:"\
	"mov eax, 6"\
	"end:"\
	"sti"\
	modify[eax ecx edx];




// Get raw data from the joystick port. Takes a LOT of time!
// Updates the global variables rawjoybuttons, joyXaxis and joyYaxis.

int getJOYrawdata4(void);
#pragma aux getJOYrawdata4=\
	"xor eax, eax"\
	"xor ebx, ebx"\
	"xor ecx, ecx"\
	"cli"\
	"mov edx, 00201h"\
	"in al, dx"\
	"xor al, 0FFh"\
	"and al, 0F0h"\
	"mov rawjoybuttons, eax"\
	"out dx, al"\
	"time1:"\
	"in al, dx"\
	"test al, 1"\
	"jz time2"\
	"inc ebx"\
	"time2:"\
	"test al, 2"\
	"jz checkaxes"\
	"inc ecx"\
	"checkaxes:"\
	"cmp ebx, 60000"\
	"jae timeup"\
	"cmp ecx, 60000"\
	"jae timeup"\
	"and al, 3"\
	"jnz time1"\
	"timeup:"\
	"sti"\
	"mov joyXaxis, ebx"\
	"mov joyYaxis, ecx"\
	modify[eax ebx ecx edx];


int getJOYrawdata6(void);
#pragma aux getJOYrawdata6=\
	"push ebp"\
	"push edi"\
	"xor ebp, ebp"\
	"xor edi, edi"\
	"xor eax, eax"\
	"xor ebx, ebx"\
	"xor ecx, ecx"\
	"cli"\
	"mov edx, 00201h"\
	"in al, dx"\
	"xor al, 0FFh"\
	"and al, 0F0h"\
	"mov rawjoybuttons, eax"\
	"out dx, al"\
	"time1:"\
	"in al, dx"\
	"test al, 1"\
	"jz time2"\
	"inc ebx"\
	"time2:"\
	"test al, 2"\
	"jz timel"\
	"inc ecx"\
	"timel:"\
	"test al, 4"\
	"jz timer"\
	"inc ebp"\
	"timer:"\
	"test al, 8"\
	"jz checkaxes"\
	"inc edi"\
	"checkaxes:"\
	"cmp ebx, 60000"\
	"jae timeup"\
	"and al, 15"\
	"jnz time1"\
	"timeup:"\
	"sti"\
	"mov joyXaxis, ebx"\
	"mov joyYaxis, ecx"\
	"mov joyLaxis, ebp"\
	"mov joyRaxis, edi"\
	"pop edi"\
	"pop ebp"\
	modify[eax ebx ecx edx];



// PUBLIC: joy_init
// Initialize joystick/pad by autodetecting it and then guessing the
// minimum and maximum timing values.
// Return value: number of buttons detected.
int joy_init(){

	if((numjoybuttons=JOYdetect())==0) return 0;
	if(numjoybuttons==6){
		getJOYrawdata6();
		joyLmax = joyLaxis;
		joyLmin = joyLaxis>>1;
		joyRmax = joyRaxis;
		joyRmin = joyRaxis>>1;
	}
	else getJOYrawdata4();
	joyXmax = joyXaxis*2;
	joyXmin = joyXaxis>>4;
	joyYmax = joyYaxis*2;
	joyYmin = joyYaxis>>4;

	return numjoybuttons;
}


// PUBLIC: joy_exit
// Reset values to defaults.
void joy_exit(){
	numjoybuttons = 0;
	joyXmax = 0;
	joyXmin = 0;
	joyYmax = 0;
	joyYmin = 0;
	joyLmax = 0;
	joyLmin = 0;
	joyRmax = 0;
	joyRmin = 0;
}



// PUBLIC: joy_getstate
// Cook up some joystick data using the raw stuff.
// This is auto-calibrated stuff.
// Return value: number of buttons detected.
unsigned int joy_getstate(){

	unsigned int joyflags;

	if(numjoybuttons==6){
		getJOYrawdata6();
		joyflags = rawjoybuttons;
		if(joyLaxis<joyLmin) joyLmin = joyLaxis;
		else if(joyLaxis>joyLmax) joyLmax = joyLaxis;
		if(joyRaxis<joyRmin) joyRmin = joyRaxis;
		else if(joyRaxis>joyRmax) joyRmax = joyRaxis;
		if(joyLaxis<((joyLmax+joyLmin)>>1)) joyflags |= JOY_5;
		if(joyRaxis<((joyRmax+joyRmin)>>1)) joyflags |= JOY_6;
	}
	else if(numjoybuttons==4){
		getJOYrawdata4();
		joyflags = rawjoybuttons;
	}
	else return 0xFFFFFFFF;

	if(joyXaxis>=59999 || joyYaxis>=59999){
		numjoybuttons = 0;
		return 0xFFFFFF;
	}

	// Autocalibrate
	if(joyXaxis<joyXmin) joyXmin = joyXaxis;
	if(joyXaxis>joyXmax) joyXmax = joyXaxis;
	if(joyYaxis<joyYmin) joyYmin = joyYaxis;
	if(joyYaxis>joyYmax) joyYmax = joyYaxis;

	// Interpret timed values
	if(joyXaxis<((((joyXmin+joyXmax)>>1)+joyXmin)>>1)) joyflags |= JOY_LEFT;
	else if(joyXaxis>((((joyXmax+joyXmin)>>1)+joyXmax)>>1)) joyflags |= JOY_RIGHT;
	if(joyYaxis<((((joyYmin+joyYmax)>>1)+joyYmin)>>1)) joyflags |= JOY_UP;
	else if(joyYaxis>((((joyYmax+joyYmin)>>1)+joyYmax)>>1)) joyflags |= JOY_DOWN;

	return joyflags;
}






