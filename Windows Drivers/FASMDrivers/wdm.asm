FILE_DEVICE_UNKNOWN                 equ 00000022h
IRP_MJ_MAXIMUM_FUNCTION             equ 1Bh
IRP_MJ_CREATE                       equ 0
IRP_MJ_CLOSE                        equ 2
IRP_MJ_DEVICE_CONTROL               equ 0Eh
STATUS_DEVICE_CONFIGURATION_ERROR   equ 00C0000182h
IO_NO_INCREMENT                     equ 0
RTL_QUERY_REGISTRY_DIRECT           equ 00000020h
RTL_REGISTRY_WINDOWS_NT             equ 3

STATUS_SUCCESS                      equ 0


struct UNICODE_STRING
    Length          rw 1	; The length in bytes of the string stored in Buffer.
    MaximumLength   rw 1	; The maximum length in bytes of Buffer.
    Buffer          rq 1	; Pointer to a buffer used to contain a string of wide characters.
ends


struct DRIVER_OBJECT    ; sizeof 0A8h
    Type                rw 1 ; 0000h
    Size                rw 1 ; 0002h
    .padding            rd 1
    DeviceObject        rq 1 ; 0008h
    Flags               rd 1 ; 0010h
    .padding2           rd 1
    DriverStart         rq 1 ; 0018h
    DriverSize          rd 1 ; 0020h
    .padding3           rd 1
    DriverSection       rq 1 ; 0028h
    DriverExtension     rq 1 ; 0030h
    DriverName          rq 2 ; 0038h
    HardwareDatabase    rq 1 ; 0048h
    FastIoDispatch      rq 1 ; 0050h
    DriverInit          rq 1 ; 0058h
    DriverStartIo       rq 1 ; 0060h
    DriverUnload        rq 1 ; 0068h
    MajorFunction       rq (IRP_MJ_MAXIMUM_FUNCTION + 1)  ; 0070h
ends

