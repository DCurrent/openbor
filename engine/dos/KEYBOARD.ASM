; ASM keyboard code for WATCOM C
; Last update: 1-feb-2003

.386p

_DATA	SEGMENT WORD PUBLIC USE32 'DATA'
	ASSUME DS: _DATA

	public	_keytable

	EVEN
	_keytable	db	256 dup (0)
	kb_callbios	dd	0
	kb_usextd	dd	0
	kb_breakflag	dd	0
	kb_lastkey	dd	0
	kb_lastkeyext	dd	0

	extendedkeynext	dd	0
	pbreaknext	dd	0

	oldkbhandler	df	0

	kb_inited	db	0

_DATA	ENDS




_TEXT	SEGMENT WORD PUBLIC USE32 'CODE'
	ASSUME CS: _TEXT


public keyboard_init_
public keyboard_exit_
public keyboard_enable_bios_
public keyboard_enable_extended_
public keyboard_getlastkey_
public keyboard_checkbreak_
public keyboard_setleds_
public keyboard_restoreleds_


; Keep a backup of the data segment in the code segment
EVEN
dataseg		dw 0


EVEN
keyboardhandler proc
	push	ds
	mov	ds, cs:dataseg
	push	eax
	push	edx
	xor	eax, eax
	in	al, 060h

	mov	edx, offset _keytable

	mov	ds:kb_lastkeyext, 0	; Used by getlastkey

	cmp	ds:extendedkeynext, 0	; Will this be an extended key code?
	je	dontusextd

					; Handle extended keypresses
	mov	ds:extendedkeynext, 0	; Only once, ofcourse
	cmp	al, 02Ah		; Ignore extended LSHIFT down code
	je	endint
	cmp	al, 0AAh		; Ignore extended LSHIFT up code
	je	endint

	cmp	al, 046h		; Break key?
	jne	notbreak
	mov	ds:kb_breakflag, eax	; Set break flag
	jmp	endint			; Return

notbreak:
	cmp	ds:kb_usextd,	0	; Use extended keyboard codes?
	je	dontusextd
	add	edx, 128		; Use keytable indices >= 128
	mov	ds:kb_lastkeyext, 128	; Used by getlastkey
dontusextd:

	cmp	ds:pbreaknext, 0	; About to receive break code?
	je	notpbreak

	dec	ds:pbreaknext		; Make sure this won't get a deadlock
					; (could happen under windoze)
	cmp	al, 0C5h		; Final break code?
	jne	endint
	mov	ds:kb_breakflag, eax	; Yep, set break flag
	mov	ds:pbreaknext, 0	; The next key is not 'break' anymore
	jmp	endint
notpbreak:

					; Check some stuff for next time
	cmp	al, 0E0h		; Will it be an extended key?
	jne	notxtdnext
	mov	ds:extendedkeynext, 1	; Yes, flag & return
	jmp	endint
notxtdnext:
	cmp	al, 0E1h		; Was the break key pressed?
	jne	notbreaknext
	mov	ds:pbreaknext, 5	; Yes, start counter & return
	jmp	endint
notbreaknext:

					; Handle normal key stuff
	add	edx, eax
	cmp	al, 128			; 128 or higher is a key up event
	jae	keyupevent

	mov	byte ptr ds:edx, al	; Key down, update table
	mov	ds:kb_lastkey, eax	; Update lastkey var
	jmp	endint			; Return
keyupevent:
	sub	edx, 128
	mov	byte ptr ds:edx, 0	; Key up, update table

endint:
	cmp	ds:kb_callbios, 0
	je	ret_nobios
	pop	edx
	pop	eax
	pop	ds
	jmp	cs:oldkbhandler		; Exit through BIOS

ret_nobios:
	mov	al, 020h
	out	020h, al		; Confirm hardware int
	pop	edx
	pop	eax
	pop	ds
	iretd				; Exit without BIOS
keyboardhandler endp



; ===========================================================
; Proc:		keyboard_init
; Destroys:	EAX, EDX
; Description:	Replaces the BIOS keyboard handler with the
;		custom handler.
; ===========================================================

