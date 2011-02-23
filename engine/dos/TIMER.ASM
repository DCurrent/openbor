; PIT Timer code
; Last update: 09-29-2000

.386p

	PIT_FREQ	equ	1193181


_DATA	SEGMENT WORD PUBLIC USE32 'DATA'
	ASSUME DS: _DATA

	oldtimer	df	0
	timer_inited	db	0

	EVEN
	ticker		dd	0
	lasttick	dd	0	; Used by gettime to guard accuracy
	lastinterval	dd	0	; Used by getinterval

_DATA	ENDS



_TEXT	SEGMENT WORD PUBLIC USE32 'CODE'
	ASSUME CS: _TEXT


public borTimerInit_
public borTimerExit_
public timer_gettime_
public timer_getinterval_


;----------------------------------------------------------------
; Proc:		timerhandler
; Description:	ISR (Interrupt Service Routine)
;----------------------------------------------------------------

EVEN

; Keep a backup of the data segment on the code segment
dataseg		dw 0

timerhandler proc
	push	ds
	mov	ds, cs:dataseg
	add	ds:ticker, 65536
	pop	ds
	jmp	cs:oldtimer
timerhandler endp



;----------------------------------------------------------------
; Proc:		borTimerInit
; Destroys:	EAX EBX EDX
; Description:	Installs the timer ISR
;----------------------------------------------------------------

borTimerInit_ proc
	mov	al, timer_inited
	or	al, al
	jnz	ti_end

	mov	dataseg, ds
	mov	ticker, 65536
	mov	lasttick, 0
	mov	lastinterval, 0

	push	es
	mov	ax, 3508h
	int	21h
	mov	dword ptr oldtimer+0, ebx
	mov	word ptr oldtimer+4, es
	pop	es

	push	ds
	mov	ax, 2508h
	push	cs
	pop	ds
	mov	edx, offset timerhandler
	int	21h
	pop	ds

	mov	al, 34h
	out	43h, al
	xor	al, al
	out	40h, al
	out	40h, al

	mov	timer_inited, 1
ti_end:
	ret
borTimerInit_ endp



;----------------------------------------------------------------
; Proc:		borTimerExit
; Destroys:	EAX EDX
; Description:	Restores the original timer ISR
;----------------------------------------------------------------

borTimerExit_ proc
	push	ds

	mov	al, timer_inited
	or	al, al
	jz	te_end		; Inited?

	mov	ax, 2508h
	lds	edx, oldtimer
	or	edx, edx	; Check if not NULL
	jz	te_end
	int	21h
te_end:
	pop	ds
	mov	timer_inited, 0
	ret
borTimerExit_ endp



;----------------------------------------------------------------
; Proc:		timer_gettime
; Destroys:	EBX
; Returns:	EAX = ticks
; Description:	Returns the time in ticks since the timer was
;		installed. Overflow occurs after approximately
;		40 minutes, I think...
;		Use only for short-interval timing!
;----------------------------------------------------------------

EVEN
timer_gettime_	proc	near
	cli
	xor	eax, eax
	out	43h, al
	in	al, 40h
	mov	ah, al
	in	al, 40h
	xchg	ah, al
	neg	eax
	add	eax, ticker	; EAX = new time
	sti

	mov	ebx, eax
	sub	ebx, lasttick	; EBX = newtick - lasttick
	jns	nointmissed	; if <0, we missed an interrupt for sure

	add	eax, 65536	; Let's pretend we did get it
EVEN
nointmissed:
	mov	lasttick, eax	; Save lasttick
	ret
timer_gettime_ endp




;----------------------------------------------------------------
; Proc:		timer_getinterval
; In:		ECX = frequency (1 to 1193181 Hz)
; Returns:	EAX = units passed since last call
; Destroys:	EBX ECX EDX
; Description:	Returns the time that passed since the last call,
;		measured in the specified frequency.
;		This function is extremely accurate, since all
;		rounding errors are compensatred for.
;		Only use for very short intervals!
;----------------------------------------------------------------

EVEN
timer_getinterval_	proc	near
	call	timer_gettime_

	mov	ebx, eax		; EBX = exact time in ticks
	sub	ebx, lastinterval	; EBX = time that passed in ticks

	mov	eax, PIT_FREQ
	xor	edx, edx		; EAX:EDX = PIT_FREQ
	div	ecx			; EAX = block size, EDX = mod

	add	ebx, edx		; Compensate for error in frequencies

	mov	ecx, eax		; ECX = block size
	mov	eax, ebx		; EAX = EBX = ticks passed

	xor	edx, edx		; EAX:EDX = ticks passed, ECX=blocksize
	div	ecx			; EAX = blocks passed, EDX=ticks left

	sub	ebx, edx		; Don't count unmeasured ticks
	add	lastinterval, ebx	; Advance counter for next call

	ret				; Return with EAX = units that passed
timer_getinterval_ endp



_TEXT	ENDS

end
