.code

_str PROC
	str ax
	ret
_str ENDP

_sldt PROC
	sldt ax
	ret
_sldt ENDP

_cli PROC
	cli
	ret
_cli ENDP

_sti PROC
	sti
	ret
_sti ENDP

_invept PROC
	invept rcx, OWORD PTR [rdx]
	ret
_invept ENDP

HypervisorBreak PROC
	pushfq
	cli
x:
	jmp x
	popfq
	ret
HypervisorBreak ENDP

END