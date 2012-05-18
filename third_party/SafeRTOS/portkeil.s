;******************************************************************************
;
; Indicate that the code in this file preserves 8-byte alignment of the stack.
;
;******************************************************************************
	PRESERVE8


;******************************************************************************
;
; Place code into the reset code section.
;
;******************************************************************************
        AREA    |.text|, CODE, READONLY
        THUMB


;******************************************************************************
;
; Set basepri to portSYSCALL_INTERRUPT_PRIORITY without effecting other
; registers.  r0 is clobbered.
;
;******************************************************************************
        EXPORT portSET_INTERRUPT_MASK
portSET_INTERRUPT_MASK
	mov r0, #191
	msr basepri, r0
	bx lr

	
;******************************************************************************
;
; Set basepri back to 0 without effective other registers.
; r0 is clobbered.
;
;******************************************************************************
        EXPORT portCLEAR_INTERRUPT_MASK
portCLEAR_INTERRUPT_MASK
		mov r0, #0
		msr basepri, r0
		bx lr


;******************************************************************************
;
; Make sure the end of this section is aligned.
;
;******************************************************************************
        ALIGN


;******************************************************************************
;
; Tell the assembler that we're done.
;
;******************************************************************************
        END
