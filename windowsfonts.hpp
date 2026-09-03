#pragma once

#include <string>
#include <vector>

struct FontInfo
{
    std::string name;
    std::wstring path;

    bool latin      = false;
    bool cyrillic   = false;
    bool vietnamese = false;
    bool japanese   = false;
    bool korean     = false;
    bool chinese    = false;
};

std::vector<FontInfo> EnumerateWindowsFonts();
