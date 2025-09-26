section .text

global asm__detail_strcmp_cs
global asm__detail_strcmp_sse_cs
global asm__detail_strcmp_avx_cs

asm__detail_strcmp_cs:
; RDI = lhs, RSI = rhs, RDX = length
    xor rcx, rcx

_loop:
    movzx eax, byte ptr [rdi + rcx]
    movzx r10d, byte ptr [rsi + rcx]
    cmp al, r10b
    jne _fail
    inc rcx
    cmp rcx, rdx
    jne _loop

; _done
    mov rax, 1
    ret

_fail:
    xor rax, rax
    ret

; end asm__detail_strcmp_cs

asm__detail_strcmp_sse_cs:
; RDI = lhs, RSI = rhs, RDX = length

    mov r10, rdx
    and r10, -16 ; r10 = simd_loop_size
    xor r9, r9 ; r9d = i

    test r10, r10
    je _tail

_simd:
    prefetcht0 byte ptr [rdi + r9 + 96]
    prefetcht0 byte ptr [rsi + r9 + 96]

    movdqa xmm0, xmmword ptr [rdi + r9]
    movdqa xmm1, xmmword ptr [rsi + r9]
    pcmpeqb xmm0, xmm1 ; sse2
    pmovmskb eax, xmm0
    cmp eax, 0000FFFFh 
    jne _mismatch

    add r9, 16
    cmp r9, r10
    jb _simd

_tail:
    mov r11, r8
    sub r11, r10
    jz _equal

_tail_loop:
    mov al, byte ptr [rdi + r10]
    cmp al, byte ptr [rsi + r10]
    jne _mismatch
    inc r10
    dec r11
    jnz _tail_loop

_equal:
    vzeroupper
    mov eax, 1
    ret

_mismatch:
    vzeroupper
    xor eax, eax
    ret

; end asm__detail_strcmp_sse_cs

asm__detail_strcmp_avx_cs:
; RDI = lhs, RSI = rhs, RDX = length
    mov     r10, rdx
    and     r10, -32
    xor     r9d, r9d

    test    r10, r10
    je      _tail

_simd:
    prefetcht0 [rdi + r9 + 192]
    prefetcht0 [rsi + r9 + 192]

    vmovdqu  ymm0, [rdi + r9]
    vmovdqu  ymm1, [rsi + r9]
    vpcmpeqb ymm2, ymm0, ymm1
    vpmovmskb eax, ymm2
    cmp eax, -1
    jne _mismatch

    add r9, 32
    cmp r9, r10
    jb _simd

_tail:
    mov r11, rdx
    sub r11, r10
    jz _equal

_tail_loop:
    mov al,  [rdi + r10]
    cmp al,  [rsi + r10]
    jne _mismatch
    inc r10
    dec r11
    jnz _tail_loop

_equal:
    vzeroupper
    mov eax, 1
    ret

_mismatch:
    vzeroupper
    xor eax, eax
    ret

; end asm__detail_strcmp_avx_cs