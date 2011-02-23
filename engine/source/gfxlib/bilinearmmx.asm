;/*
; * OpenBOR - http://www.LavaLit.com
; * -----------------------------------------------------------------------
; * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
; *
; * Copyright (c) 2004 - 2011 OpenBOR Team
; */

%include "macros.mac"

;%define FAR_POINTER

BITS 32

SECTION .text

NEWSYM _BilinearMMX
; Store some stuff
	 push ebp
	 mov ebp, esp

	 push ebx
         mov eax, [ebp+24] ;dx
         mov ebx, [ebp+28] ;dy
         push edx

         movq mm0, [eax]
         movq mm1, [ebx]

         psrlw mm0, 11  ;reduce to 5 bits
         psrlw mm1, 11

         movq [eax], mm0
         movq [ebx], mm1

         mov edx, [ebp+20]  ;D
         pmullw mm0, mm1
         movq mm5, [RedMask]
         movq mm6, [GreenMask]
         movq mm7, [BlueMask]
         psrlw mm0, 5

         pand mm5, [edx]
         pand mm6, [edx]

         psrlw mm5, 5
         pand mm7, [edx]

         pmullw mm5, mm0
         pmullw mm6, mm0
         pmullw mm7, mm0

         movq mm4, mm0       ;store x*y


         mov edx, [ebp+16] ;C
         movq mm0, [ebx]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]

         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+12] ;B
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         movq mm0, [eax]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]


         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+8] ;A
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3


         movq mm0, [All32s]
         movq mm1, mm4
         movq mm2, [eax]
         movq mm3, [ebx]
         paddw mm0, mm1
         paddw mm2, mm3
         psubw mm0, mm2
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+32]
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         psrlw mm6, 5
         psrlw mm7, 5

         pand mm5, [RedMask]
         pand mm6, [GreenMask]
         pand mm7, [BlueMask]

         por mm5, mm6
         por mm7, mm5
%ifdef FAR_POINTER
         movq [fs:edx], mm7
%else
         movq [edx], mm7
%endif
         pop edx
         pop ebx
	 mov esp, ebp
	 pop ebp
	 ret


NEWSYM _BilinearMMXGrid0
; Store some stuff
	 push ebp
	 mov ebp, esp

	 push ebx
         mov eax, [ebp+24] ;dx
         mov ebx, [ebp+28] ;dy
         push edx

         movq mm0, [eax]
         movq mm1, [ebx]

         psrlw mm0, 11  ;reduce to 5 bits
         psrlw mm1, 11

         movq [eax], mm0
         movq [ebx], mm1

         mov edx, [ebp+20]  ;D
         pmullw mm0, mm1
         movq mm5, [RedMask]
         movq mm6, [GreenMask]
         movq mm7, [BlueMask]
         psrlw mm0, 5

         pand mm5, [edx]
         pand mm6, [edx]

         psrlw mm5, 5
         pand mm7, [edx]

         pmullw mm5, mm0
         pmullw mm6, mm0
         pmullw mm7, mm0

         movq mm4, mm0       ;store x*y


         mov edx, [ebp+16] ;C
         movq mm0, [ebx]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]

         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+12] ;B
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         movq mm0, [eax]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]


         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+8] ;A
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3


         movq mm0, [All32s]
         movq mm1, mm4
         movq mm2, [eax]
         movq mm3, [ebx]
         paddw mm0, mm1
         paddw mm2, mm3
         psubw mm0, mm2
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+32]
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         psrlw mm6, 5
         psrlw mm7, 5

         pand mm5, [RedMask]
         pand mm6, [GreenMask]
         pand mm7, [BlueMask]

         por mm5, mm6
         pxor mm0, mm0
         movq mm6, mm7
         por mm7, mm5
         por mm6, mm5
         punpcklwd mm6, mm0
         punpckhwd mm7, mm0
%ifdef FAR_POINTER
         movq [fs:edx], mm6
         movq [fs:edx+8], mm7
%else
         movq [edx], mm6
         movq [edx+8], mm7
%endif
         pop edx
         pop ebx
	 mov esp, ebp
	 pop ebp
	 ret

NEWSYM _BilinearMMXGrid1
; Store some stuff
	 push ebp
	 mov ebp, esp

	 push ebx
         mov eax, [ebp+24] ;dx
         mov ebx, [ebp+28] ;dy
         push edx

         movq mm0, [eax]
         movq mm1, [ebx]

         psrlw mm0, 11  ;reduce to 5 bits
         psrlw mm1, 11

         movq [eax], mm0
         movq [ebx], mm1

         mov edx, [ebp+20]  ;D
         pmullw mm0, mm1
         movq mm5, [RedMask]
         movq mm6, [GreenMask]
         movq mm7, [BlueMask]
         psrlw mm0, 5

         pand mm5, [edx]
         pand mm6, [edx]

         psrlw mm5, 5
         pand mm7, [edx]

         pmullw mm5, mm0
         pmullw mm6, mm0
         pmullw mm7, mm0

         movq mm4, mm0       ;store x*y


         mov edx, [ebp+16] ;C
         movq mm0, [ebx]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]

         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+12] ;B
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         movq mm0, [eax]
         movq mm1, mm4
         psubw mm0, mm1
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]


         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+8] ;A
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3


         movq mm0, [All32s]
         movq mm1, mm4
         movq mm2, [eax]
         movq mm3, [ebx]
         paddw mm0, mm1
         paddw mm2, mm3
         psubw mm0, mm2
         movq mm1, [RedMask]
         movq mm2, [GreenMask]
         movq mm3, [BlueMask]
         pand mm1, [edx]
         pand mm2, [edx]
         psrlw mm1, 5
         pand mm3, [edx]

         pmullw mm1, mm0
         pmullw mm2, mm0
         pmullw mm3, mm0

         mov edx, [ebp+32]
         paddw mm5, mm1
         paddw mm6, mm2
         paddw mm7, mm3

         psrlw mm6, 5
         psrlw mm7, 5

         pand mm5, [RedMask]
         pand mm6, [GreenMask]
         pand mm7, [BlueMask]

         por mm5, mm6
         pxor mm0, mm0
         por mm7, mm5
         pxor mm1, mm1
         punpcklwd mm0, mm7
         punpckhwd mm1, mm7
%ifdef FAR_POINTER
         movq [fs:edx], mm0
         movq [fs:edx+8], mm1
%else
         movq [edx], mm0
         movq [edx+8], mm1
%endif
         pop edx
         pop ebx
	 mov esp, ebp
	 pop ebp
	 ret



NEWSYM _EndMMX
         emms
         ret

	SECTION .data ALIGN = 32
;Some constants
RedMask       dd 0xF800F800, 0xF800F800
BlueMask      dd 0x001F001F, 0x001F001F
GreenMask     dd 0x07E007E0, 0x07E007E0
All32s        dd 0x00200020, 0x00200020
