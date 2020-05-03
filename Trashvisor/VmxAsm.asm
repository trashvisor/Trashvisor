.code

VmxInitialiseLogicalProcessorStub PROC

	mov rdx, SuccessTarget
	mov r8, rsp

	sub rsp, 20h
	call VmxInitialiseLogicalProcessor 
	add rsp, 20h

SuccessTarget:
	ret
VmxInitialiseLogicalProcessorStub ENDP

end