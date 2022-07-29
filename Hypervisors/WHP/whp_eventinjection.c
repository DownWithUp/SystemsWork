#include <windows.h>
#include <stdio.h>
#include <WinHvPlatform.h>

#pragma comment(lib, "WinHvPlatform.lib")

#define GUEST_MEMORY_SIZE 0x100000

void PrintHResultMessage(HRESULT Result)
{
	CHAR szBuffer[512] = { 0 };

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)szBuffer, 512, NULL);

	printf("%ls\n", szBuffer);
}

BOOL LoadOS(CHAR* FileName, PVOID Buffer, DWORD BufferSize)
{
	HANDLE	hFile;
	DWORD	dwFileSize;

	hFile = CreateFileA(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return(FALSE);
	}

	dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize >= BufferSize)
	{
		CloseHandle(hFile);
		return(FALSE);
	}

	printf("Buffer address: %p\n", Buffer);

	if (!ReadFile(hFile, Buffer, dwFileSize, NULL, NULL))
	{
		CloseHandle(hFile);
		return(FALSE);
	}

	return(TRUE);
}

int main(int argc, char* argv[])
{
	WHV_GUEST_PHYSICAL_ADDRESS	guestPhysAddr;
	WHV_RUN_VP_EXIT_CONTEXT		exitContext;
	WHV_PARTITION_PROPERTY		partitionProperty;
	WHV_PARTITION_HANDLE		hPartition;
	WHV_REGISTER_VALUE			registerValues[5] = { 0 };
	WHV_REGISTER_NAME			registerNames[] = { WHvX64RegisterRax, WHvX64RegisterRip, WHvX64RegisterCr0, WHvX64RegisterCs };
	HRESULT						hResult;
	LPVOID						lpGuestMem;

	if (argc != 2)
	{
		printf("Usage: %s: [Example OS File]\n", argv[0]);
		return(-1);
	}

	hResult = WHvCreatePartition(&hPartition);
	if (hResult != S_OK)
	{
		printf("Error on WHvCreatePartition: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	RtlSecureZeroMemory(&partitionProperty, sizeof(partitionProperty));
	partitionProperty.ProcessorCount = 1;
	hResult = WHvSetPartitionProperty(hPartition, WHvPartitionPropertyCodeProcessorCount, &partitionProperty, sizeof(partitionProperty));
	if (hResult != S_OK)
	{
		printf("Error on WHvSetPartitionProperty: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	// Set up the partition with any of the properties
	WHvSetupPartition(hPartition);
	hResult = WHvCreateVirtualProcessor(hPartition, 0, 0);
	if (hResult != S_OK)
	{
		printf("Error on WHvCreateVirtualProcessor: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	hResult = WHvGetVirtualProcessorRegisters(hPartition, 0, &registerNames, sizeof(registerNames) / sizeof(WHV_REGISTER_NAME), &registerValues);
	if (hResult != S_OK)
	{
		printf("Error on WHvGetVirtualProcessorRegisters: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	// Initial state
	registerValues[1].Reg64 = 0x7c00;	// IP
	registerValues[2].Reg64 = 0;		// CR0
	registerValues[3].Segment.Base = 0; // CS.Base

	hResult = WHvSetVirtualProcessorRegisters(hPartition, 0, &registerNames, sizeof(registerNames) / sizeof(WHV_REGISTER_NAME), &registerValues);
	if (hResult != S_OK)
	{
		printf("Error on WHvSetVirtualProcessorRegisters: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	hResult = WHvGetVirtualProcessorRegisters(hPartition, 0, &registerNames, sizeof(registerNames) / sizeof(WHV_REGISTER_NAME), &registerValues);

	if (hResult != S_OK)
	{
		printf("Error on WHvGetVirtualProcessorRegisters: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	lpGuestMem = VirtualAlloc(0x0, GUEST_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	printf("Guest Memory Host Address: %I64X\n", lpGuestMem);
	if (!LoadOS(argv[1], (BYTE*)lpGuestMem + 0x7C00, GUEST_MEMORY_SIZE))
	{
		printf("Error loading example OS file! GetLastError: 0x%X\n", GetLastError());
		return(-1);
	}

	guestPhysAddr = 0;
	hResult = WHvMapGpaRange(hPartition, lpGuestMem, guestPhysAddr, 0x100000000, WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite | WHvMapGpaRangeFlagExecute);
	if (hResult != S_OK)
	{
		printf("Error on WHvMapGpaRange: %x\n", hResult);
		PrintHResultMessage(hResult);
		return(-1);
	}

	ZeroMemory(&exitContext, sizeof(exitContext));

	// Infinite vmrun loop
	while (TRUE)
	{
		printf("Starting Run...\n");

		hResult = WHvRunVirtualProcessor(hPartition, 0, &exitContext, sizeof(exitContext));
		printf("Exit Reason: %X (%d)\n", exitContext.ExitReason, exitContext.ExitReason);
		// Read registers after the exit...
		hResult = WHvGetVirtualProcessorRegisters(hPartition, 0, &registerNames, sizeof(registerNames) / sizeof(WHV_REGISTER_NAME), &registerValues);
		switch (exitContext.ExitReason)
		{
		case WHvRunVpExitReasonX64IoPortAccess:
			printf("ExitReason - IoPortAccess: Port: %x\n", exitContext.IoPortAccess.PortNumber);
			printf("ExitReason - IoPortAccess: Access Size: %x\n", exitContext.IoPortAccess.AccessInfo.AccessSize);
			printf("ExitReason - IoPortAccess: VpContext RIP: %I64x\n", exitContext.VpContext.Rip);

			// Increment RIP to pass over the PIO instruction (2 bytes)
			registerValues[1].Reg64 = registerValues[1].Reg64 + 2;
			hResult = WHvSetVirtualProcessorRegisters(hPartition, 0, &registerNames, sizeof(registerNames) / sizeof(WHV_REGISTER_NAME), &registerValues);
			if (exitContext.IoPortAccess.PortNumber == 0xAA)
			{
				printf("Port number matched magic number 0xAA, injecting InvalidOpcode (#UD) event...\n");
				WHV_REGISTER_NAME interruptRegsNames[] = { WHvRegisterPendingEvent };
				WHV_REGISTER_VALUE interruptRegsVals[1] = { 0 };

				interruptRegsVals->ExceptionEvent.EventPending = 1;
				interruptRegsVals->ExceptionEvent.EventType = WHvX64PendingEventException;
				interruptRegsVals->ExceptionEvent.DeliverErrorCode = 0; // $UD does not diliver an error code
				interruptRegsVals->ExceptionEvent.Vector = WHvX64ExceptionTypeInvalidOpcodeFault;

				hResult = WHvSetVirtualProcessorRegisters(hPartition, 0, &interruptRegsNames, sizeof(interruptRegsNames) / sizeof(WHV_REGISTER_NAME), &interruptRegsVals);
			}
			break;
		default:
			printf("[!!!] Unhandled exit reason: 0x%X ()\n", exitContext.ExitReason, exitContext.ExitReason);
			break;
		}

		if (hResult != S_OK)
		{
			printf("error WHvGetVirtualProcessorRegisters: %x\n", hResult);
			PrintHResultMessage(hResult);
			return(-1);
		}
	}

	return(0);
}