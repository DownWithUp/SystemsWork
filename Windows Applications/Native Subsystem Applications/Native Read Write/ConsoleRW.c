#define  _AMD64_
#include <ntifs.h>
#include <intrin.h>

#pragma comment(lib, "ntdll.lib")

#define PROCESS_QUERY_LIMITED_INFORMATION	0x1000
#define PROCESS_SUSPEND_RESUME			0x800
#define FILE_SHARE_ALL				0x00000007


extern NTSTATUS NtDisplayString(PUNICODE_STRING String);
extern NTSTATUS NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitCode);
extern NTSTATUS NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
extern NTSTATUS NtSuspendProcess(HANDLE ProcessHandle);
extern NTSTATUS NtResumeProcess(HANDLE ProcessHandle);
extern NTSTATUS NtCreateEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, BOOLEAN InitialState);
extern NTSTATUS NtClearEvent(HANDLE EventHandle);
extern NTSTATUS NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);

NTSTATUS OpenKeyboard(PHANDLE Handle)
{
	OBJECT_ATTRIBUTES	objAttr;
	IO_STATUS_BLOCK		ioBlock;
	UNICODE_STRING		usDriver;
	NTSTATUS		ntRet;
	HANDLE			hDriver;

	RtlInitUnicodeString(&usDriver, L"\\Device\\KeyboardClass0");
	InitializeObjectAttributes(&objAttr, &usDriver, OBJ_CASE_INSENSITIVE, NULL, NULL);
	ntRet = NtCreateFile(&hDriver, SYNCHRONIZE | GENERIC_READ | FILE_READ_ATTRIBUTES, &objAttr, &ioBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_ALL, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, 0);
	*Handle = hDriver;
	return(ntRet);
}

NTSTATUS WaitForInput(HANDLE hDriver, HANDLE hEvent, PVOID Buffer, PULONG BufferSize)
{
	IO_STATUS_BLOCK ioBlock;
	LARGE_INTEGER	ByteOffset;
	NTSTATUS	Status;

	RtlZeroMemory(&ioBlock, sizeof(ioBlock));
	RtlZeroMemory(&ByteOffset, sizeof(ByteOffset));

	Status = NtReadFile(hDriver, hEvent, NULL, NULL, &ioBlock,
		Buffer, *BufferSize, &ByteOffset, NULL);

	if (Status == STATUS_PENDING) {
		// Wait on the data to be read
		Status = NtWaitForSingleObject(hEvent, TRUE, NULL);
	}

	*BufferSize = ioBlock.Information;
	return Status;
}


// https://learn.microsoft.com/en-us/windows/win32/api/ntddkbd/ns-ntddkbd-keyboard_input_data
typedef struct _KEYBOARD_INPUT_DATA 
{
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

NTSTATUS NtProcessStartup()
{
	PROCESS_BASIC_INFORMATION	BasicProcessInformation;
	KEYBOARD_INPUT_DATA		keyboardData;
	OBJECT_ATTRIBUTES		objAttr;
	OBJECT_ATTRIBUTES		objProcessAttr;
	UNICODE_STRING			usString;
	CLIENT_ID			client;
	NTSTATUS			ntRet;		
	HANDLE				hProcess;
	HANDLE				hEvent;
	HANDLE				hKeyboard;
	ULONG				ulSize;

	RtlInitUnicodeString(&usString, L"Press 'C' to continue...\n");
	NtDisplayString(&usString);

	RtlZeroMemory(&BasicProcessInformation, sizeof(BasicProcessInformation));
	ntRet = NtQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, &BasicProcessInformation, sizeof(BasicProcessInformation), NULL);
	client.UniqueProcess = BasicProcessInformation.InheritedFromUniqueProcessId;
	client.UniqueThread = 0;

	RtlZeroBytes(&objProcessAttr, sizeof(objProcessAttr));
	ntRet = NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SUSPEND_RESUME, &objProcessAttr, &client);
	if (NT_SUCCESS(ntRet))
	{
		NtSuspendProcess(hProcess);
		OpenKeyboard(&hKeyboard);

		InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);
		NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, &objAttr, 1, 0);
		ulSize = sizeof(keyboardData);
		do
		{
			RtlZeroMemory(&keyboardData, sizeof(keyboardData));
			NtClearEvent(hEvent);
			WaitForInput(hKeyboard, hEvent, &keyboardData, &ulSize);
			// 'C' scan code
			// https://www.millisecond.com/support/docs/current/html/language/scancodes.htm
		} while (keyboardData.MakeCode != 46);

		RtlInitUnicodeString(&usString, L":)\n");
		NtDisplayString(&usString);
		NtResumeProcess(hProcess);
	}

	NtTerminateProcess(NtCurrentProcess(), 0);
	// Shouldn't reach here
	return(STATUS_SUCCESS);
}
