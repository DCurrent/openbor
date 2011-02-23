; Basic VGA code for mode 13h, mode X and a widescreen version of mode 13h.
; Last update: 21-jun-2002

.386p

	; Used by mode X and other tweaking code
	SC_INDEX	equ	003C4h
	CRTC_INDEX	equ	003D4h
	MISC_OUTPUT	equ	003C2h
	CRTC_OFFSET	equ	013h


_DATA	SEGMENT WORD PUBLIC USE32 'DATA'
	ASSUME DS: _DATA

	x_tweaktable	db	0E3h	; 60 Hz dot clock (for misc. out)
			db	10	; Number of CRTC Registers to update
			dw	00D06h	; vertical total
			dw	03E07h	; overflow (bit 8 of vertical counts)
			dw	04109h	; cell height (2 to double-scan)
			dw	0EA10h	; v sync start
			dw	0AC11h	; v sync end and protect cr0-cr7
			dw	0DF12h	; vertical displayed
			dw	00014h	; turn off dword mode
			dw	0E715h	; v blank start
			dw	00616h	; v blank end
			dw	0E317h	; turn on byte mode

	w_tweaktable	db	067h	; Misc. output (67 for max refs rate)
			db	8	; Number of CRTC regs
			dw	00A06h	; Vertical total (0A)
			dw	03E07h	; Overflow
			dw	04109h	; cell height (2 to double-scan)
			dw	0CA10h	; Vertical retrace start
			dw	08C11h	; Vertical retrace end (8C)
			dw	09712h	; Vertical display enable end
			dw	0DF15h	; Vertical blank start (DF)
			dw	00816h	; Vertical blank end (08)

COMMENT ^
			dw	00D06h	; Vertical total (0A)
			dw	03E07h	; Overflow
			dw	04109h	; cell height (2 to double-scan)
			dw	0CA10h	; Vertical retrace start
			dw	0AC11h	; Vertical retrace end (8C)
			dw	09712h	; Vertical display enable end
			dw	0E715h	; Vertical blank start (DF)
			dw	00616h	; Vertical blank end (08)
^

	t_tweaktable	db	20	; 20 regs
			dw	00011h	; unprotects registers 0x00 - 0x07 from write
			dw	05F00h	; Horizontal Total Register..same as MODE 13H
			dw	04701h	; Horizontal Display-Enable Register
			dw	04802h	; Start Horizontal Blanking Register
			dw	08203h	; End Horizontal Blanking Register
			dw	05004h	; Start Horizontal Retrace Pulse Register
			dw	08005h	; End Horizontal Retrace Register
			dw	0DE06h	; Vertical Total Register
			dw	01F07h	; Overflow Register
			dw	00008h	; Preset Row Scan Register
			dw	04109h	; Maximum Scan Line Register
			dw	0BC10h	; Vertical Retrace Start Register
			dw	00011h	; Vertical Retrace End Register
			dw	0AF12h	; Vertical Display-Enable Register
			dw	02413h	; Offset Register
			dw	04014h	; Underline Location Register
			dw	0B615h	; Start Vertical Blanking Register
			dw	0D916h	; End Vertical Blanking Register
			dw	0A317h	; CRT Mode Control Register
			dw	0FF18h	; Line Compare Register


_DATA	ENDS



_TEXT	SEGMENT WORD PUBLIC USE32 'CODE'
	ASSUME CS: _TEXT


public vga_setmode_
public vga_vwait_
public vga_setpalette_
public vga_setborderindex_
public vga_setmodex_
public vga_clearmodex_
public vga_xpageflip_
public vga_setwidemode_
public vga_setmodet_



;==================================================================
; Proc: 	vga_setmode
; In:		EAX = videomode
; Destroys:	EAX, ECX (depends on BIOS version)
; Description:	Sets VGA BIOS video mode.
;		Mode 03h = textmode
;		Mode 13h = 320x200x256
;==================================================================
vga_setmode_ proc near
	int	010h
	ret
vga_setmode_ endp




;==================================================================
; Proc: 	vga_vwait
; Destroys:	EAX, EDX
; Description:	Waits for new vertical retrace.
;		Interrupts are left ON, incase real-time interrupts
;		are active (soundcard, PIT...)
;==================================================================
vga_vwait_ proc near
	mov	dx, 003DAh
wait0:
	in	al, dx
	test	al, 008h
	jnz	wait0
wait1:
	in	al, dx
	test	al, 008h
	jz	wait1
	ret
vga_vwait_ endp




;====================================================================
; Proc: 	vga_setpalette
; In:		ESI -> palette
; Destroys:	EAX, ECX, EDX, ESI
; Description:	Sets VGA-type palette (256 colours, 8-bit RGB values)
;====================================================================
vga_setpalette_ proc near
	xor	al, al
	mov	edx, 003C8h
	out	dx, al
	inc	edx
;	out	dx, al
;	out	dx, al
;	out	dx, al
;	mov	ecx, 768-3
;	add	esi, 3
	mov	ecx, 768
sp_outloop:
	mov	al, [esi]
	shr	al, 2		; Convert 8-bit to 6-bit
	out	dx, al
	inc	esi
	loop	sp_outloop
	ret
vga_setpalette_ endp



