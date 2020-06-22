# C8asm
![code size shield](https://img.shields.io/github/languages/code-size/c99zealot/c8asm?style=plastic)
![top language shield](https://img.shields.io/github/languages/top/c99zealot/c8asm?label=C&style=plastic)
![license shield](https://img.shields.io/github/license/c99zealot/c8asm?style=plastic)
![goto counter shield](https://img.shields.io/github/search/c99zealot/c8asm/goto?style=plastic)

# Usage
`./c8asm <c8asm source file> <output file name>` (if no name is supplied for the output file then "out.ch8" is used)

# Language documentation
`the following assumes the reader is familiar with the CHIP8 architecture`

# Names
C8asm has 58 reserved names, these consist of..

25 mnemonics:
```
cls
jmp
vjmp
call
ret
sne
se
mov
or
and
xor
add
sub
subn
shr
shl
rnd
drw
wkp
skd
sku
ldf
bcd
lod
str
```

3 reserved keywords:
```
stimer
dtimer
I
```

30 register names:
```
v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 va vb vc vd ve vf
V0 V1 V2 V3 V4 V5 V6 V7 V8 V9 VA VB VC VD VE VF
```

These are referred to as "names" in error messages.

# Constants
Constants are used to supply operands to mnemonics and c8asm supports four formats for integer constants:

<table style="width:100%;border:1px solid black">
        <tr>
                <th style="border:1px solid black">format</th>
                <th style="border:1px solid black">notation</th>
        </tr>
        <tr>
                <td style="border:1px solid black">decimal</td>
                <td style="border:1px solid black">&ltdecimal number&gt</td>
        </tr>
        <tr>
                <td style="border:1px solid black">hexadecimal</td>
                <td style="border:1px solid black">0x&lthexadecimal number&gt</td>
        </tr>
        <tr>
                <td style="border:1px solid black">binary</td>
                <td style="border:1px solid black">0b&ltbinary number&gt</td>
        </tr>
        <tr>
                <td style="border:1px solid black">octal</td>
                <td style="border:1px solid black">0o&ltoctal number&gt</td>
        </tr>
</table>

# Labels
Labels are supported as an abstraction over addresses and can be used as operands to instructions jmp, vjmp and call.
To define a label a name which is not a reserved keyword is written before a colon (:) and to reference a label it's
name is written with no leading or trailing characters.

Characters which are valid in label names include underscores, lower and upper case ASCII characters and digits. Note
that digits may not be used as the first character of a label name, for example, `7abel` is not a valid label name but
`_7abel` is.

Simple examples of using labels:
```
loop:
        add v0, 1
        jmp loop
```
```
jmp skip_xor
xor v0, v0
skip_xor:
```
```
call some_proc
add va, vb

some_proc:
        xor v0, v0
        ret
```

# Mnemonics and their expected operands
