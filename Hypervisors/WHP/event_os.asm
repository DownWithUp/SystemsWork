Format binary

use16
org 0x7C00

enter_protected_mode:
        ; Setup messy stack segment
        mov ax, 0x10
        mov ss, ax 
        xor ax, ax
        mov ds, ax

        cli
        lgdt [gdt_descriptor]   
        mov eax, cr0
        or al, 1
        mov cr0, eax 

        jmp 8:protected_mode

        use32
        protected_mode:
        sti
        mov bx, 0x10
        mov ds, bx ; set data segment
        mov es, bx ; set extra segment
        ;and al, 0xfe    ; clear protected mode bit
        ;mov cr0, eax
        ;in al, 0x70
        ; Load IDT
        lidt [idt_descriptor]

        in al, 0xAA

        infinite_loop:
                jmp $
        
        halt_cpu:
                hlt

align 8
gdt_descriptor:
        ; Limit
        .limit dw 0xFFFF
        ; Table
        .base dd gdt_descriptors

align 8
gdt_descriptors:
        .null_descriptor dq 0
        ; Offset 0x8
        .code_descriptor:        ; cs should point to this descriptor
	dw 0xffff               ; segment limit first 0-15 bits
	dw 0                    ; base first 0-15 bits
	db 0                    ; base 16-23 bits
	db 0x9a                 ; access byte
	db 11001111b            ; high 4 bits (flags) low 4 bits (limit 4 last bits)(limit is 20 bit wide)
	db 0                    ; base 24-31 bits
        ; Offset 0x10
        .data_descriptor:        ; ds should point to this descriptor
	dw 0xffff               ; segment limit first 0-15 bits
	dw 0                    ; base first 0-15 bits
	db 0                    ; base 16-23 bits
	db 0x92                 ; access byte
	db 11001111b            ; high 4 bits (flags) low 4 bits (limit 4 last bits)(limit is 20 bit wide)
	db 0                    ; base 24-31 bits

align 8
idt_descriptor:
        ; Limit. Should be (8*N) -1  - SDM Volume 3: Figure 6-1
        dw (7 * 8) - 1
        ; Table
        dd idt_descriptors

align 8
idt_descriptors:
        ; For more info, see: https://wiki.osdev.org/Interrupt_Descriptor_Table
        .cpl0_idt_gate_0:
                dq 0
        .cpl0_idt_gate_1:
                dq 0
        .cpl0_idt_gate_2:
                dq 0
        .cpl0_idt_gate_3:
                dq 0
        .cpl0_idt_gate_4:
                dq 0
        .cpl0_idt_gate_5:
                dq 0
        .cpl0_idt_gate_6:
                ; Trap gate (#UD)
                dw cpl0_int8_handler    ; Lower offset
                dw 8                    ; Segment selector
                db 0                    ; Reserved and constants
                db 10001111b            ; P, DPL D and constants (least significant 111 = Trap Gate, 110 = Interrupt Gate)
                dw 0                    ; Upper offset

; IDT gate handler functions
cpl0_int8_handler:
        nop
        nop
        in al, 0x44
        iret


; Rest of the floppy
times 510 - ($ - $$) db 0
dw 0xAA55
