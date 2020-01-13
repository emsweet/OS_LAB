section .data
color_red:      
        db      1Bh, '[31;1m', 0
.len    equ     $ - color_red
color_blue:     
        db      1Bh, '[34;1m', 0
.len    equ     $ - color_blue
color_default:  
        db      1Bh, '[37;0m', 0
.len    equ     $ - color_default

global my_print

section .text
my_print:
            mov eax,[esp+12]
            cmp eax,0
            je   label_b
            call set_red
            jmp label_b
            call set_default
label_b:
            mov  ecx,[esp+4] 
            mov edx,[esp+8]
            mov ebx,1
            mov eax,4
            int 80h
            call set_default
            ret
set_red:
           mov     eax, 4                 ; 系统调用号为4
           mov     ebx, 1                  
           mov     ecx, color_red
        mov     edx, color_red.len
         int     80h
ret
set_default:
            mov     eax, 4                 ; 系统调用号为4
            mov     ebx, 1                  
            mov     ecx, color_default
            mov     edx, color_default.len
             int     80h
ret