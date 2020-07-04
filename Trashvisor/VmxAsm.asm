SaveState MACRO
	push	r15
	push	r14
	push	r13
	push	r12
	push	r11
	push	r10
	push	r9
	push	r8
	push	rdi
	push	rsi
	push	rbp
	sub	rsp, 8	; placeholder
	push	rbx
	push	rdx
	push	rcx
	push	rax
ENDM

RestoreState MACRO
	pop	rax
	pop	rcx
	pop	rdx
	pop	rbx
	add	rsp, 8
	pop	rbp
	pop	rsi
	pop	rdi
	pop	r8
	pop	r9
	pop	r10
	pop	r11
	pop	r12
	pop	r13
	pop	r14
	pop	r15
ENDM

extern VmxInitialiseProcessor:proc
extern VmxVmExitHandler:proc
extern GetVmxInstructionError:proc

.code

VmxInitialiseProcessorStub PROC
	pushfq
	SaveState

	mov rdx, GuestEntry
	mov r8, rsp

	sub rsp, 20h
	call VmxInitialiseProcessor
	add rsp, 20h

	RestoreState
	popfq

	mov rax, 0
	ret

GuestEntry:
	;int 3

	RestoreState
	popfq

	mov rax, 1
	ret
VmxInitialiseProcessorStub ENDP

VmxVmExitHandlerStub PROC
	SaveState

	mov rcx, rsp

	sub rsp, 20h
	call VmxVmExitHandler
	add rsp, 20h

	RestoreState
	vmresume

	call GetVmxInstructionError

	ret
VmxVmExitHandlerStub ENDP

END