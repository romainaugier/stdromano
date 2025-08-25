section .text

global asm__detail_strcmp_cs

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

global asm__detail_strcmp_avx_cs

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
    cmp     eax, -1
    jne     _mismatch

    add     r9, 32
    cmp     r9, r10
    jb      _simd

_tail:
    mov     r11, rdx
    sub     r11, r10
    jz      _equal

_tail_loop:
    mov     al,  [rdi + r10]
    cmp     al,  [rsi + r10]
    jne     _mismatch
    inc     r10
    dec     r11
    jnz     _tail_loop

_equal:
    vzeroupper
    mov     eax, 1
    ret

_mismatch:
    vzeroupper
    xor     eax, eax
    ret

; end strcmp_avx_kernel_case_sensitive