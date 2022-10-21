format PE64 Native
include 'win64ax.inc'
include 'wdm.inc'

entry DriverEntry

section '.data' data readable writeable notpageable
    HELLO_STRING        db "Hello world!", 0xA, 0
    g_DeviceName        UNICODE_STRING

align 8
    DeviceName      du '\Device\HelloDriver',0
align 8
    pDeviceObject   dq 0

section '.text' code readable executable notpageable

proc DriverEntry DriverObject, RegistryPath
    mov [DriverObject], rcx
    mov [RegistryPath], rdx
    local device_obj dq 0
    frame ; Frame of the invoke macros...
    mov rax, [DriverObject]
    lea rdx, [DriverUnload]
    mov [rax + DRIVER_OBJECT.DriverUnload], rdx
    invoke RtlInitUnicodeString, g_DeviceName, DeviceName	
    lea rdx, [pDeviceObject]
    mov [device_obj], rdx
    invoke IoCreateDevice, [DriverObject], 0, g_DeviceName, FILE_DEVICE_UNKNOWN, 0, 0, [device_obj] ; FIX This last parameter
    .if eax = STATUS_SUCCESS
        mov rcx, [DriverObject]
        lea rdx, [DispatchCreateClose]
        mov	[rcx + DRIVER_OBJECT.MajorFunction + (IRP_MJ_CREATE * 8)], rdx
        mov	[rcx + DRIVER_OBJECT.MajorFunction + (IRP_MJ_CLOSE * 8)], rdx		
        invoke DbgPrint, HELLO_STRING
    .endif
    endf
    ret
endp

proc DriverUnload DriverObject
    lea rcx, [pDeviceObject]
    .if QWORD [rcx] <> 0
        mov rcx, [rcx]
        invoke IoDeleteDevice, rcx
    .endif
    xor eax, eax ; STATUS_SUCCESS
    ret
endp

proc DispatchCreateClose DeviceObject, Request
    ; RCX = DeviceObject
    ; RDX = IRP
    invoke IoCompleteRequest, rdx, IO_NO_INCREMENT
    xor	eax, eax ; STATUS_SUCCESS
    ret
endp


section '.import' import readable writeable discardable
    library ntoskrnl,'ntoskrnl.exe'
    import	ntoskrnl,\
        RtlInitUnicodeString,'RtlInitUnicodeString',\
        IoCreateDevice,'IoCreateDevice',\
        IoDeleteDevice,'IoDeleteDevice',\
        IoCompleteRequest,'IoCompleteRequest',\
        DbgPrint,'DbgPrint'

section '.reloc' data fixups readable discardable