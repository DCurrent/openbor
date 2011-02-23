; Functions to copy or clear screens.
; Useful for all sorts of screen copying.
; Last update: 10-20-2000


         .386p

_TEXT   SEGMENT WORD PUBLIC USE32 'CODE'
        ASSUME  CS:_TEXT


	PUBLIC	asm_copy_
        PUBLIC  asm_clear_
        PUBLIC  asm_planarvcopy_
;        PUBLIC  copyrect_



;=================================================================
; PROC:		asm_copy
; Parms:	ESI -> source
;		EDI -> destination
;		ECX = #bytes
; Destroys:	EAX EBX ECX EDX EDI
;
; Description:	Precached copying for use with large memory blocks.
;=================================================================


EVEN
asm_copy_	PROC	NEAR

	push	ebp
	push	esi

	mov	ebp, edi
	mov	ebx, esi

	mov	eax, ecx
	and	eax, 31		; Remaining bytes after 32-byte block copy
	push	eax

	shr	ecx, 5
	jz	sc_remainder	; No 32-byte blocks in there?

	dec	ecx		; Number of precached loops
	jz	sc_noprecache	; Not enough blocks to use precache?

EVEN
scloop:
	mov	al, [esi+32]	; Precache source

	mov	eax, [esi]
	mov	edx, [ebx+4]
	mov	[edi], eax
	mov	[ebp+4], edx
	mov	eax, [esi+8]
	mov	edx, [ebx+12]
	mov	[edi+8], eax
	mov	[ebp+12], edx
	mov	eax, [esi+16]
	mov	edx, [ebx+20]
	mov	[edi+16], eax
	mov	[ebp+20], edx
	mov	eax, [esi+24]
	mov	edx, [ebx+28]
	mov	[edi+24], eax
	mov	[ebp+28], edx
	add	esi, 32
	add	ebx, 32
	add	edi, 32
	add	ebp, 32

	dec	ecx
	jnz	scloop
sc_noprecache:
					; Now copy without precaching
	mov	eax, [esi]
	mov	edx, [ebx+4]
	mov	[edi], eax
	mov	[ebp+4], edx
	mov	eax, [esi+8]
	mov	edx, [ebx+12]
	mov	[edi+8], eax
	mov	[ebp+12], edx
	mov	eax, [esi+16]
	mov	edx, [ebx+20]
	mov	[edi+16], eax
	mov	[ebp+20], edx
	mov	eax, [esi+24]
	mov	edx, [ebx+28]
	mov	[edi+24], eax
	mov	[ebp+28], edx

	add	esi, 32			; Did I forget this earlier?
	add	ebx, 32
	add	edi, 32
	add	ebp, 32

sc_remainder:
	pop	ecx			; Now copy remaining bytes
	mov	eax, ecx
	shr	ecx, 2			; # dwords
	and	eax, 3

	rep	movsd
	mov	ecx, eax
	rep	movsb

sc_end:
	pop	esi
	pop	ebp	
	ret
asm_copy_	ENDP




;=======================================================================
; PROC:		asm_clear
; Parms:	EDI -> buffer
;		ECX = #bytes
; Destroys:	EAX, EBX, ECX, EDI
; Description:	Zero-fill buffer.
;=======================================================================

EVEN
asm_clear_	PROC	NEAR
	mov	ebx, ecx
	xor	eax, eax
	shr	ecx, 2
	and	ebx, 3
	rep	stosd
	mov	ecx, ebx
	rep	stosb
	ret
asm_clear_	ENDP





;=================================================================
; PROC:		asm_planarvcopy
; Parms:	ESI -> backbuffer
;		EDI -> Video RAM at desired video page
;		EAX = startline (0 - 239)
;		ECX = # lines to copy (0 - 240)
; Destroys:	EAX EBX ECX EDX EDI
; Description:	Linear-to-planar copying
;		Copies 16 lines per loop at all 4 planes.
;		This way, the data fits nicely into the cache
;		for the last 3 loops. Also, this method prevents
;		the ugly sawtooth effect.
;=================================================================


