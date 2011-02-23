#ifndef RAND32_H
#define RAND32_H

extern unsigned int __near rand32(void);
#pragma aux rand32 modify[eax edx] value[eax];

extern void __near srand32(int);
#pragma aux srand32 parm[eax];


#endif




