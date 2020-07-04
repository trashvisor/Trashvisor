.code

_str PROC
	str ax
	ret
_str ENDP

_sldt PROC
	sldt ax
	ret
_sldt ENDP

HypervisorBreak PROC
	pushfq
	cli
x:
	jmp x
	popfq
	ret
HypervisorBreak ENDP

END