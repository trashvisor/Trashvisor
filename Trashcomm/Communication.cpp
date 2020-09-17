#include <Windows.h>
#include "../Trashvisor/Shared.h"
#include <iostream>
#include <psapi.h>
#include <tlhelp32.h>

DWORD
GetPidByName (
    const wchar_t* ProcessName
)
{
    auto Snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        NULL
    );
    
    PROCESSENTRY32 ProcEntry;
    ProcEntry.dwSize = sizeof(ProcEntry);
    Process32First(Snapshot, &ProcEntry);

    do
    {
        if (lstrcmpW(ProcEntry.szExeFile, ProcessName) == 0)
        {
            return ProcEntry.th32ProcessID;
        }
    } while (Process32Next(Snapshot, &ProcEntry));

    return -1;
}

UINT64
AddEptHook (
)
{
    // Search through all processes
    auto Pid = GetPidByName(L"minigamz.exe");
    //auto Pid = GetPidByName(L"Project1.exe");

    if (Pid == -1)
    {
        std::cout << "Could not find pid for name" << std::endl;
        return 0;
    }

    INFO_IOCTL_EPT_HOOK_INFO EptHookInfo;
    EptHookInfo.ProcessId = Pid;
    
    std::cout << "Pid: " << Pid << std::endl;

    auto ProcessHandle = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        Pid
    );

    if (ProcessHandle == NULL)
    {
        std::cout << "Could not open process" << std::endl;
        return 0;
    }

    DWORD SizeNeeded;
    HMODULE Modules[50];

    auto AcquiredModules = EnumProcessModules(
        ProcessHandle,
        Modules,
        sizeof Modules,
        &SizeNeeded
    );

    if (!AcquiredModules)
    {
        std::cout << "Could not acquire modules" << std::endl;
        return 0;
    }

    if (SizeNeeded > sizeof Modules)
    {
        std::cout << "Size needed: " << SizeNeeded << std::endl;
        return 0;
    }

    auto AcquiredCount = SizeNeeded / sizeof HMODULE;
    std::cout << "Successfully acquired " << AcquiredCount << " modules" << std::endl;

    UINT64 ModuleAddress = 0;

    for (int i = 0; i < AcquiredCount; i++)
    {
        const auto& Module = Modules[i];
        PCHAR ImageStart = 0;

        RtlCopyMemory(
            &ImageStart,
            &Module,
            sizeof Module
        );
        
        std::cout << "Module at: 0x" << std::hex << reinterpret_cast<PVOID>(ImageStart) << std::endl;

        CHAR Buffer[4];
        SIZE_T SizeRead = 0;

        auto ReadSuccess = ReadProcessMemory(
            ProcessHandle,
            ImageStart + 0x1000,
            Buffer,
            sizeof Buffer,
            &SizeRead
        );

        if (!ReadSuccess)
        {
            std::cout << "ReadProcessMemory failed with: 0x" << std::hex << GetLastError() << std::endl;
            return 0;
        }

        if (RtlCompareMemory(Buffer, "\x22\x05\x93\x19", 4) == 4)
        //if (RtlCompareMemory(Buffer, "\x48\x83\xEC\x28", 4) == 4)
        {
            std::cout << "Found minigamz.exe module at 0x" << std::hex << reinterpret_cast<PVOID>(Module) << std::endl;

            RtlCopyMemory(
                &ModuleAddress,
                &Module,
                sizeof ModuleAddress
            );

            break;
        }
    }

    if (ModuleAddress == 0)
    {
        std::cout << "Could not find minigamz.exe module" << std::endl;
        return 0;
    }

    PCHAR pModuleAddress = 0;

    ModuleAddress = (ModuleAddress + 0x5000) & ~0xfffULL;
    //ModuleAddress = (ModuleAddress + 0x1F23) & ~0xfffULL;

    std::cout << "Aligned address: 0x" << std::hex << ModuleAddress << std::endl;

    RtlCopyMemory(
        &pModuleAddress,
        &ModuleAddress,
        sizeof pModuleAddress
    );

    SIZE_T BytesRead = 0;

    auto ReadSuccess = ReadProcessMemory(
        ProcessHandle,
        pModuleAddress,
        EptHookInfo.ModifiedPage,
        sizeof EptHookInfo.ModifiedPage,
        &BytesRead
    );


    if (!ReadSuccess)
    {
        std::cout << "ReadProcessMemory failed on page: 0x" << std::hex << pModuleAddress << std::endl;
        return 0;
    }

    std::cout << "Modified page value 1: 0x" << std::hex << ((int)EptHookInfo.ModifiedPage[0] & 0xff) << std::endl;
    
    EptHookInfo.ModifiedPage[0x9ca] = 0;
    EptHookInfo.ModifiedPage[0x9cb] = 0;
    EptHookInfo.ModifiedPage[0x9cc] = 0;
    EptHookInfo.ModifiedPage[0x9cd] = 0;

    EptHookInfo.ModifiedPage[0x9d1] = 0;
    EptHookInfo.ModifiedPage[0x9d2] = 0;
    EptHookInfo.ModifiedPage[0x9d3] = 0;
    EptHookInfo.ModifiedPage[0x9d4] = 0;

    //EptHookInfo.ModifiedPage[0xf48] = 0x44;

    RtlCopyMemory(
        &EptHookInfo.VirtualAddress,
        &pModuleAddress,
        sizeof EptHookInfo.VirtualAddress
    );

    auto DeviceHandle = CreateFileW(
        WIN32_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Could not open a handle to " << WIN32_DEVICE_NAME << std::endl;
        return 0;
    }

    DWORD BytesReturned = 0;

    Sleep(1000);
    
    auto IoCtrlStatus = DeviceIoControl(
        DeviceHandle,
        IOCTL_ADD_EPT_HOOK,
        &EptHookInfo,
        sizeof EptHookInfo,
        nullptr,
        0,
        &BytesReturned,
        nullptr
    );
    
    if (!IoCtrlStatus)
    {
        std::cout << "DeviceIoControl failed with error: 0x" << std::hex << GetLastError() << std::endl;
        return 0;
    }

    return 1;
}

