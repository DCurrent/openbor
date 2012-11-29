; OpenBOR - http://www.LavaLit.com
; ----------------------------------------------------------------------
; All rights reserved, see LICENSE in OpenBOR root for details.
;
; Copyright (c) 2004 - 2012 OpenBOR Team

; Scales images by 2x using bilinear filtering.  This implementation uses the
; SIMD capabilities of MMX to operate on four source pixels at a time.  The
; GPL-encumbered bilinear filter implementation that was in OpenBOR before
; November 2012 also had an MMX implementation, but this code was written from
; scratch by Plombo and isn't related to the old MMX implementation.

%macro FUNCTION 1
%ifdef LINUX
GLOBAL %1
%1:
%else
GLOBAL _%1
_%1:
%endif
%endmacro

; void _BilinearMMX(u8* srcPtr, u32 srcPitch, u8* dstPtr, u32 dstPitch, int width, int height)
FUNCTION _BilinearMMX
push ebp
mov ebp, esp

mov ecx, [ebp+8]         ; srcPtr
mov edx, [ebp+16]        ; dstPtr
mov esi, [ebp+12]        ; srcPitch
mov edi, [ebp+20]        ; dstPitch
mov ebx, 0xf7def7de
movd mm7, ebx
punpckldq mm7, mm7       ; 0xf7def7def7def7de
shr dword [ebp+24], 2
mov ebx, [ebp+24]        ; width >> 2
mov eax, [ebp+28]        ; height

.loopstart:
movq mm0, [ecx]          ; mm0 := S0 (and D0)
movq mm1, [ecx+2]        ; mm1 := S1
movq mm2, [ecx+esi]      ; mm2 := S2
movq mm3, [ecx+esi+2]    ; mm3 := S3

movq mm4, mm0
pand mm4, mm7
psrlw mm4, 1
pand mm1, mm7
psrlw mm1, 1
paddw mm1, mm4           ; mm0 := D1
movq mm5, mm0            ; mm5 := D0
punpcklwd mm0, mm1
punpckhwd mm5, mm1
movq [edx], mm0
movq [edx+8], mm5

pand mm2, mm7
psrlw mm2, 1
paddw mm4, mm2            ; mm4 := D3
pand mm3, mm7
psrlw mm3, 1
paddw mm2, mm3
pand mm2, mm7
psrlw mm2, 1
pand mm1, mm7
psrlw mm1, 1
paddw mm1, mm2            ; mm1 := D4
movq mm0, mm4
punpcklwd mm0, mm1
punpckhwd mm4, mm1
movq [edx+edi], mm0
movq [edx+edi+8], mm4

; done with pixel, go to next pixel in row
add ecx, 8
add edx, 16
dec ebx
jnz .loopstart

; done with row, go to next row in source image
add [ebp+8], esi         ; srcPtr += srcPitch
add [ebp+16], edi        ; dstPtr += dstPitch
add [ebp+16], edi        ; dstPtr += dstPitch (because we write 2 lines of dst for every 1 line of src)
mov ecx, [ebp+8]
mov edx, [ebp+16]
mov ebx, [ebp+24]
dec eax
jnz .loopstart

; done with function
mov esp, ebp
pop ebp
emms
ret

