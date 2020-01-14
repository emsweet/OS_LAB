
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
INT_VECTOR_SYS_CALL equ 0x90
_NR_system_process_sleep equ 1
_NR_system_disp_str      equ 2
_NR_system_P 		equ 3
_NR_system_V		equ 4

; 导出符号
global	get_ticks
global	system_process_sleep
global  system_disp_str

global	system_P
global  system_V

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
    push ebx
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

;自己加的四个系统调用
system_process_sleep:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_system_process_sleep
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

system_disp_str:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_system_disp_str
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

system_P:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_system_P
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

system_V:
	push ebx
	mov ebx, [esp + 8]
	mov	eax, _NR_system_V
	int	INT_VECTOR_SYS_CALL
	pop ebx
	ret

