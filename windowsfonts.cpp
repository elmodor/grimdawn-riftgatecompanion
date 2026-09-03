#include "windowsfonts.hpp"

#include <windows.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

// Characters used to determine whether a font supports a language
static constexpr std::wstring_view LatinChars =
    L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    L"abcdefghijklmnopqrstuvwxyz";

static constexpr std::wstring_view CyrillicChars =
    L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
    L"абвгдежзийклмнопрстуфхцчшщъыьэюя";

static constexpr std::wstring_view VietnameseChars =
    L"ĂăÂâĐđÊêÔôƠơƯư"
    L"ÀàÁáẢảÃãẠạ"
    L"ẦầẤấẨẩẪẫẬậ"
    L"ẰằẮắẲẳẴẵẶặ"
    L"ÈèÉéẺẻẼẽẸẹ"
    L"ỀềẾếỂểỄễỆệ"
    L"ÌìÍíỈỉĨĩỊị"
    L"ÒòÓóỎỏÕõỌọ"
    L"ỒồỐốỔổỖỗỘộ"
    L"ỜờỚớỞởỠỡỢợ"
    L"ÙùÚúỦủŨũỤụ"
    L"ỪừỨứỬửỮữỰự"
    L"ỲỳÝýỶỷỸỹỴỵ";

static constexpr std::wstring_view JapaneseChars =
    L"日本語漢字"
    L"あいうえおかきくけこ"
    L"さしすせそたちつてと"
    L"なにぬねのはひふへほ"
    L"まみむめもやゆよ"
    L"らりるれろわをん"
    L"アイウエオカキクケコ"
    L"サシスセソタチツテト"
    L"ナニヌネノハヒフヘホ"
    L"マミムメモヤユヨ"
    L"ラリルレロワヲン";

static constexpr std::wstring_view KoreanChars =
    L"한국어한글"
    L"가나다라마바사아자차카타파하";

static constexpr std::wstring_view ChineseChars =
    L"中文汉字"
    L"的一是不了人我在有他这为之大来以个中上们到说时国和地要就出会可也你对生能而子那得于着下自之年过发后作里用道行所然家种事成方多经";


static std::string WideToUtf8(std::wstring_view str)
{
    if (str.empty())
        return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size, nullptr, nullptr);
    return result;
}

static bool FontContainsCharacters(IDWriteFont* font, std::wstring_view characters)
{
    if (!font)
        return false;
    for (wchar_t character : characters)
    {
        BOOL exists = FALSE;
        if (FAILED(font->HasCharacter(character, &exists)))
            return false;
        if (!exists)
            return false;
    }
    return true;
}

static bool GetFontFilePath(IDWriteFont* font, std::wstring& path)
{
    path.clear();
    if (!font)
        return false;
    ComPtr<IDWriteFontFace> fontFace;
    HRESULT hr = font->CreateFontFace(&fontFace);
    if (FAILED(hr))
        return false;
    UINT32 fileCount = 0;
    hr = fontFace->GetFiles(&fileCount, nullptr);
    if (FAILED(hr) || fileCount == 0)
        return false;
    std::vector<IDWriteFontFile*> files(fileCount);
    hr = fontFace->GetFiles(&fileCount, files.data());
    if (FAILED(hr) || fileCount == 0 || files.size() == 0)
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    // We only care about the first font face for now
    IDWriteFontFile* fontFile = files[0];
    const void* referenceKey = nullptr;
    UINT32 referenceKeySize = 0;
    hr = fontFile->GetReferenceKey(&referenceKey, &referenceKeySize);
    if (FAILED(hr))
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    ComPtr<IDWriteFontFileLoader> loader;
    hr = fontFile->GetLoader(&loader);
    if (FAILED(hr))
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    // Get the actual windows path
    ComPtr<IDWriteLocalFontFileLoader> localLoader;
    hr = loader.As(&localLoader);
    if (FAILED(hr))
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    UINT32 pathLength = 0;
    hr = localLoader->GetFilePathLengthFromKey(referenceKey, referenceKeySize, &pathLength);
    if (FAILED(hr))
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    // +1 for null terminator
    std::wstring result(pathLength + 1, L'\0');
    hr = localLoader->GetFilePathFromKey(referenceKey, referenceKeySize, result.data(), pathLength + 1);
    if (FAILED(hr))
    {
        for (IDWriteFontFile* file : files)
        {
            if (file)
                file->Release();
        }
        return false;
    }
    result.resize(pathLength);
    path = std::move(result);
    for (IDWriteFontFile* file : files)
    {
        if (file)
            file->Release();
    }
    return true;
}


// Use english font names
static std::wstring GetFontFamilyName(IDWriteFontFamily* family)
{
    if (!family)
        return {};
    ComPtr<IDWriteLocalizedStrings> names;
    if (FAILED(family->GetFamilyNames(&names)) || !names)
        return {};
    UINT32 index = 0;
    BOOL exists = FALSE;
    names->FindLocaleName(L"en-us", &index, &exists);
    if (!exists)
        index = 0;
    UINT32 length = 0;
    if (FAILED(names->GetStringLength(index, &length)))
        return {};
    std::wstring name(length + 1, L'\0');
    if (FAILED(names->GetString(index, name.data(), length + 1)))
        return {};
    name.resize(length);
    return name;
}


std::vector<FontInfo> EnumerateWindowsFonts()
{
    std::vector<FontInfo> result;
    ComPtr<IDWriteFactory> factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
    if (FAILED(hr))
        return result;
    ComPtr<IDWriteFontCollection> collection;
    hr = factory->GetSystemFontCollection(&collection, FALSE);
    if (FAILED(hr))
        return result;
    const UINT32 familyCount = collection->GetFontFamilyCount();
    result.reserve(familyCount);
    for (UINT32 i = 0; i < familyCount; ++i)
    {
        ComPtr<IDWriteFontFamily> family;
        if (FAILED(collection->GetFontFamily(i, &family)))
            continue;
        std::wstring wideName = GetFontFamilyName(family.Get());
        if (wideName.empty())
            continue;
        ComPtr<IDWriteFont> font;
        hr = family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
        if (FAILED(hr) || !font)
            continue;
        std::wstring path;
        if (!GetFontFilePath(font.Get(), path))
            continue;

        FontInfo info;
        info.name = WideToUtf8(wideName);
        info.path = std::move(path);
        info.latin = FontContainsCharacters(font.Get(), LatinChars);
        info.cyrillic = FontContainsCharacters(font.Get(), CyrillicChars);
        info.vietnamese = FontContainsCharacters(font.Get(), VietnameseChars);
        info.japanese = FontContainsCharacters(font.Get(), JapaneseChars);
        info.korean = FontContainsCharacters(font.Get(), KoreanChars);
        info.chinese = FontContainsCharacters(font.Get(), ChineseChars);
        result.push_back(std::move(info));
    }
    return result;
}