EVEN
asm_planarvcopy_	PROC	NEAR

	push	esi
	push	ebp

	or	cl, cl		; Copy anything at all?
	jz	pvc_end

	shl	eax, 4		; Do a fast mul by 80
	add	edi, eax
	shl	eax, 2
	add	edi, eax	; edi += line*80 (# bytes per line per plane)

	mov	edx, 003C4h	; VGA register for plane select
	mov	eax, 01102h	; plane select function & byteplane 0

EVEN
blockloop:
	mov	ch, 4		; 4 planes

	mov	ebp, 320	; # dwords in 16 lines on one plane
	cmp	cl, 16		; Check cl>=16
	jae	planeloop	; OK, go

	xor	ebx, ebx
	mov	bl, cl
	shl	bl, 2		; lines*4
	add	bl, cl		; lines*5
	shl	ebx, 2		; lines*20 (# dwords per line per plane)
	mov	ebp, ebx
EVEN
planeloop:
	out	dx, ax		; Activate byteplane
	rol	ah, 1		; Get ready for next plane

	push	ebp		; Save dword counter
	push	edi		; Save dest. pointer
	push	esi		; Save source pointer
EVEN
dwordloop:
	mov	bh, [esi+12]	; Gather bytes
	mov	bl, [esi+8]
	shl	ebx, 16
	mov	bh, [esi+4]
	mov	bl, [esi]
	mov	[edi], ebx	; Store dword
	add	esi, 16		; Advance pointers
	add	edi, 4
	dec	ebp		; Dec dword counter
	jnz	dwordloop

	pop	esi		; Restore source pointer
	pop	edi		; Restore dest. pointer
	pop	ebp		; Restore dword counter
	inc	esi		; Source ptr forward 1 byte
	dec	ch		; Dec plane counter
	jnz	planeloop

	add	esi, 5116	; Source ptr forward 16 lines - 1 dword
	add	edi, 320*4	; Forward again...
	sub	cl, 16		; Line counter
	ja	blockloop

pvc_end:
	pop	ebp
	pop	esi
	ret

asm_planarvcopy_	ENDP





COMMENT ^

;=======================================================================
; PROC:		copyrect_
; Parms:	ESI -> source
;		EDI -> dest
;		EAX = left
;		EBX = top
;		ECX = right
;		EDX = bottom
;		[ESP+4] = screenheight
; Destroys:	EAX, EBX, ECX, EDX, EDI
;
; Copy a rectangle from one screen to another. Supports clipping.
; Fast enough, but be warned: copies dwords, no single bytes.
;=======================================================================


EVEN
copyrect_	PROC	NEAR

	push	ebp
	push	esi

	mov	ebp, [esp+12]		; EBP = screenheight

	and	eax, 0FFFFFFFCh		; Round left DOWN to dword boundary
	add	ecx, 3			; Round right UP to dword boundary
	and	ecx, 0FFFFFFFCh		; Keep in mind, nothing will be copied
					; from this column...

	inc	edx			; Also copy the bottom line

	cmp	eax, 320		; Verify left side is visible
	jae	cr_end
	cmp	ecx, 0			; Verify right side invisible
	jle	cr_end
	cmp	ebx, ebp		; Verify top visible
	jae	cr_end
	cmp	edx, 0			; Verify bottom visible
	jle	cr_end

; Checked after clipping!
;	cmp	eax, ecx		; Verify left<right
;	jge	cr_end
;	cmp	ebx, edx		; Verify top<bottom
;	jge	cr_end

	cmp	eax, 0			; Clip left side?
	jl	cr_clipleft
cr_leftok:
	cmp	ecx, 320		; Clip right side?
	ja	cr_clipright
cr_rightok:
	sub	ecx, eax		; ECX = width
	jle	cr_end			; If width<=0, return

	cmp	ebx, 0			; Clip top?
	jl	cr_cliptop
cr_topok:
	cmp	edx, ebp		; Clip bottom?
	ja	cr_clipright
cr_bottomok:
	sub	edx, ebx		; EDX = height
	jle	cr_end			; If height<=0, return


	mov	ebp, ecx		; Save width
	add	esi, eax		; Calculate correct offsets
	add	edi, eax
	mov	eax, ebx
	shl	eax, 8
	shl	ebx, 6
	add	eax, ebx
	add	esi, eax
	add	edi, eax
EVEN
morelines:
	shr	ecx, 2
	rep	movsd
	add	esi, 320
	add	edi, 320
	sub	esi, ebp
	sub	edi, ebp
	mov	ecx, ebp
	dec	edx
	jnz	morelines
cr_end:
	pop	esi
	pop	ebp

	pop	eax		; Get return address
	pop	ebx		; Remove argument from stack
	push	eax		; Put return address back on stack
	ret			; ------ Return ---------

EVEN
cr_clipleft:
	xor	eax, eax
	jmp	cr_leftok

EVEN
cr_clipright:
	mov	ecx, 320	; Nothing will be copied from column 320
	jmp	cr_rightok

EVEN
cr_cliptop:
	xor	ebx, ebx
	jmp	cr_topok

EVEN
cr_clipbottom:
	mov	edx, ebp	; Nothing will be copied from this line
	jmp	cr_bottomok

copyrect_	ENDP

^








_TEXT	ENDS

END