UINT64
AddEptHook2 (
)
{
    // Search through all processes
    auto Pid = GetPidByName(L"minigamz.exe");
    //auto Pid = GetPidByName(L"Project1.exe");

    if (Pid == -1)
    {
        std::cout << "Could not find pid for name" << std::endl;
        return 0;
    }

    INFO_IOCTL_EPT_HOOK_INFO EptHookInfo;
    EptHookInfo.ProcessId = Pid;
    
    std::cout << "Pid: " << Pid << std::endl;

    auto ProcessHandle = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        Pid
    );

    if (ProcessHandle == NULL)
    {
        std::cout << "Could not open process" << std::endl;
        return 0;
    }

    DWORD SizeNeeded;
    HMODULE Modules[50];

    auto AcquiredModules = EnumProcessModules(
        ProcessHandle,
        Modules,
        sizeof Modules,
        &SizeNeeded
    );

    if (!AcquiredModules)
    {
        std::cout << "Could not acquire modules" << std::endl;
        return 0;
    }

    if (SizeNeeded > sizeof Modules)
    {
        std::cout << "Size needed: " << SizeNeeded << std::endl;
        return 0;
    }

    auto AcquiredCount = SizeNeeded / sizeof HMODULE;
    std::cout << "Successfully acquired " << AcquiredCount << " modules" << std::endl;

    UINT64 ModuleAddress = 0;

    for (int i = 0; i < AcquiredCount; i++)
    {
        const auto& Module = Modules[i];
        PCHAR ImageStart = 0;

        RtlCopyMemory(
            &ImageStart,
            &Module,
            sizeof Module
        );
        
        std::cout << "Module at: 0x" << std::hex << reinterpret_cast<PVOID>(ImageStart) << std::endl;

        CHAR Buffer[4];
        SIZE_T SizeRead = 0;

        auto ReadSuccess = ReadProcessMemory(
            ProcessHandle,
            ImageStart + 0x1000,
            Buffer,
            sizeof Buffer,
            &SizeRead
        );

        if (!ReadSuccess)
        {
            std::cout << "ReadProcessMemory failed with: 0x" << std::hex << GetLastError() << std::endl;
            return 0;
        }

        if (RtlCompareMemory(Buffer, "\x22\x05\x93\x19", 4) == 4)
        //if (RtlCompareMemory(Buffer, "\x48\x83\xEC\x28", 4) == 4)
        {
            std::cout << "Found minigamz.exe module at 0x" << std::hex << reinterpret_cast<PVOID>(Module) << std::endl;

            RtlCopyMemory(
                &ModuleAddress,
                &Module,
                sizeof ModuleAddress
            );

            break;
        }
    }

    if (ModuleAddress == 0)
    {
        std::cout << "Could not find minigamz.exe module" << std::endl;
        return 0;
    }

    PCHAR pModuleAddress = 0;

    ModuleAddress = (ModuleAddress + 0x902b) & ~0xfffULL;
    //ModuleAddress = (ModuleAddress + 0x45000) & ~0xfffULL;

    std::cout << "Aligned address: 0x" << std::hex << ModuleAddress << std::endl;

    RtlCopyMemory(
        &pModuleAddress,
        &ModuleAddress,
        sizeof pModuleAddress
    );

    SIZE_T BytesRead = 0;

    auto ReadSuccess = ReadProcessMemory(
        ProcessHandle,
        pModuleAddress,
        EptHookInfo.ModifiedPage,
        sizeof EptHookInfo.ModifiedPage,
        &BytesRead
    );


    if (!ReadSuccess)
    {
        std::cout << "ReadProcessMemory failed on page: 0x" << std::hex << pModuleAddress << std::endl;
        return 0;
    }

    std::cout << "Modified page value 1: 0x" << std::hex << ((int)EptHookInfo.ModifiedPage[0x2b] & 0xff) << std::endl;
    
    
    EptHookInfo.ModifiedPage[0x2b] = 0xeb;
    EptHookInfo.ModifiedPage[0x2c] = 0x29;
    EptHookInfo.ModifiedPage[0x2d] = 0xcc;
    EptHookInfo.ModifiedPage[0x2e] = 0xcc;
    EptHookInfo.ModifiedPage[0x2f] = 0xcc;
    EptHookInfo.ModifiedPage[0x30] = 0xcc;

    EptHookInfo.ModifiedPage[0x5a] = 0x10;
    
    //EptHookInfo.ModifiedPage[0x4] = 0;

    RtlCopyMemory(
        &EptHookInfo.VirtualAddress,
        &pModuleAddress,
        sizeof EptHookInfo.VirtualAddress
    );

    auto DeviceHandle = CreateFileW(
        WIN32_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Could not open a handle to " << WIN32_DEVICE_NAME << std::endl;
        return 0;
    }

    DWORD BytesReturned = 0;

    Sleep(1000);
    
    auto IoCtrlStatus = DeviceIoControl(
        DeviceHandle,
        IOCTL_ADD_EPT_HOOK,
        &EptHookInfo,
        sizeof EptHookInfo,
        nullptr,
        0,
        &BytesReturned,
        nullptr
    );
    
    if (!IoCtrlStatus)
    {
        std::cout << "DeviceIoControl failed with error: 0x" << std::hex << GetLastError() << std::endl;
        return 0;
    }

    CHAR Buff[0x10];
    /*ReadProcessMemory(
        ProcessHandle,
        pModuleAddress,
        Buff,
        sizeof Buff,
        &BytesRead
    );

    std::cout << "Buff: ";

    for (int i = 0; i < 0x10; i++)
    {
        std::cout << ((int)Buff[i] & 0xff) << " ";
    }
    std::cout << std::endl;
    */
    return 1;
}

