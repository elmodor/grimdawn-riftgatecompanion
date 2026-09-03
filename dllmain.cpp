#include <windows.h>
#include <stdio.h>

#include "d3d11_hook.hpp"
#include "logger.hpp"

DWORD WINAPI ThreadProc(void*)
{
    Log("RiftgateCompanion thread started");
    InstallD3D11Hook();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
#ifndef NOLOG
        remove("RiftgateCompanion.log");
#endif
        CreateThread(nullptr, 0, ThreadProc, nullptr, 0, nullptr);
    }
    return TRUE;
}
