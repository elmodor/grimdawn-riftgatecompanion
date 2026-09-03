#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdarg>

inline void Log(const char* fmt, ...)
{
#ifndef NOLOG
    FILE* f = fopen("RiftgateCompanion.log", "a");
    if(!f)
        return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02u:%02u:%02u.%03u] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);
    fclose(f);
#endif
}
