.code

_str PROC
	str word ptr [rcx]
	ret
_str ENDP

_sldt PROC
	sldt word ptr [rcx]
	ret
_sldt ENDP

; RtlCaptureContext
CaptureRegisterContext PROC
	pushfq
	mov [rcx + 78h], rax
	mov [rcx + 80h], rcx
	mov [rcx + 88h], rdx
	mov [rcx + 0B8h], r8
	mov [rcx + 0C0h], r9
	mov [rcx + 0C8h], r10
	mov [rcx + 0D0h], r11

	movaps xmmword ptr [rcx + 1A0h], xmm0
	movaps xmmword ptr [rcx + 1B0h], xmm1
	movaps xmmword ptr [rcx + 1C0h], xmm2
	movaps xmmword ptr [rcx + 1D0h], xmm3
	movaps xmmword ptr [rcx + 1E0h], xmm4
	movaps xmmword ptr [rcx + 1F0h], xmm5

	; CcSaveNVContext
	mov word ptr [rcx + 38h], cs
	mov word ptr [rcx + 3Ah], ds
	mov word ptr [rcx + 3Ch], es
	mov word ptr [rcx + 42h], ss
	mov word ptr [rcx + 3Eh], fs
	mov word ptr [rcx + 40h], gs

	mov [rcx + 90h], rbx
	mov [rcx + 0A0h], rbp
	mov [rcx + 0A8h], rsi
	mov [rcx + 0B0h], rdi
	mov [rcx + 0D8h], r12
	mov [rcx + 0E0h], r13
	mov [rcx + 0E8h], r14
	mov [rcx + 0F0h], r15

	fnstcw word ptr [rcx + 100h]
	mov dword ptr [rcx + 102h], 0

	movaps xmmword ptr [rcx + 200h], xmm6
	movaps xmmword ptr [rcx + 210h], xmm7
	movaps xmmword ptr [rcx + 220h], xmm8
	movaps xmmword ptr [rcx + 230h], xmm9
	movaps xmmword ptr [rcx + 240h], xmm10
	movaps xmmword ptr [rcx + 250h], xmm11
	movaps xmmword ptr [rcx + 260h], xmm12
	movaps xmmword ptr [rcx + 270h], xmm13
	movaps xmmword ptr [rcx + 280h], xmm14
	movaps xmmword ptr [rcx + 290h], xmm15

	stmxcsr dword ptr [rcx + 118h]
	stmxcsr dword ptr [rcx + 34h]

	lea rax, [rsp + 10h]
	mov [rcx + 98h], rax
	mov rax, [rsp + 8h]
	mov [rcx + 0F8h], rax
	mov eax, [rsp]
	mov [rcx + 44h], eax
	mov dword ptr [rcx + 30h], 10000Fh
	add rsp, 8

	ret
CaptureRegisterContext ENDP

end