int
main (
    int Argc,
    char* Argv[]
)
{

    AddEptHook();
    AddEptHook2();
    /*
    auto Handle = CreateFileW(
        WIN32_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (Handle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Handle sucked. Error: " << "0x" << std::hex << GetLastError() << std::endl;
        return -1;
    }

    INFO_IOCTL_CPUID_PROCESS InfoIoctl;

    RtlZeroMemory(
        &InfoIoctl,
        sizeof(InfoIoctl)
    );

    auto FilePath = L"\\DosDevices\\C:\\Users\\trashcan\\Desktop\\Log.txt";
    auto ProcName = L"Project1.exe";
    wmemcpy(InfoIoctl.FilePath, FilePath, std::char_traits<wchar_t>::length(FilePath));
    wmemcpy(InfoIoctl.ProcessName, ProcName, std::char_traits<wchar_t>::length(ProcName));
    InfoIoctl.LogCount = 100;

    DWORD BytesReturned;

    auto Status = DeviceIoControl(
        Handle,
        IOCTL_LOG_CPUID_PROCESS,
        &InfoIoctl,
        sizeof(InfoIoctl),
        nullptr,
        0,
        &BytesReturned,
        nullptr
    );

    if (!Status)
    {
        std::cout << "DeviceIoControl failed. Error: " << "0x" << std::hex << GetLastError() << std::endl;
        return -1;
    }

    */
    return 0;
}