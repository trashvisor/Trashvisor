#include <Windows.h>
#include "../Trashvisor/Shared.h"

#include <iostream>

int main()
{
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

    auto FilePath = L"C:\\Users\\trashcan\\Desktop\\Log.txt";
    auto ProcName = L"Uniquename.exe";
    wmemcpy(InfoIoctl.FilePath, FilePath, std::char_traits<wchar_t>::length(FilePath));
    wmemcpy(InfoIoctl.ProcessName, ProcName, std::char_traits<wchar_t>::length(ProcName));

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
    
    return 0;
}