;====================================================================
; Proc: 	vga_setborderindex
; In:		ECX = colour index
; Destroys:	AL, DX
; Description:	Sets VGA border colour index
;====================================================================
vga_setborderindex_ proc near
	mov	dx, 003DAh
	in	al, dx
	mov	al, 031h
	mov	dx, 003C0h
	out	dx, al
	mov	al, cl
	out	dx, al
	ret
vga_setborderindex_ endp






; ----------------------------- VGA mode X --------------------------------



;====================================================================
; Proc: 	vga_setmodex
; Destroys:	EAX, ECX, EDX, EDI (everything in vga_clearmodex)
; Description:	Sets mode X, the 320x240x256 planar VGA mode
;====================================================================
vga_setmodex_ proc near
	mov	eax, 013h
	int	010h		; First set BIOS mode 13h

	cli
	push	esi		; Now start tweaking the video registers
	mov	edx, SC_INDEX
	mov	eax, 0604h
	out	dx, ax
	mov	eax, 0100h
	out	dx, ax
	mov	esi, offset x_tweaktable
	lodsb
	mov	edx, MISC_OUTPUT
	out	dx, al
	mov	edx, SC_INDEX
	mov	eax, 0300h
	out	dx, ax
	mov	edx, CRTC_INDEX
	mov	al, 11h
	out	dx, al
	inc	dx
	in	al, dx
	and	al, 7Fh
	out	dx, al
	dec	edx
	xor	ecx, ecx
	lodsb
	mov	cl, al
CRTparmsloop:
	lodsw
	out	dx, ax
	loop	CRTparmsloop

	pop	esi
	sti

	jmp	vga_clearmodex_	; End by clearing VRAM
vga_setmodex_ endp




;====================================================================
; Proc: 	vga_clearmodex
; Destroys:	EAX, ECX, EDX, EDI
; Description:	Clears VGA RAM in mode x
;====================================================================
vga_clearmodex_ proc near
	mov	edi, 0A0000h
	mov	edx, SC_INDEX
	mov	eax, 0FF02h
	out	dx, ax
	xor	eax, eax
	mov	ecx, 16384
	rep	stosd
	ret
vga_clearmodex_ endp



;====================================================================
; Proc: 	vga_xpageflip
; In:		ECX = page (0,1 or 2)
; Destroys:	EAX, EBX, ECX, EDX
; Returns:	EBX -> active video page or NULL if page doesn't exist.
; Description:	Sets active video page in mode X
;====================================================================
vga_xpageflip_ proc near
	xor	ebx, ebx
	cmp	ecx, 2
	ja	xpf_end

	mov	ebx, 04B00h	; The size of one page
	dec	ecx
	shl	ebx, cl		; The correct offset of the page, I think

	mov	ah, bl
	mov	al, 0Dh
	mov	edx, 03D4h
	cli
	out	dx, ax
	mov	ah, bh
	dec	al
	out	dx, ax
	sti

	add	ebx, 0A0000h	; EBX -> active video page
xpf_end:
	ret
vga_xpageflip_ endp





; ---------------------- VGA 320x204 widescreen mode -------------------------



;====================================================================
; Proc: 	vga_setwidemode
; Destroys:	EAX, ECX, EDX
; Description:	Sets square-pixel, linear, widescreen, 320x204 VGA mode
;====================================================================
vga_setwidemode_ proc near

	mov	eax, 00013h
	int	010h			; First set BIOS mode 13h

	cli
	push	esi			; Now tweak the video registers

	mov	edx, SC_INDEX		; Prepare VGA for reset
	mov	eax, 0100h
	out	dx, ax

	mov	esi, offset w_tweaktable
	mov	edx, MISC_OUTPUT
	lodsb
	out	dx, al			; Set dot clock & such

	mov	edx, SC_INDEX		; Restart VGA
	mov	eax, 0300h
	out	dx, ax

	mov	edx, CRTC_INDEX		; Unprotect registers 0-7
;	mov	al, 11h
;	out	dx, al
	mov	ax, 0011h
	out	dx, ax


	xor	ecx, ecx
	lodsb
	mov	cl, al			; # registers
	mov	edx, CRTC_INDEX
w_CRTparmsloop:
	lodsw
	out	dx, ax
	loop	w_CRTparmsloop

	pop	esi
	sti

	ret
vga_setwidemode_ endp





; ------------------------- VGA mode T (288x216) ----------------------------


;====================================================================
; Proc: 	vga_setmodet
; Destroys:	EAX, ECX, EDX
; Description:	Sets mode T (288x216)
;====================================================================
vga_setmodet_ proc near
	mov	eax, 00013h
	int	010h			; First set BIOS mode 13h

	cli

	mov	dx, 03C4h		; Now reset the VGA
	mov	ax, 0100h
	out	dx, ax
	mov	dx, 03C2h
	mov	al, 0E3h
	out	dx, al
	mov	dx, 03C4h
	mov	ax, 0300h
	out	dx, ax


	xor	ecx, ecx		; Now set all the registers
	push	esi
	mov	esi, offset t_tweaktable

	lodsb
	mov	cl, al			; # registers
	mov	edx, CRTC_INDEX
t_CRTparmsloop:
	lodsw
	out	dx, ax
	loop	t_CRTparmsloop

	pop	esi
	sti
	ret
vga_setmodet_ endp






_TEXT	ENDS

end








