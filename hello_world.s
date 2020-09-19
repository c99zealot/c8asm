; an example hello world program

mov ve, 5   ; offset for sprites
mov I, 0x50 ; start location for sprites

; H
mov v0, 0b10010000
mov v1, 0b10010000
mov v2, 0b11110000
mov v3, 0b10010000
mov v4, 0b10010000
str v4

; E
add I, ve
mov v0, 0b01110000
mov v1, 0b10000000
mov v2, 0b11100000
mov v3, 0b10000000
mov v4, 0b01110000
str v4

; L
add I, ve
mov v0, 0b10000000
mov v2, 0b10000000
str v4

; L
add I, ve
str v4

; O
add I, ve
mov v0, 0b01100000
mov v1, 0b10010000
mov v2, 0b10010000
mov v3, 0b10010000
mov v4, 0b01100000
str v4

; ' '
add I, ve
xor v0, v0
xor v1, v1
xor v2, v2
xor v3, v3
xor v4, v4
str v4

; W
add I, ve
mov v0, 0b10001000
mov v1, 0b10001000
mov v2, 0b10101000
mov v3, 0b10101000
mov v4, 0b01010000
str v4

; O
add I, ve
mov v0, 0b00110000
mov v1, 0b01001000
mov v2, 0b01001000
mov v3, 0b01001000
mov v4, 0b00110000
str v4

; R
add I, ve
mov v0, 0b00110000
mov v1, 0b01001000
mov v2, 0b01110000
mov v3, 0b01001000
mov v4, 0b01001000
str v4

; L
add I, ve
mov v0, 0b01000000
mov v1, 0b01000000
mov v2, 0b01000000
mov v3, 0b01000000
mov v4, 0b00111000
str v4

; D
add I, ve
mov v0, 0b01110000
mov v1, 0b01001000
mov v2, 0b01001000
mov v3, 0b01001000
mov v4, 0b01110000
str v4

; !
add I, ve
mov v0, 0b00110000
mov v1, 0b01111000
mov v2, 0b00110000
mov v3, 0b00000000
mov v4, 0b00110000
str v4

mov vb, 10 ; initial y
mov vc, 1
mov vd, 8
main:
        mov I, 0x50
        xor va, va ; reset x to 0
        draw: ; draw sprites
                drw va, vb, 5
                add I, ve
                add va, 5
                add vb, vc ; offset y by vc
                se va, 60
                jmp draw

        ; increment offset to y
        add vc, 1

        mov dtimer, vd ; delay for 10 cycles
        cls
        jmp main
