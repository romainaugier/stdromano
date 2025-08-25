PUBLIC asm__detail_strcmp_cs
PUBLIC asm__detail_strcmp_avx_cs

.code

asm__detail_strcmp_cs PROC
; RCX = lhs, RDX = rhs, R8 = length
    xor r9, r9

_loop:
    movzx eax, byte ptr [rcx + r9]
    movzx r10d, byte ptr [rdx + r9]
    cmp al, r10b
    jne _fail
    inc r9
    cmp r9, r8
    jne _loop

; _done
    mov rax, 1
    ret

_fail:
    xor rax, rax
    ret

asm__detail_strcmp_cs ENDP

asm__detail_strcmp_avx_cs PROC
    ; simd_loop_size = length & ~31
    mov     r10, r8
    and     r10, -32                 ; r10 = simd_loop_size
    xor     r9d, r9d                 ; r9 = i

    test    r10, r10
    je      _tail

_simd:
    prefetcht0 byte ptr [rcx + r9 + 192]
    prefetcht0 byte ptr [rdx + r9 + 192]

    vmovdqa ymm0, ymmword ptr [rcx + r9]
    vmovdqa ymm1, ymmword ptr [rdx + r9]
    vpcmpeqb ymm2, ymm0, ymm1            ; 0xFF where equal
    vpmovmskb eax, ymm2                  ; 32-bit mask; 1 bit per byte
    cmp     eax, 0FFFFFFFFh              ; all equal ?
    jne     _mismatch

    add     r9, 32
    cmp     r9, r10
    jb      _simd

_tail:
    ; remaining_size
    mov     r11, r8
    sub     r11, r10                     ; r11 = remaining
    jz      _equal

_tail_loop:
    mov     al,  byte ptr [rcx + r10]
    cmp     al,  byte ptr [rdx + r10]
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

asm__detail_strcmp_avx_cs ENDP

END