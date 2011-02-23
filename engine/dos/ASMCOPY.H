#ifndef ASMCOPY_H
#define ASMCOPY_H

// ASM data copy routines


// Syntax: asm_copy(dest, source, bytes)
extern void __near asm_copy(void*,void*,int);
#pragma aux asm_copy parm[edi][esi][ecx] modify[eax ebx ecx edx edi];

// Syntax: asm_clear(buf, bytes)
extern void __near asm_clear(void*,int);
#pragma aux asm_clear parm[edi][ecx] modify[eax ebx ecx edi];


// Syntax: asm_planarvcopy(vram,source,startline,lines)
extern void __near asm_planarvcopy(void*,void*,int,int);
#pragma aux asm_planarvcopy parm[edi][esi][eax][ecx] modify[eax ebx ecx edx edi];


/*
// Syntax: copyrect(source,dest,left,top,right,bottom,screenheight)
// Watcom accepts only 6 registers in a call, so the last one is
// passed through the stack. Argh!
extern void __near copyrect(void*,void*,int,int,int,int,int);
#pragma aux copyrect parm[esi][edi][eax][ebx][ecx][edx] modify[eax ebx ecx edx edi];
*/


#endif


