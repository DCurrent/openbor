; Fast and reliable randomizer.
; Last update: 21-jun-2002

.386p

_DATA	SEGMENT WORD PUBLIC USE32 'DATA'
	ASSUME DS: _DATA

	EVEN
	seed	dd	1234567890

_DATA	ENDS


_TEXT	SEGMENT WORD PUBLIC USE32 'CODE'
	ASSUME CS: _TEXT


public rand32_
public srand32_



;==================================================================
; Proc: 	rand32
; Destroys:	EAX, EDX
; Returns:	EAX = pseudo-random number
; Description:	Returns a pseudo-random long integer.
;		I 'borrowed' this from the Inner Loops collection.
;==================================================================
EVEN
rand32_ proc near
	mov	eax, seed		; 1; get last random base 
	mov	edx, 1103515245		; 0; and a fixed multiplier
	mul	edx			; 9; do the unsigned multiply
	shl	edx, 16			; 1; prepare high 16 bits     
	add	eax, 12345		; 0; do a fixed addition
	adc	edx, 0FFFFh		; 1; carry to the high 16
	mov	seed, eax		; 0; record latest value
	shr	eax, 16			; 1; toss non-random low 16
	and	edx, 0FFFF0000h		; 0; mask the high 16
	or	eax, edx		; 1; transfer high 16 down
	ret				; 2; and return it, 32 bits
rand32_ endp



;==================================================================
; Proc: 	srand32
; In:		EAX = seed
; Destroys:	Nothing
; Description:	Seeds the randomizer
;==================================================================
EVEN
srand32_ proc near
	mov	seed, eax
	ret
srand32_ endp



_TEXT	ENDS

END