keyboard_init_ proc
	mov	al, kb_inited			; Already initialized?
	or	al, al
	jnz	kbi_end

	mov	dataseg, ds			; Save data segment for ISR

	push	es
	mov	ax, 3509h			; Get the INT 9 vector...
	int	21h				; using INT 21h
	mov	dword ptr oldkbhandler+0, ebx
	mov	word ptr oldkbhandler+4, es
	pop	es

	push	ds
	mov	ax, 2509h			; Set the new INT 9 vector
	push	cs
	pop	ds
	mov	edx, offset keyboardhandler	; DS:EDX -> new vector
	int	21h

	pop	ds
	mov	kb_inited, 1
kbi_end:
	ret
keyboard_init_ endp



; ===========================================================
; Proc:		keyboard_exit
; Destroys:	EAX, EDX
; Description:	Restores the original keyboard handler state.
; ===========================================================

keyboard_exit_ proc
	push	ds

	mov	al, kb_inited		; If not initialized, return
	or	al, al
	jz	kbe_end

	lds	edx, oldkbhandler
	or	edx, edx		; Check if not NULL!
	jz	kbe_end			; If NULL, return

	mov	ax, 2509h		; Use INT 21h to restore INT 9 vector
	int	21h
kbe_end:
	pop	ds
	mov	kb_inited, 0
	ret
keyboard_exit_ endp




; =========================================================
; Proc:		keyboard_enable_bios
; In:		EAX
; Description:	If EAX is TRUE, enables use of the BIOS
;		keyboard handler (in addition to the
;		custom handler), otherwise disables it.
; =========================================================
keyboard_enable_bios_ proc
	mov	kb_callbios, eax
	ret
keyboard_enable_bios_ endp



; =========================================================
; Proc:		keyboard_enable_extended
; In:		EAX
; Description:	If EAX is TRUE, enables use of extended
;		keycodes, else disables it.
; =========================================================
keyboard_enable_extended_ proc
	mov	kb_usextd, eax
	ret
keyboard_enable_extended_ endp


; =========================================================
; Proc:		keyboard_getlastkey
; Returns:	EAX
; Description:	Returns the scan code of the key that
;		was last pressed, or FALSE if no key
;		was pressed since the previous call
; =========================================================
keyboard_getlastkey_ proc
	cli
	xor	eax, eax
	add	eax, kb_lastkey
	jz	no_orring
	or	eax, kb_lastkeyext
no_orring:
	mov	kb_lastkey, 0
	sti
	ret
keyboard_getlastkey_ endp


; ========================================================
; Proc:		keyboard_checkbreak
; Returns:	EAX
; Description:	Returns TRUE if the break key was pressed
;		since the previous call
; ========================================================
keyboard_checkbreak_ proc
	mov	eax, kb_breakflag
	mov	kb_breakflag, 0
	ret
keyboard_checkbreak_ endp



; ========================================================
; Proc:		keyboard_setleds
; In:		BL = desired led status (bit 0,1,2)
; Destroys:	AL, AH
; Description:	changes the status of the keyboard leds.
; ========================================================
keyboard_setleds_ proc
	cli

	xor	ah, ah			; Use timeout = 256
wait1:
	dec	ah
	jz	led_end
	in	al, 064h		; Read data ready indicator
	and	al, 2			; Bit must be clear before write
	jnz	wait1

	mov	al, 0EDh		; EDh = Set lights command
	out	060h, al		; Send command

	xor	ah, ah			; Timeout = 256
wait2:
	dec	ah
	jz	led_end
	in	al, 064h		; Read data ready indicator
	and	al, 2			; Bit must be clear before write
	jnz	wait2

	mov	al, bl			; Copy the led request
	and	al, 7			; Make sure unused bits are 0!
	out	060h, al		; Set leds
led_end:
	sti
	ret
keyboard_setleds_ endp




; ================================================================
; Proc:		keyboard_restoreleds
; Destroys:	EBX + all registers destroyed in keyboard_setleds
; Description:	Restores the keyboard leds to the status
;		expected by the BIOS keyboard handler.
;		This status is found at memory address
;		00417h	(0040:0017)
; ================================================================

keyboard_restoreleds_ proc
	mov	ebx,	00000417h	; Keyboard data address
	mov	bl,	[ebx]		; Read byte
	shr	bl,	4		; Move bits 654 to 210
	jmp	keyboard_setleds_	; Set the leds
keyboard_restoreleds_ endp



_TEXT	ENDS

end





