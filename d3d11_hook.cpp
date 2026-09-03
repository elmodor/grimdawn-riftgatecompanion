#include "d3d11_hook.hpp"

#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <atomic>

#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"

#include "logger.hpp"
#include "function_symbols.hpp"
#include "localization.hpp"
#include "savesystem.hpp"
#include "windowsfonts.hpp"

void InitImGui(IDXGISwapChain* swap);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND g_hwnd = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static ID3D11RenderTargetView* g_renderTarget = nullptr;
static std::atomic<bool> imguiInitialized = false;

static bool menuOpen = false;
static bool menuGrabKeyboard = true;
static bool menuReleaseMouse = false;
static bool menuReleaseKeyboard = false;
static bool menuSortTeleports = true;
static bool menuSortTowns = true;
static bool menuFavoriteTowns = true;
static char menuSearch[128] = "";
static bool lastToggleState = false;
static bool lastEscState = false;
static std::unordered_map<std::string_view, uintptr_t> g_GameAddresses;
static std::unordered_map<std::string_view, uintptr_t> g_EngineAddresses;
static bool updateReq = false;
static bool g_waitingForKey = false;
static bool g_captureArmed = false;
static bool languageChanged = false;

static UINT g_toggleKey = VK_F6;
static std::atomic<bool> showEndlessDungeonNotify = false;
static std::atomic<bool> isEndlessDungeonComplete = false;
static std::atomic<bool> isEndlessDungeonNew = false;
static bool enabledNearestTown = true;
static bool enabledFavoriteTown = true;
static bool enabledConvenientTown = false;
static bool enabledEndlessDungeonNotifyPortal = false;
static float endlessDungeonNotifyYPos = 0.75f;
static bool enabledQuestTrackRestore = false;
static DifficultyData currentSaveData{};

static std::vector<FontInfo> availableFonts;
static FontInfo* userFont = nullptr;
static std::vector<const char*> fontNames;
static bool fontChanged = false;
static int32_t fontSize = 13;

static Localization l;

struct Vec3f
{
    float x;
    float y;
    float z;
};

struct UID
{
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t c;
    std::uint32_t d;

    bool operator==(const UID& other) const
    {
        return a == other.a &&
            b == other.b &&
            c == other.c &&
            d == other.d;
    }
};

struct UIDHash
{
    size_t operator()(const UID& uid) const
    {
        // Hashing as boost is doing it
        size_t h = std::hash<std::uint32_t>{}(uid.a);
        h ^= std::hash<std::uint32_t>{}(uid.b) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint32_t>{}(uid.c) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint32_t>{}(uid.d) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};


#define UI_LANGUAGES(X) \
    X(Automatic)        \
    X(English)          \
    X(Spanish)          \
    X(French)           \
    X(German)           \
    X(Italian)          \
    X(Czech)            \
    X(Japanese)         \
    X(Korean)           \
    X(Polish)           \
    X(Portuguese)       \
    X(Russian)          \
    X(Vietnamese)       \
    X(Chinese)          \
    X(END)

enum class UiLanguages
{
#define X(name) name,
    UI_LANGUAGES(X)
#undef X
};

const char* ToString(UiLanguages value)
{
    switch (value)
    {
#define X(name) case UiLanguages::name: return #name;
        UI_LANGUAGES(X)
#undef X
    }
    return "Unknown";
}

enum class TeleportSort
{
    Default,
    Distance,
    Discovery,
    Alphabetical
};
static UID favoriteTown = {0x800B7FB7, 0xF6944F29, 0xB7F76950, 0x53E76CD7};
static TeleportSort menuTeleportSort = TeleportSort::Default;
static std::atomic<UiLanguages> uiLanguage = UiLanguages::Automatic;

static void* ReadSettingsOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    if (strcmp(name, "Teleport") == 0)
        return (void*)1;
    if (strcmp(name, "EndlessDungeon") == 0)
        return (void*)2;
    if (strcmp(name, "Quest") == 0)
        return (void*)3;
    if (strcmp(name, "Misc") == 0)
        return (void*)4;
    if (strcmp(name, "Font") == 0)
        return (void*)5;
    return nullptr;
}
static void ReadSettingsLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    switch (reinterpret_cast<uintptr_t>(entry))
    {
        case 1:
            {
                int value = 0;
                if (sscanf(line, "SortBy=%d", &value) == 1)
                {
                    if (value >= 0 && value <= 3)
                        menuTeleportSort = static_cast<TeleportSort>(value);
                }
                if (sscanf(line, "EnabledNearestTown=%d", &value) == 1)
                {
                    enabledNearestTown = (value != 0);
                }
                if (sscanf(line, "EnabledFavoriteTown=%d", &value) == 1)
                {
                    enabledFavoriteTown = (value != 0);
                }
                if (sscanf(line, "EnabledConvenientTown=%d", &value) == 1)
                {
                    enabledConvenientTown = (value != 0);
                }
                std::uint32_t a, b, c, d;
                if (sscanf(line, "FavoriteTown=%X,%X,%X,%X", &a, &b, &c, &d) == 4)
                {
                    favoriteTown.a = a;
                    favoriteTown.b = b;
                    favoriteTown.c = c;
                    favoriteTown.d = d;
                }
                break;
            }
        case 2:
            {
                int value = 0;
                if (sscanf(line, "EnabledEndlessDungeonNotifyPortal=%d", &value) == 1)
                {
                    enabledEndlessDungeonNotifyPortal = (value != 0);
                }
                float valueF;
                if (sscanf(line, "EndlessDungeonNotifyYPos=%f", &valueF) == 1)
                {
                    endlessDungeonNotifyYPos = valueF;
                }
                break;
            }
        case 3:
            {
                int value = 0;
                if (sscanf(line, "EnabledQuestTrackRestore=%d", &value) == 1)
                {
                    enabledQuestTrackRestore = (value != 0);
                }
                break;
            }
        case 4:
            {
                uint32_t value = 0;
                if (sscanf(line, "ToggleKey=%d", &value) == 1)
                {
                    g_toggleKey = static_cast<UINT>(value);
                }
                if (sscanf(line, "Language=%d", &value) == 1)
                {
                    if (value >= 0 && value < static_cast<uint32_t>(UiLanguages::END))
                        uiLanguage = static_cast<UiLanguages>(value);
                }
                break;
            }
        case 5:
            {
                if (strncmp(line, "FontName=", 9) == 0)
                {
                    std::string fontName = line + 9;
                    if (!fontName.empty())
                    {
                        for (auto& font : availableFonts)
                        {
                            if (font.name == fontName)
                            {
                                userFont = &font;
                                break;
                            }
                        }
                    }
                }
                uint32_t value = 0;
                if (sscanf(line, "FontSize=%d", &value) == 1)
                {
                    if (value >= 0 && value < 31)
                        fontSize = static_cast<int32_t>(value);
                }
                break;
            }
    }
}
static void WriteSettings(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf)
{
    out_buf->appendf("[%s][Teleport]\n", handler->TypeName);
    out_buf->appendf("SortBy=%d\n", static_cast<uint32_t>(menuTeleportSort));
    out_buf->appendf("EnabledNearestTown=%d\n", enabledNearestTown ? 1 : 0);
    out_buf->appendf("EnabledFavoriteTown=%d\n", enabledFavoriteTown ? 1 : 0);
    out_buf->appendf("EnabledConvenientTown=%d\n", enabledConvenientTown ? 1 : 0);
    out_buf->appendf("FavoriteTown=%08X,%08X,%08X,%08X\n", favoriteTown.a, favoriteTown.b, favoriteTown.c, favoriteTown.d);
    out_buf->appendf("\n[%s][EndlessDungeon]\n", handler->TypeName);
    out_buf->appendf("EnabledEndlessDungeonNotifyPortal=%d\n", enabledEndlessDungeonNotifyPortal ? 1 : 0);
    out_buf->appendf("EndlessDungeonNotifyYPos=%f\n", endlessDungeonNotifyYPos);
    out_buf->appendf("\n[%s][Quest]\n", handler->TypeName);
    out_buf->appendf("EnabledQuestTrackRestore=%d\n", enabledQuestTrackRestore ? 1 : 0);
    out_buf->appendf("\n[%s][Misc]\n", handler->TypeName);
    out_buf->appendf("ToggleKey=%d\n", static_cast<uint32_t>(g_toggleKey));
    out_buf->appendf("Language=%d\n", static_cast<uint32_t>(uiLanguage.load()));
    out_buf->appendf("\n[%s][Font]\n", handler->TypeName);
    out_buf->appendf("FontName=%s\n", userFont ? userFont->name.c_str() : "");
    out_buf->appendf("FontSize=%d\n", static_cast<uint32_t>(fontSize));
}

struct Teleports
{
    Vec3f worldPos;
    std::uint32_t order;
    std::string tag;
    std::string name;
};

static std::unordered_map<UID, Teleports, UIDHash> teleports =
{
    {{0x800B7FB7, 0xF6944F29, 0xB7F76950, 0x53E76CD7}, {{98.98, 6.94, 41.03}, 0, "tagRiftDevilsCrossing"}},
    {{0xCF9C3325, 0x388F42DF, 0x9CE8FEAA, 0x3F91E610}, {{-68.09, 3.23, -279.80}, 1, "tagRiftLowerCrossing"}},
    {{0x234EEDF3, 0x71994FD8, 0x8583A341, 0x52C2269A}, {{-160.11, 0.45, -644.13}, 2, "tagRiftWightmire"}},
    {{0xE3143058, 0xF2FB4436, 0x9ABF0269, 0xEE858BC1}, {{-244.49, 0.61, -858.56}, 3, "tagRiftBurrwitchRoad"}},
    {{0xE8A9452E, 0x0683463C, 0x87282B17, 0xBDED04A2}, {{863.21, 3.83, -1193.41}, 4, "tagRiftFloodedPassage"}},
    {{0xFB15683B, 0x82A5491C, 0xBE23DAA3, 0x8069C748}, {{-496.38, 10.54, -909.48}, 5, "tagRiftBurrwitchOutskirts"}},
    {{0x6E40DBDC, 0xCABE445E, 0x99A45A47, 0xA5D61C05}, {{-763.43, 0.92, -1201.93}, 6, "tagRiftBurrwitchVillage"}},
    {{0x93B12E9D, 0x3AB64A5A, 0xADC82BF8, 0x2E8851AF}, {{71.20, 0.30, -2182.05}, 7, "tagRiftWardensCellar"}},
    {{0xA270B781, 0xAF7C41EF, 0xA7A53CCC, 0x1F47FD49}, {{940.68, -0.33, -2381.65}, 8, "tagRiftWardensLabratory01"}},
    {{0x9E0983C2, 0xFE794998, 0x89B5310A, 0x0C02E86B}, {{979.00, -4.73, -2784.00}, 9, "tagRiftWardensLabratory02"}},
    {{0x36B8EA0A, 0x78504D33, 0xA2840CBA, 0x4007A1E4}, {{-429.53, 9.51, 268.74}, 10, "tagRiftArkovianFoothills"}},
    {{0x2906B19F, 0xE1634496, 0x9DBD4804, 0x64C27347}, {{-781.88, 24.41, 338.65}, 11, "tagRiftBrokenHills"}},
    {{0xB390FAC0, 0x725E432D, 0x83B4BF27, 0x206D493A}, {{-838.82, 11.78, 41.16}, 12, "tagRiftOldArkovia"}},
    {{0xBBBD726E, 0xAC2C489A, 0xB03E315E, 0x10AD555E}, {{-632.16, -0.28, 1333.73}, 13, "tagRiftCronleysHideout"}},
    {{0xD288B96B, 0x78254EA4, 0x801DD461, 0x06224595}, {{-1038.00, 10.50, 153.00}, 14, "tagRiftTwinFalls"}},
    {{0xDE4D3520, 0x792C4832, 0xB7B2ABEA, 0xF0B660AF}, {{-1104.95, 6.59, 1700.69}, 15, "tagRiftSmugglersPass"}},
    {{0xC2D9E1F2, 0x72C144E9, 0x94929B71, 0x92885F0B}, {{-1736.50, 6.47, 856.05}, 16, "tagRiftDeadmansGulch"}},
    {{0x7B66A856, 0x547F4D30, 0xA16EBED2, 0xD6E33372}, {{-1557.28, 11.69, 248.75}, 17, "tagRiftJaggedWaste"}},
    {{0x3D410313, 0x91AB4EE8, 0x81C5D234, 0x1AE247C9}, {{-1872.72, 29.01, 267.09}, 18, "tagRiftSmugglersRoad"}},
    {{0xEAC518F7, 0x29DE4C6B, 0x8692C54D, 0xE42840CC}, {{-2066.52, 12.22, 57.62}, 19, "tagRiftHomestead"}},
    {{0x3CE56564, 0xCA254BBC, 0x9C564F91, 0x74AB312F}, {{-2185.05, 11.47, 541.22}, 20, "tagRiftRottedCroplands"}},
    {{0x2E461CDC, 0x8B794AF2, 0xBCDDADED, 0x09FDC0C1}, {{-2449.93, 7.93, -19.28}, 21, "tagRiftSorrowsBastion"}},
    {{0x820D23A3, 0x0F934B0F, 0x9A42B8A4, 0xAA67ECCB}, {{-2905.10, 18.32, -187.28}, 22, "tagRiftBloodGrove"}},
    {{0x4796E7C6, 0xA0EC4F5B, 0x97FF2ADB, 0xD15B3C70}, {{-5314.42, -1.37, 727.51}, 23, "tagRiftDarkvale"}},
    {{0xEFCA31DB, 0x855D4C63, 0xBA7B8C2E, 0x2A00E877}, {{-3820.55, 86.84, 185.40}, 24, "tagRiftAlpineGate"}},
    {{0x8D288E68, 0x5AFB407D, 0xA8E45A2B, 0x92B9FD7E}, {{-4317.78, 82.09, 208.06}, 25, "tagRiftAlpineRoad"}},
    {{0xD584D2BB, 0x941C451F, 0x85F3E13D, 0x05D23CD2}, {{-4739.97, 20.25, 175.14}, 26, "tagRiftAlpineValley"}},
    {{0x47193FC3, 0x38274C8C, 0x940FECB6, 0xFE641565}, {{-4802.17, 22.71, -448.86}, 27, "tagRiftAlpineFort"}},
    {{0xCCD4C664, 0x095D4015, 0x8105C01B, 0xE21E0D0A}, {{-4811.23, 8.57, -1095.98}, 28, "tagRiftNecropolisGate"}},
    {{0x13C6F5D6, 0xA795463A, 0xAE6755A1, 0xC3DBFFE0}, {{-5175.43, 17.22, -1556.22}, 29, "tagRiftNecropolisInterior"}},
    {{0xEDFF66D2, 0x27B74F73, 0xA4F80FB6, 0x09EAADA1}, {{-916.70, 7.66, -1788.13}, 30, "tagGDX1RiftGloomwaldEntrance"}},
    {{0x2833B82F, 0x773D4BAB, 0x93EFC519, 0x63F8E761}, {{-1203.92, 3.07, -2207.71}, 31, "tagGDX1RiftGloomwaldRiverCrossing"}},
    {{0x350A3D97, 0x9FA04278, 0xBEFE8592, 0xFEF3BB38}, {{-857.36, 1.18, -2610.58}, 32, "tagGDX1RiftUgdenbogCoven"}},
    {{0xAB2BD6BB, 0xC4BE49FA, 0x9D57EED1, 0xAEE43244}, {{-1085.67, 10.67, -3150.83}, 33, "tagGDX1RiftUgdenbogCenter"}},
    {{0x51A84CB4, 0x560447B1, 0x92FFB15E, 0x31600E5F}, {{-1575.82, 6.43, -3365.65}, 34, "tagGDX1RiftUgdenbogVillage"}},
    {{0x5ECED1FB, 0x554B464E, 0x85E6019A, 0x11FBAE09}, {{-2007.85, 58.06, -3755.31}, 35, "tagGDX1RiftMalmouthAetherfire"}},
    {{0x93CF2182, 0xE1484135, 0xB0DD777D, 0x32C2F961}, {{-2566.30, 18.27, -4357.69}, 36, "tagGDX1RiftMalmouthOutskirts"}},
    {{0x8167A3FF, 0xCB5F45D3, 0xB38C526E, 0x6B4F5EB9}, {{862.50, 7.24, -4934.50}, 37, "tagGDX1RiftMalmouthSewers"}},
    {{0xDD3A1529, 0x57504654, 0xB894D3CF, 0xA04D492B}, {{-2752.78, 28.79, -4051.44}, 38, "tagGDX1RiftMalmouthIndustrial"}},
    {{0x7A4A37AB, 0xCCD84C84, 0xBE79BDB0, 0xA2097F13}, {{-3127.86, 28.59, -4224.99}, 39, "tagGDX1RiftMalmouthInner"}},
    {{0x54D3689C, 0xCF7E4177, 0x8AEF7330, 0x64D75673}, {{-3384.14, 55.10, 6970.72}, 40, "tagGDX2RiftWitchGodBase"}},
    {{0x80226FCF, 0x2F9946D3, 0x90C59887, 0x52D1D74A}, {{-3538.19, 3.78, 6606.87}, 41, "tagGDX2RiftKorvanPlateau"}},
    {{0x0A532129, 0xA8B84263, 0xB11ABB5B, 0xE3AE5FC2}, {{-3574.29, 21.75, 6226.57}, 42, "tagGDX2RiftTempleOfOsyr"}},
    {{0x63EF9B42, 0xCB324BB6, 0x9D265C82, 0xF46F20B8}, {{-4183.20, 5.41, 5898.99}, 43, "tagGDX2RiftKorvanSands"}},
    {{0xFD74D90D, 0x02B6413B, 0x8750D00C, 0xEB69948F}, {{-4040.88, 3.35, 5513.78}, 44, "tagGDX2RiftCairanDocks"}},
    {{0x26BBABF7, 0x80D442BC, 0x88388D15, 0xEB2DDE73}, {{-3703.20, 4.89, 5219.68}, 45, "tagGDX2RiftOasis"}},
    {{0xD00BFEEC, 0xBCF74E2F, 0x84298BB1, 0xC04B30EF}, {{-3524.79, 18.23, 4841.29}, 46, "tagGDX2RiftVanguard"}},
    {{0x86C4AEAA, 0x49674F8E, 0x959BCAF7, 0x6EEE0B5C}, {{-3591.34, 23.19, 4646.46}, 47, "tagGDX2RiftAbyd"}},
    {{0xF82B9423, 0xB41C4E45, 0xB0AE76DE, 0x75F0F057}, {{-3913.23, 47.45, 4682.33}, 48, "tagGDX2RiftInfernalWastes"}},
    {{0xA0B92E53, 0xC06B43AC, 0xACF4B685, 0xC0FDF362}, {{-4313.37, 42.61, 4601.34}, 49, "tagGDX2RiftKorvanCity"}},
    {{0x0F7BE2D1, 0xBB524C80, 0x9C5939D5, 0xE5ABE6E5}, {{-4806.98, 49.62, 3943.85}, 50, "tagGDX2RiftTemple"}},
    {{0xEF2F7579, 0x04DD4B30, 0xB2D968CC, 0x01B65CB4}, {{-5946.62, 12.01, 3837.64}, 51, "tagGDX2RiftEldritch"}},
    {{0x2C7C2CD9, 0xD7A144F3, 0xA909FE7C, 0x3ACB6C0A}, {{-3657.12, 4.92, 3357.21}, 52, "tagGDX2RiftLostOasis"}},
    {{0xB7D96D2F, 0xB7AA46C1, 0xB37E05D4, 0x22ECC54A}, {{-5422.78, 37.76, -781.45}, 53, "tagGDX3RiftHunterCamp"}},
    {{0x0EDAF065, 0xCE0A4570, 0x9D94454C, 0xE2FAC7F1}, {{-5681.91, 80.48, -1167.20}, 54, "tagGDX3RiftFrozenWastes"}},
    {{0x80A20B41, 0xAF5D4368, 0xB75FAC95, 0x1C2D95F4}, {{-6431.15, 91.80, -1220.74}, 55, "tagGDX3RiftIcetunnel"}},
    {{0xC44C7F26, 0xF2EB4BCF, 0x8EE608AE, 0x54C54617}, {{-7062.17, 33.57, -509.14}, 56, "tagGDX3RiftVerdantValley"}},
    {{0x815EFF1A, 0x184040FF, 0x8A97A4A6, 0x818BAFED}, {{-7539.52, 22.68, -911.40}, 57, "tagGDX3RiftKurnhold"}},
    {{0x6A7B991C, 0xF03B4589, 0xBAB4F354, 0x4EF0418B}, {{-7419.45, 32.62, -1533.88}, 58, "tagGDX3RiftVerdantValley02"}},
    {{0x7AC62E0E, 0xFE7C45A8, 0x8A68848B, 0x3EB6843C}, {{-7677.06, 64.12, -1908.07}, 59, "tagGDX3RiftGuardian"}},
    {{0x1934D4DA, 0xB0D74E4B, 0xAFB261FF, 0x59A6BDA2}, {{-8228.05, 74.74, -2192.88}, 60, "tagGDX3RiftHotSprings"}},
    {{0x5C36523D, 0x34E94AFF, 0x9619901B, 0x7F7C644D}, {{-8680.70, 80.11, -2058.00}, 61, "tagGDX3RiftRuinedVillage"}},
    {{0x770CB845, 0xEBC84E71, 0x9EF5362F, 0xC440EFDC}, {{-8942.59, 44.14, -1996.26}, 62, "tagGDX3RiftDreadWastes"}},
    {{0xAE81578E, 0xC7F040F0, 0x91CE00EA, 0x5CF05370}, {{-9318.09, 38.56, -1998.12}, 63, "tagGDX3RiftBlackCitadel"}},
    {{0x40D2922C, 0x52664CA2, 0x94A7C614, 0x7927864D}, {{-8380.93, 26.07, -2844.37}, 64, "tagGDX3RiftGlacialGrotto"}},
    {{0x7F2CDC21, 0x3C61475B, 0xB074EF01, 0x4C4C7700}, {{-9023.97, 61.90, -3223.48}, 65, "tagGDX3RiftPilgrimRest"}},
    {{0xA2F495DB, 0x28B64C06, 0x92CF0244, 0x4C441E21}, {{-8721.95, 53.80, -3044.20}, 66, "tagGDX3RiftGlacierEdge"}},
    {{0x4B3B4457, 0xF1A64E0F, 0x916690FE, 0x2695E194}, {{-9092.58, 102.55, -3330.52}, 67, "tagGDX3RiftMountainBase"}},
    {{0x27425CC4, 0x4FD840F5, 0x9E34E366, 0x3C1CEB94}, {{-9053.30, 88.07, -3761.10}, 68, "tagGDX3RiftBitterWindPeak"}},
    {{0x2A3514F1, 0x4F3C4BF3, 0x8A503B84, 0xC337CD3A}, {{-9049.57, 61.58, -5063.82}, 69, "tagGDX3RiftAuroraPeaks"}},
    {{0x123C5DD7, 0x69AD4110, 0xBFEB7681, 0x9BB24006}, {{-9137.35, 66.57, -5243.58}, 70, "tagGDX3RiftRoofOfTheWorld"}},
    {{0x2024F9A2, 0x4F6F4904, 0x927F648F, 0x16D51C38}, {{-7586.41, 93.81, -2374.95}, 71, "tagGDX3RiftDranghoulLowlands"}},
};

struct Towns
{
    Vec3f worldPos;
    std::uint32_t order;
    std::string tag;
    std::string name;
};

static std::unordered_map<UID, Towns, UIDHash> towns =
{
    {{0x800B7FB7, 0xF6944F29, 0xB7F76950, 0x53E76CD7}, {{42.151, 9.050, 64.729}, 0, "tagMapDevilsCrossing01"}},
    {{0xEAC518F7, 0x29DE4C6B, 0x8692C54D, 0xE42840CC}, {{-2119.792, 19.800, 54.285}, 19, "tagMapHomestead"}},
    {{0x47193FC3, 0x38274C8C, 0x940FECB6, 0xFE641565}, {{-4827.574, 39.050, -498.739}, 27, "tagMapFortIkon"}},
    {{0x350A3D97, 0x9FA04278, 0xBEFE8592, 0xFEF3BB38}, {{-816.800, 0.594, -2606.041}, 32, "tagGDX1MapUgdenbogCoven"}},
    {{0x8167A3FF, 0xCB5F45D3, 0xB38C526E, 0x6B4F5EB9}, {{893.490, 7.490, -5018.916}, 37, "tagGDX1UGMalmouthHideout01"}},
    {{0xDD3A1529, 0x57504654, 0xB894D3CF, 0xA04D492B}, {{-2752.78, 28.79, -4051.44}, 38, "tagGDX1RiftMalmouthIndustrial"}},
    {{0x54D3689C, 0xCF7E4177, 0x8AEF7330, 0x64D75673}, {{-3390.885, 54.099, 6937.304}, 40, "tagGDX2MapWitchGodBase"}},
    {{0x815EFF1A, 0x184040FF, 0x8A97A4A6, 0x818BAFED}, {{-7506.952, 35.800, -988.155}, 57, "tagGDX3MapKurnhold"}},
};

Vec3f GetTownPos(Towns const& town)
{
    if(!enabledConvenientTown)
    {
        for (const auto& [uid, value] : towns)
        {
            if (&value == &town)
                return teleports.at(uid).worldPos;
        }
    }
    return town.worldPos;
}

std::string GetKeyName(UINT vk)
{
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    switch (vk)
    {
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            scanCode |= 0x100;
            break;
    }
    char name[128]{};
    if (GetKeyNameTextA(static_cast<LONG>(scanCode << 16), name, sizeof(name)))
    {
        return name;
    }
    return "???";
}
bool AnyKeyDown()
{
    for (UINT vk = 1; vk <= 0xFE; ++vk)
    {
        if (GetAsyncKeyState(vk) & 0x8000)
            return true;
    }
    return false;
}
UINT GetPressedKey()
{
    for (UINT vk = 1; vk <= 0xFE; ++vk)
    {
        // Ignore mouse buttons
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2)
        {
            continue;
        }
        if (GetAsyncKeyState(vk) & 0x8000)
            return vk;
    }
    return 0;
}

#define GAME_FN(name, type, symbol)          \
    inline type name()                             \
{                                              \
    static type fn = nullptr; \
    if (!fn) \
    fn = (type)(g_GameAddresses.at(symbol)); \
    return fn; \
}
#define ENGINE_FN(name, type, symbol)          \
    inline type name()                             \
{                                              \
    static type fn = nullptr; \
    if (!fn) \
    fn = (type)(g_EngineAddresses.at(symbol)); \
    return fn; \
}

struct WorldCoords
{
    void* regionData; // 0x00
    float x; // 0x08
    float y; // 0x0C
    float z; // 0x10
    float unk1; // 0x14
    float basis0[4]; // 0x18
    float basis1[4]; // 0x28
    float basis2[4]; // 0x38
};

std::unordered_map<std::uint32_t, WorldCoords> playerRifts;
std::mutex playerRiftsMutex;

static HMODULE GameModule()
{
    static HMODULE mod = nullptr;
    if (!mod)
        mod = GetModuleHandleA("Game.dll");
    return mod;
}

static HMODULE EngineModule()
{
    static HMODULE mod = nullptr;
    if (!mod)
        mod = GetModuleHandleA("Engine.dll");
    return mod;
}

void* GetGameEnginePtr()
{
    static void** gGameEngine = nullptr;
    if (!gGameEngine)
    {
        gGameEngine = (void**)(g_GameAddresses.at("GameEngine"));
    }
    if (!gGameEngine)
        return nullptr;
    return *gGameEngine;
}

struct GameString
{
    union
    {
        char buffer[16];
        char* ptr;
    };

    std::uint64_t size;
    std::uint64_t capacity;
    const char* c_str() const
    {
        return capacity >= 16 ? ptr : buffer;
    }
};

struct GameWString
{
    union
    {
        uint16_t buffer[8];
        uint16_t* ptr;
    };
    std::uint64_t size;
    std::uint64_t capacity;
    const uint16_t* c_str() const
    {
        return capacity >= 8 ? ptr : buffer;
    }
};
std::string WStringToUtf8(const GameWString& str)
{
    if (str.size == 0)
        return {};
    const uint16_t* data = str.c_str();
    if (!data)
        return {};
    const int inputLength = static_cast<int32_t>(str.size);
    int outputSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reinterpret_cast<const wchar_t*>(data), inputLength, nullptr, 0, nullptr, nullptr);
    if (outputSize <= 0)
        return {};
    std::string result(outputSize, '\0');
    int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reinterpret_cast<const wchar_t*>(data), inputLength, result.data(), outputSize, nullptr, nullptr);
    if (written <= 0)
        return {};
    result.resize(written);
    return result;
}
std::string WStringToUtf8(const wchar_t* wstr)
{
    if (!wstr)
        return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string result(size, '\0');
    int written = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size, nullptr, nullptr);
    if (written <= 0)
        return {};
    result.resize(written - 1); // Exclude null terminator
    return result;
}
std::string WStringToUtf8(const uint16_t* data)
{
    if (!data)
        return {};
    return WStringToUtf8(reinterpret_cast<const wchar_t*>(data));
}
std::string WStringToUtf8(const std::wstring& wstr)
{
    return WStringToUtf8(wstr.c_str());
}

using GetWorldPosition_t = Vec3f* (__fastcall*)(const WorldCoords* worldCoords, Vec3f* out);
ENGINE_FN(GetWorldPositionFn, GetWorldPosition_t, "sym.Engine.dll__GetWorldPosition_WorldVec3_GAME__QEBA_AVVec3_2_XZ");
Vec3f GetWorldPosition(const WorldCoords& worldCoords)
{
    static auto fn = GetWorldPositionFn();
    if (!fn)
        return {};
    Vec3f worldPos;
    fn(&worldCoords, &worldPos);
    return worldPos;
}

using MainPlayerCanUsePersonalTeleport_t = bool(__fastcall*)(void* gameEngine);
GAME_FN(GetMainPlayerCanUsePersonalTeleportFn, MainPlayerCanUsePersonalTeleport_t, "sym.Game.dll__MainPlayerCanUsePersonalTeleport_GameEngine_GAME__QEBA_NXZ");
bool MainPlayerCanUsePersonalTeleport()
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return false;
    static auto fn = GetMainPlayerCanUsePersonalTeleportFn();
    if (!fn)
        return false;
    return fn(engine);
}

using CreateFixedItemTeleport_t = void(__fastcall*)(void* pThis);
GAME_FN(GetCreateFixedItemTeleportFn, CreateFixedItemTeleport_t, "sym.Game.dll__CreateFixedItemTeleport_GameEngine_GAME__QEAAXXZ");
bool CreateFixedItemTeleport()
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return false;
    static auto fn = GetCreateFixedItemTeleportFn();
    if (!fn)
        return false;
    fn(engine);
    return true;
}

using InitiateTeleport_t = void(__fastcall*)(void* engine, int x, int y, int z, int effect, bool playEffect);
GAME_FN(GetInitiateTeleportFn, InitiateTeleport_t, "sym.Game.dll__InitiatePlayerTeleport_GameEngine_GAME__QEAAXHHHW4TeleportEffect_2__N_Z");
bool InitiateTeleport(const Vec3f& pos)
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return false;
    static auto fn = GetInitiateTeleportFn();
    if (!fn)
        return false;
    Log("Teleporting to %u %u %u", pos.x, pos.y, pos.z);
    fn(engine, pos.x, pos.y, pos.z, 1, true);
    return true;
}

using GetPlayerManagerClient_t = void* (__fastcall*)(void*);
GAME_FN(GetPlayerManagerClientFn, GetPlayerManagerClient_t, "sym.Game.dll__GetPlayerManagerClient_GameEngine_GAME__QEBAPEAVPlayerManagerClient_2_XZ");
void* GetPlayerManagerClient()
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return {};
    static auto fn = GetPlayerManagerClientFn();
    if (!fn)
        return {};
    return fn(engine);
}

using GetAllPlayersInGame_t = std::vector<std::uint32_t>&(__fastcall*)(void*);
GAME_FN(GetAllPlayersInGameFn, GetAllPlayersInGame_t, "sym.Game.dll__GetAllPlayersInGame_PlayerManagerClient_GAME__QEBAAEBV__vector_I_mem__XZ");
std::vector<std::uint32_t> GetAllPlayersInGame()
{
    static auto pm = GetPlayerManagerClient();
    if (!pm)
        return {};
    static auto fn = GetAllPlayersInGameFn();
    if (!fn)
        return {};
    return fn(pm);
}

using GetPlayerName_t = GameWString* (__fastcall*)(void* pm, GameWString* out, std::uint32_t id);
GAME_FN(GetPlayerNameFn, GetPlayerName_t, "sym.Game.dll__GetPlayerName_PlayerManagerClient_GAME__QEBA_AV__basic_string_GU__char_traits_G_std__V__allocator_G_2__std__I_Z");
std::string GetPlayerName(std::uint32_t id)
{
    static auto pm = GetPlayerManagerClient();
    if (!pm)
        return {};
    static auto fn = GetPlayerNameFn();
    if (!fn)
        return {};
    GameWString name{};
    fn(pm, &name, id);
    return WStringToUtf8(name);
}

using GetGameDifficulty_t = int(__fastcall*)(void* gameEngine);
GAME_FN(GetGameDifficultyFn, GetGameDifficulty_t, "sym.Game.dll__GetGameDifficulty_GameEngine_GAME__QEBA_AW4GameDifficulty_2_XZ");
uint8_t GetGameDifficulty()
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return 0;
    static auto fn = GetGameDifficultyFn();
    if (!fn)
        return 0;
    return fn(engine);
}

using GetMainPlayer_t = void* (__fastcall*)(void*);
GAME_FN(GetMainPlayerFn, GetMainPlayer_t, "sym.Game.dll__GetMainPlayer_GameEngine_GAME__QEBAPEAVPlayer_2_XZ");
void* GetMainPlayer()
{
    static auto engine = GetGameEnginePtr();
    if (!engine)
        return nullptr;
    static auto fn = GetMainPlayerFn();
    if (!fn)
        return nullptr;
    return fn(engine);
}

using GetPlayerNamePlayer_t = const uint16_t* (__fastcall*)(void*);
GAME_FN(GetPlayerNamePlayerFn, GetPlayerNamePlayer_t, "sym.Game.dll__GetPlayerName_Player_GAME__QEBAPEBGXZ");
std::string GetMainPlayerName()
{
    static auto fn = GetPlayerNamePlayerFn();
    if (!fn)
        return {};
    auto mainPlayer = GetMainPlayer();
    if(!mainPlayer)
        return {};
    auto name = fn(mainPlayer);
    if (!name)
        return {};
    return WStringToUtf8(name);
}

struct VecTeleportUIDs
{
    UID* first;
    UID* last;
    UID* capacity;
    UID* begin() const { return first; }
    UID* end() const { return last; }
    size_t size() const { return last - first; }
};
using GetTeleportUIDs_t = VecTeleportUIDs* (__fastcall*)(void* player);
GAME_FN(GetTeleportUIDsFn, GetTeleportUIDs_t, "sym.Game.dll__GetTeleportUIDs_Player_GAME__QEBAAEBV__vector_VUniqueId_GAME___mem__XZ");
VecTeleportUIDs GetTeleportUIDs()
{
    auto mainPlayer = GetMainPlayer();
    if (!mainPlayer)
        return {};
    static auto fn = GetTeleportUIDsFn();
    if (!fn)
        return {};
    auto vec = fn(mainPlayer);
    if (!vec)
        return {};
    return *vec;
}

using GetFootCoords_t = WorldCoords* (__fastcall*)(void* player, WorldCoords* out, bool snap);
GAME_FN(GetFootCoordsFn, GetFootCoords_t, "sym.Game.dll__GetFootCoords_Player_GAME__UEAA_AVWorldCoords_2__N_Z");
WorldCoords GetFootCoords()
{
    auto mainPlayer = GetMainPlayer();
    if (!mainPlayer)
        return {};
    static auto fn = GetFootCoordsFn();
    if (!fn)
        return {};
    WorldCoords worldCoords;
    fn(mainPlayer, &worldCoords, true);
    return worldCoords;
}

static const FontInfo* FindFont(const std::vector<FontInfo>& fonts, const char* preferredName, bool FontInfo::*languageFlag)
{
    if(!preferredName || !languageFlag)
        return nullptr;

    for (const auto& font : fonts)
        if (font.name == preferredName && font.*languageFlag)
            return &font;

    for (const auto& font : fonts)
        if (font.*languageFlag)
            return &font;

    return nullptr;
}
bool FileExists(const char* path)
{
    return std::filesystem::is_regular_file(path);
}
void LoadImGuiFont(const FontInfo* selectedFont, const std::vector<FontInfo>& fonts)
{
    ImGui_ImplDX11_InvalidateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();
    if (!io.Fonts)
        return;
    io.Fonts->Clear();

    ImFontConfig defaultCfg{};
    defaultCfg.OversampleH = 0;
    defaultCfg.OversampleV = 0;
    defaultCfg.PixelSnapH = true;
    defaultCfg.SizePixels = static_cast<float>(fontSize);

    ImFont* font = nullptr;
    ImFontConfig cfg = defaultCfg;
    if (selectedFont && FileExists(WStringToUtf8(selectedFont->path).c_str()))
    {
        font = io.Fonts->AddFontFromFileTTF(WStringToUtf8(selectedFont->path).c_str(), 0.0f, &cfg);
    }
    else
    {
        io.FontDefault = io.Fonts->AddFontDefaultVector(&defaultCfg);
        font = io.FontDefault;
    }

    if (!font)
    {
        Log("Failed to load font: %s", selectedFont ? selectedFont->name.c_str() : "Default");
        io.FontDefault = io.Fonts->AddFontDefaultVector(&defaultCfg);
        io.Fonts->Build();
        ImGui_ImplDX11_CreateDeviceObjects();
        return;
    }

    cfg.MergeMode = true;
    if (!selectedFont || !selectedFont->vietnamese)
    {
        const FontInfo* fallback = FindFont(fonts, "Malgun Gothic", &FontInfo::vietnamese);
        if (fallback && FileExists(WStringToUtf8(fallback->path).c_str()))
            font = io.Fonts->AddFontFromFileTTF(WStringToUtf8(fallback->path).c_str(), 0.0f, &cfg);
    }
    if (!selectedFont || !selectedFont->korean)
    {
        const FontInfo* fallback = FindFont(fonts, "Malgun Gothic", &FontInfo::korean);
        if (fallback && FileExists(WStringToUtf8(fallback->path).c_str()))
            font = io.Fonts->AddFontFromFileTTF(WStringToUtf8(fallback->path).c_str(), 0.0f, &cfg);
    }
    if (!selectedFont || !selectedFont->japanese)
    {
        const FontInfo* fallback = FindFont(fonts, "Malgun Gothic", &FontInfo::japanese);
        if (fallback && FileExists(WStringToUtf8(fallback->path).c_str()))
            font = io.Fonts->AddFontFromFileTTF(WStringToUtf8(fallback->path).c_str(), 0.0f, &cfg);
    }
    if (!selectedFont || !selectedFont->chinese)
    {
        const FontInfo* fallback = FindFont(fonts, "Microsoft Yahei", &FontInfo::chinese);
        if (fallback && FileExists(WStringToUtf8(fallback->path).c_str()))
            font = io.Fonts->AddFontFromFileTTF(WStringToUtf8(fallback->path).c_str(), 0.0f, &cfg);
    }

    ImGui::GetStyle().FontSizeBase = static_cast<float>(fontSize);
    io.Fonts->Build();
    io.FontDefault = font;
    ImGui_ImplDX11_CreateDeviceObjects();
    Log("Loaded font: %s, size: %u", selectedFont ? selectedFont->name.c_str() : "Default ProggyClean", fontSize);
}

using LocalizationInstance_t = void* (__fastcall*)();
ENGINE_FN(LocalizationInstanceFn, LocalizationInstance_t, "sym.Engine.dll__Instance_LocalizationManager_GAME__SAAEAV12_XZ")
using GetCurrentLanguage_t = std::uint32_t(__fastcall*)(void* localizationManager);
ENGINE_FN(GetCurrentLanguageFn, GetCurrentLanguage_t, "sym.Engine.dll__GetCurrentLanguage_LocalizationManager_GAME__QEAAIXZ")
using GetLanguageName_t = const char* (__fastcall*)(void*, std::uint32_t);
ENGINE_FN(GetLanguageNameFn, GetLanguageName_t, "sym.Engine.dll__GetLanguageName_LocalizationManager_GAME__QEBAAEBV__basic_string_DU__char_traits_D_std__V__allocator_D_2__std__W4Language_2__Z")
using Localize_t = const uint16_t* (__fastcall*)(void* thisptr, const char* key);
ENGINE_FN(LocalizeFn, Localize_t, "sym.Engine.dll__Localize_LocalizationManager_GAME__QEAAPEBGPEBDZZ")
void* GetLocalizationManager()
{
    static auto localizationManagerFn = LocalizationInstanceFn();
    if (!localizationManagerFn)
        return {};
    static auto localizationManager = localizationManagerFn();
    return localizationManager;
}
std::string GetLanguageName()
{
    static auto localizationManager = GetLocalizationManager();
    if (!localizationManager)
        return {};
    static auto getCurrentLanguageFn = GetCurrentLanguageFn();
    if (!getCurrentLanguageFn)
        return {};
    auto langId = getCurrentLanguageFn(localizationManager);
    static auto getLanguageNameFn = GetLanguageNameFn();
    if (!getLanguageNameFn)
        return {};
    auto lang = getLanguageNameFn(localizationManager, langId);
    return lang;
}
std::string trim(std::string_view s)
{
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos)
        return {};
    auto end = s.find_last_not_of(" \t\n\r");
    return std::string(s.substr(start, end - start + 1));
}
std::string Localize(const std::string& tag)
{
    static auto localizationManager = GetLocalizationManager();
    if (!localizationManager)
        return {};
    static auto localizeFn = LocalizeFn();
    if (!localizeFn)
        return {};
    auto name = localizeFn(localizationManager, tag.c_str());
    if (!name)
        return {};
    return trim(WStringToUtf8(name));
}
void loadLanguage(std::string name)
{
    Log("Loading language: %s", name.c_str());
    for (auto& item : teleports)
    {
        auto localized = Localize(item.second.tag);
        if (name == "English")
            if (localized.ends_with(" Rift"))
                localized.erase(localized.size() - 5);
        item.second.name = localized;
    }
    for (auto& item : towns)
    {
        item.second.name = Localize(item.second.tag);
    }

    l.Load(name);
}

using ReloadLanguage_t = void(__fastcall*)(void* localizationManager, const char* language);
ReloadLanguage_t oReloadLanguage;
void __fastcall hkReloadLanguage(void* localizationManager, const char* language)
{
    oReloadLanguage(localizationManager, language);
    if (uiLanguage == UiLanguages::Automatic && imguiInitialized)
    {
        Log("Game language changed");
        loadLanguage(language);
    }
}

using CreateFixedItemTeleportNetHook_t =
void(__fastcall*)(void* thisptr, const WorldCoords& coords, std::uint32_t playerId, std::uint32_t a3, GameString& name);
CreateFixedItemTeleportNetHook_t oCreateFixedItemTeleportNetHook = nullptr;
void __fastcall hkCreateFixedItemTeleportNetHook(void* thisptr, const WorldCoords& coords, std::uint32_t playerId, std::uint32_t a3, GameString& name)
{
    Log("CreateFixedItemTeleportNetHook local: %.2f %.2f %.2f playerId: %u", coords.x, coords.y, coords.z, playerId);
    {
        std::lock_guard<std::mutex> lock(playerRiftsMutex);
        playerRifts[playerId] = coords;
    }
    oCreateFixedItemTeleportNetHook(thisptr, coords, playerId, a3, name);
}

struct Player
{
    std::uint32_t id;
    std::string name;
};

std::vector<Player> GetPlayerNames()
{
    std::vector<Player> players;
    auto ps = GetAllPlayersInGame();
    players.reserve(ps.size());
    for (auto & id : ps)
    {
        players.emplace_back(Player{id, GetPlayerName(id)});
    }
    return players;
}

bool IsReadableAddress(const void* ptr, SIZE_T size)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
        return false;
    if (mbi.State != MEM_COMMIT)
        return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;
    const auto start = reinterpret_cast<std::uintptr_t>(ptr);
    const auto end = start + size;
    const auto regionEnd = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return end <= regionEnd;
}


using GetDescriptionTag_t = void* (__fastcall*)(void* actor);
ENGINE_FN(GetDescriptionTagFn, GetDescriptionTag_t, "sym.Engine.dll__GetDescriptionTag_Actor_GAME__QEBAPEBDXZ")
using GetUniqueID_t = void* (__fastcall*)(void* entity);
ENGINE_FN(GetUniqueIDFn, GetUniqueID_t, "sym.Engine.dll__GetUniqueID_Entity_GAME__QEAAAEBVUniqueId_2_XZ")
using GetCoordsEntity_t = void* (__fastcall*)(void* entity, void* out);
ENGINE_FN(GetCoordsEntityFn, GetCoordsEntity_t, "sym.Engine.dll__GetCoords_Entity_GAME__QEBA_AVWorldCoords_2_XZ")
using ActorAttach_t = void(__fastcall*)(void* actor, void* entity, void* coords, void* name, bool a, bool b, bool c, bool d);
ActorAttach_t oActorAttach = nullptr;
void __fastcall hkActorAttach(void* actor, void* entity, void* coords, void* name, bool a, bool b, bool c, bool d)
{
    oActorAttach(actor, entity, coords, name, a, b, c, d);

    if (actor)
    {
        auto addr = reinterpret_cast<std::uintptr_t>(actor) + 8;
        if(!IsReadableAddress(reinterpret_cast<void*>(addr), sizeof(char*)))
            return;
        char* name = *reinterpret_cast<char**>(addr);
        if(!IsReadableAddress(name, 1))
            return;
        static std::uint32_t counter = 0;
        if (name && strstr(name, "riftgatemap"))
        {
            static auto getUniqueId = GetUniqueIDFn();
            if(!getUniqueId)
                return;
            UID* uid = static_cast<UID*>(getUniqueId(actor));
            if(!uid)
                return;
            static auto getDescriptionTag = GetDescriptionTagFn();
            if(!getDescriptionTag)
                return;
            const char* tag = static_cast<const char*>(getDescriptionTag(actor));
            if(!tag)
                return;
            static auto getCoordsEntity = GetCoordsEntityFn();
            if(!getCoordsEntity)
                return;
            WorldCoords wc;
            getCoordsEntity(actor, &wc);
            Vec3f worldCoords = GetWorldPosition(wc);
            Log("Riftgate UID={{0x%08X, 0x%08X, 0x%08X, 0x%08X}, {{%.2f, %.2f, %.2f}, %u, \"%s\"}}, dbr=%s", uid->a, uid->b, uid->c, uid->d, worldCoords.x, worldCoords.y, worldCoords.z, counter/2, tag, name);
            counter++;
        }
    }
}

using InEndlessDungeon_t = bool (__fastcall*)(void* character);
GAME_FN(InEndlessDungeonFn, InEndlessDungeon_t, "sym.Game.dll__InEndlessDungeon_Character_GAME__QEBA_NXZ");
bool IsInEndlessDungeon()
{
    static auto fn = InEndlessDungeonFn();
    if(!fn)
        return false;
    auto mainPlayer = GetMainPlayer();
    if(!mainPlayer)
        return false;
    return fn(mainPlayer);
}

using RTTI_new_FixedItemDungeonTeleport_t = void* (__fastcall*)();
RTTI_new_FixedItemDungeonTeleport_t oRTTI_new_FixedItemDungeonTeleport = nullptr;
void* __fastcall hkRTTI_new_FixedItemDungeonTeleport()
{
    if(IsInEndlessDungeon() && isEndlessDungeonComplete)
    {
        showEndlessDungeonNotify = true;
    }
    else
    {
        showEndlessDungeonNotify = false;
        isEndlessDungeonComplete = false;
        isEndlessDungeonNew = false;
    }
    return oRTTI_new_FixedItemDungeonTeleport();
}

using SyncDungeonProgress_t = void (*)(void* gameEngine, unsigned int);
SyncDungeonProgress_t oSyncDungeonProgress = nullptr;
void hkSyncDungeonProgress(void* gameEngine, unsigned int arg)
{
    if(IsInEndlessDungeon() && !isEndlessDungeonNew)
    {
        isEndlessDungeonComplete = true;
    }
    else
    {
        showEndlessDungeonNotify = false;
        isEndlessDungeonComplete = false;
        isEndlessDungeonNew = false;
    }
    oSyncDungeonProgress(gameEngine, arg);
}

using ClearEndlessDungeonGenerator_t = void(__fastcall*)(void* _this);
ClearEndlessDungeonGenerator_t oClearEndlessDungeonGenerator = nullptr;
void __fastcall hkClearEndlessDungeonGenerator(void* _this)
{
    showEndlessDungeonNotify = false;
    isEndlessDungeonComplete = false;
    isEndlessDungeonNew = true;
    oClearEndlessDungeonGenerator(_this);
}


using RequestToUse_FixedItemDungeonTeleportFn = void(*)(void* _this, std::uint32_t itemId);
RequestToUse_FixedItemDungeonTeleportFn oRequestToUse_FixedItemDungeonTeleport = nullptr;
void hkRequestToUse_FixedItemDungeonTeleport(void* _this, std::uint32_t itemId)
{
    showEndlessDungeonNotify = false;
    isEndlessDungeonComplete = false;
    isEndlessDungeonNew = false;
    oRequestToUse_FixedItemDungeonTeleport(_this, itemId);
}

using GetQuest2Repository_t = void* (__fastcall*)();
GAME_FN(GetQuest2RepositoryFn, GetQuest2Repository_t, "sym.Game.dll__Get___Singleton_VQuest2Repository_GAME___GAME__SAPEAVQuest2Repository_2_XZ");
void* GetQuest2Repository()
{
    static auto fn = GetQuest2RepositoryFn();
    if (!fn)
        return nullptr;
    return fn();
}

enum QuestFilter : std::uint32_t
{
    QuestFilter_All  = 0,
    QuestFilter_Active = 1,
    QuestFilter_Completed = 2,
    QuestFilter_ActiveAndTracked = 4,
    QuestFilter_Flag8 = 8,
};
struct VecQuests
{
    void** first;
    void** last;
    void** capacity;
    void** begin() const { return first; }
    void** end() const { return last; }
    size_t size() const { return last - first; }
};
using GetQuests_t = void(__fastcall*)(void* pQuestRepository, void* pVecQuests, QuestFilter filter);
GAME_FN(GetQuestsFn, GetQuests_t, "sym.Game.dll__GetQuests_Quest2Repository_GAME__QEAAXAEAV__vector_PEAVQuest2_GAME___mem__W4Filter_12__Z");
VecQuests GetQuests(QuestFilter filter)
{
    static auto quest2Repository = GetQuest2Repository();
    if (!quest2Repository)
        return {};
    static auto fn = GetQuestsFn();
    if (!fn)
        return {};
    VecQuests quests{};
    fn(quest2Repository, &quests, filter);
    return quests;
}

using IsTrackedQuest_t = bool(__fastcall*)(void* quest);
GAME_FN(IsTrackedQuestFn, IsTrackedQuest_t, "sym.Game.dll__IsTracked_Quest2_GAME__QEBA_NXZ");
bool IsTrackedQuest(void* quest)
{
    static auto fn = IsTrackedQuestFn();
    if (!fn)
        return false;
    return fn(quest);
}

using SetTrackedQuest_t = void(__fastcall*)(void*, bool);
GAME_FN(SetTrackedQuestFn, SetTrackedQuest_t, "sym.Game.dll__SetTracked_Quest2_GAME__QEAAX_N_Z");
bool SetTrackedQuest(void* quest, bool tracked)
{
    static auto fn = SetTrackedQuestFn();
    if (!fn)
        return false;
    fn(quest, tracked);
    return true;
}
void (__fastcall* oSetTracked)(void* self, bool tracked) = nullptr;
void __fastcall hkSetTracked(void* self, bool tracked)
{
    if(enabledQuestTrackRestore)
    {
        std::uint32_t questId = *static_cast<std::uint32_t*>(self);
        std::string name = GetMainPlayerName();
        if(!name.empty())
        {
            auto& untrackedQuests = currentSaveData.untrackedQuests;
            if(tracked && untrackedQuests.contains(questId))
            {
                untrackedQuests.erase(questId);
                Log("SAVING");
                SaveSystem::SaveDifficulty(name, GetGameDifficulty(), currentSaveData);
            }
            else if (!tracked && !untrackedQuests.contains(questId))
            {
                untrackedQuests.insert(questId);
                Log("SAVING");
                SaveSystem::SaveDifficulty(name, GetGameDifficulty(), currentSaveData);
            }
        }
        Log("SetTracked(%d) id 0x%08X - %p", tracked, questId, self);
    }

    oSetTracked(self, tracked);
}

using GetEventManager_t = void* (__fastcall*)();
ENGINE_FN(GetEventManagerFn, GetEventManager_t, "sym.Engine.dll__Get___Singleton_VEventManager_GAME___GAME__SAPEAVEventManager_2_XZ");
void* GetEventManager()
{
    static auto eventManager = GetEventManagerFn();
    if (!eventManager)
        return nullptr;
    return eventManager();
}

using SendEvent_t = void(__fastcall*)(void*, void*, std::uint32_t);
ENGINE_FN(SendEventFn, SendEvent_t, "sym.Engine.dll__Send_EventManager_GAME__QEAAXPEBUGameEvent_2_I_Z");
struct GameEvent
{
    std::uint32_t id;
    std::int32_t a;
    std::int32_t b;
    std::uint16_t c;
};
bool SendEvent(GameEvent& event)
{
    static auto eventManager = GetEventManager();
    if (!eventManager)
        return false;
    static auto fn = SendEventFn();
    if (!fn)
        return false;
    fn(eventManager, &event, event.id);
    return true;
}

using PlayerManagerSetMainPlayer_t = void(__fastcall*)(void* manager, std::uint32_t playerId);
PlayerManagerSetMainPlayer_t oPlayerManagerSetMainPlayer = nullptr;
void __fastcall hkPlayerManagerSetMainPlayer(void* manager, std::uint32_t playerId)
{
    Log("SetMainPlayer manager %p, playerId %u (0x%X)", manager, playerId, playerId);
    oPlayerManagerSetMainPlayer(manager, playerId);

    if(enabledQuestTrackRestore)
    {
        VecQuests quests = GetQuests(QuestFilter::QuestFilter_Active);
        bool changed = false;
        std::string name = GetMainPlayerName();
        if(!name.empty())
        {
            currentSaveData = SaveSystem::LoadDifficulty(name, GetGameDifficulty());
            auto& untrackedQuests = currentSaveData.untrackedQuests;
            std::unordered_set<uint32_t> questIds;
            for (void* quest : quests)
            {
                questIds.insert(*static_cast<uint32_t*>(quest));
            }
            size_t erased = std::erase_if(untrackedQuests, [&](std::uint32_t id)
                    {
                    return !questIds.contains(id);
                    });
            if (erased > 0)
            {
                Log("SAVING");
                SaveSystem::SaveDifficulty(name, GetGameDifficulty(), currentSaveData);
            }
            for(auto quest : quests)
            {
                std::uint32_t questId = *static_cast<uint32_t*>(quest);
                if (untrackedQuests.contains(questId) && IsTrackedQuest(quest))
                {

                    SetTrackedQuest(quest, false);
                    changed = true;
                }
            }
            if (changed)
            {
                GameEvent evt{0x2A, -1, -1, 0}; // Refresh quest log ui
                SendEvent(evt);
            }
        }
    }
}

using GetRawInputData_t = UINT (WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
GetRawInputData_t oGetRawInputData = nullptr;
UINT WINAPI hkGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
    UINT result = oGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

    if (!menuOpen)
        return result;
    if (!pData || result == 0)
        return result;
    if (!ImGui::GetCurrentContext())
        return result;

    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard && !io.WantCaptureMouse)
        return result;

    RAWINPUT* raw = static_cast<RAWINPUT*>(pData);
    if (raw && uiCommand == RID_INPUT)
    {
        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            if (menuReleaseMouse)
            {
                // Inject releases
                raw->data.mouse.usButtonFlags =
                    RI_MOUSE_LEFT_BUTTON_UP |
                    RI_MOUSE_RIGHT_BUTTON_UP |
                    RI_MOUSE_MIDDLE_BUTTON_UP |
                    RI_MOUSE_BUTTON_4_UP |
                    RI_MOUSE_BUTTON_5_UP;

                raw->data.mouse.lLastX = 0;
                raw->data.mouse.lLastY = 0;

                menuReleaseMouse = false;
            }
            else if(io.WantCaptureMouse || io.WantCaptureKeyboard)
            {
                raw->data.mouse.lLastX = 0;
                raw->data.mouse.lLastY = 0;
                raw->data.mouse.usButtonFlags = 0;
                raw->data.mouse.usButtonData = 0;
            }
        }

        if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            if (menuReleaseKeyboard)
            {
                raw->data.keyboard.Flags |= RI_KEY_BREAK;
                raw->data.keyboard.Message = WM_KEYUP;

                menuReleaseKeyboard = false;
            }
            else if(io.WantCaptureKeyboard || io.WantCaptureMouse)
            {
                if (!(raw->data.keyboard.Flags & RI_KEY_BREAK))
                {
                    raw->data.keyboard.Flags |= RI_KEY_BREAK;
                    raw->data.keyboard.Message = WM_KEYUP;
                }
            }
        }
    }
    return result;
}

void closeMenu()
{
    menuOpen = false;
    menuGrabKeyboard = true;
    menuSearch[0] = '\0';
    menuSortTeleports = true;
    menuSortTowns = true;
    menuFavoriteTowns = true;
}

void UpdateMenuToggle()
{
    if(g_toggleKey == 0)
        return;

    if(g_waitingForKey)
        return;

    bool toggleDown = (GetAsyncKeyState(g_toggleKey) & 0x8000) != 0;
    if (toggleDown && !lastToggleState)
    {
        if(menuOpen)
        {
            closeMenu();
        }
        else
        {
            menuOpen = true;
            menuReleaseMouse = true;
            menuReleaseKeyboard = true;
        }
    }
    lastToggleState = toggleDown;

    if (menuOpen)
    {
        bool escDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            if (escDown && !lastEscState)
                closeMenu();
        }
        lastEscState = escDown;
    }
}

void DrawEndlessDungeonNotifyPortal()
{
    const char* msg = l.tr(TextId::GameEndlessDungeonNotifyPortal);
    ImFont* font = ImGui::GetFont();
    float fontSizeDungeon = 32.0f;
    ImVec2 size = font->CalcTextSizeA(fontSizeDungeon, FLT_MAX, 0.0f, msg);
    ImVec2 display = ImGui::GetIO().DisplaySize;
    ImVec2 pos((display.x - size.x) * 0.5f, (display.y - size.y) * endlessDungeonNotifyYPos);
    ImGui::GetBackgroundDrawList()->AddText(font, 32.0f, ImVec2(pos.x + 2, pos.y + 2), IM_COL32_BLACK, msg);
    ImGui::GetBackgroundDrawList()->AddText(font, 32.0f, pos, IM_COL32_WHITE, msg);
}

using Present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
Present_t OriginalPresent = nullptr;
HRESULT __stdcall HookPresent(IDXGISwapChain* swap, UINT sync, UINT flags)
{
    if(!imguiInitialized)
        InitImGui(swap);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    UpdateMenuToggle();

    if(updateReq)
    {
        if(menuOpen)
        {
            ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
            ImGui::Begin(l.tr(TextId::WindowName));
            ImGui::Text(l.tr(TextId::MainTextUpdate));
            ImGui::End();
        }
    }
    else
    {
        if(enabledEndlessDungeonNotifyPortal && showEndlessDungeonNotify)
            DrawEndlessDungeonNotifyPortal();

        if(menuOpen)
        {
            ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
            ImGui::Begin(l.tr(TextId::WindowName));

            void* engine = GetGameEnginePtr();
            void* mainPlayer = GetMainPlayer();
            bool canUsePersonalTeleport =  MainPlayerCanUsePersonalTeleport();
            struct TeleportEntry
            {
                UID uid;
                const Teleports* teleport;
                float distance;
                std::uint32_t idx;
            };
            static std::vector<TeleportEntry> sortedTeleports;
            struct TownEntry
            {
                UID uid;
                const Towns* town;
            };
            static std::vector<TownEntry> sortedTowns;
            static std::vector<TownEntry> sortedFavoriteTown;
            static std::vector<std::string> townModesStrings;
            static std::vector<const char*> townModes;
            static int currentFavoriteTown = -1;

            VecTeleportUIDs teleportUIDs = GetTeleportUIDs();

            if (ImGui::BeginTabBar("MainTabs"))
            {
                if (ImGui::BeginTabItem(l.tr(TextId::TabsTeleport)))
                {
#ifndef NOLOG
                    if (ImGui::Button("Debug: Log TeleportUIDs, Player Position, Player Name"))
                    {
                        VecTeleportUIDs vec = GetTeleportUIDs();
                        for (UID* cur = vec.first; cur < vec.last; cur++)
                        {
                            if(cur)
                                Log("{{0x%08X, 0x%08X, 0x%08X, 0x%08X}, {{}, \"\"}},", cur->a, cur->b, cur->c, cur->d);
                        }
                        if (mainPlayer)
                        {
                            Vec3f world = GetWorldPosition(GetFootCoords());
                            Log("Player feet world: {%.3f, %.3f, %.3f}", world.x, world.y, world.z);
                            Log("Player: %s", GetMainPlayerName().c_str());
                        }
                    }
#endif
                    if (mainPlayer)
                    {
                        if (ImGui::BeginTable("teleport_button_table", 2))
                        {
                            if(enabledFavoriteTown)
                            {
                                ImGui::TableNextColumn();
                                bool unlockedFavoriteTown = std::find(teleportUIDs.begin(), teleportUIDs.end(), favoriteTown) != teleportUIDs.end();
                                auto it = towns.find(favoriteTown);
                                if (it == towns.end())
                                {
                                    it = towns.begin();
                                    favoriteTown = it->first;
                                    Log("Invalid Favorite Town. Setting new default");
                                }

                                const Towns& tp = it->second;
                                ImGui::BeginDisabled(!engine || !canUsePersonalTeleport || !unlockedFavoriteTown);
                                if (ImGui::Button((l.tr(TextId::MainButtonFavoriteTown) + tp.name).c_str()))
                                {
                                    if (!engine)
                                    {
                                        Log("No GameEngine");
                                    }
                                    else if (!MainPlayerCanUsePersonalTeleport())
                                    {
                                        Log("Teleport currently not allowed");
                                    }
                                    else if (!unlockedFavoriteTown)
                                    {
                                        Log("Favorite town not unlocked");
                                    }
                                    else
                                    {
                                        CreateFixedItemTeleport();
                                        InitiateTeleport(GetTownPos(tp));
                                        closeMenu();
                                    }
                                }
                                ImGui::EndDisabled();
                            }

                            if(enabledNearestTown)
                            {
                                ImGui::TableNextColumn();

                                const Towns* nearestTown = nullptr;
                                float nearestDistance = FLT_MAX;

                                Vec3f world = GetWorldPosition(GetFootCoords());

                                for (auto const& uid : teleportUIDs)
                                {
                                    auto it = towns.find(uid);
                                    if (it == towns.end())
                                        continue;

                                    const Towns& town = it->second;

                                    float dx = world.x - town.worldPos.x;
                                    float dz = world.z - town.worldPos.z;
                                    float distance = (dx * dx) + (dz * dz);
                                    if (distance < nearestDistance)
                                    {
                                        nearestDistance = distance;
                                        nearestTown = &town;
                                    }
                                }

                                if (nearestTown)
                                {
                                    ImGui::BeginDisabled(!engine || !canUsePersonalTeleport);
                                    if (ImGui::Button((l.tr(TextId::MainButtonNearestTown) + nearestTown->name).c_str()))
                                    {
                                        if (!engine)
                                        {
                                            Log("No GameEngine");
                                        }
                                        else if (!MainPlayerCanUsePersonalTeleport())
                                        {
                                            Log("Teleport currently not allowed");
                                        }
                                        else
                                        {
                                            CreateFixedItemTeleport();
                                            InitiateTeleport(GetTownPos(*nearestTown));
                                            closeMenu();
                                        }
                                    }
                                    ImGui::EndDisabled();
                                }
                            }
                            ImGui::EndTable();
                        }
                    }

                    const char* sortModes[] =
                    {
                        l.tr(TextId::MainTeleportTableSortByDefault),
                        l.tr(TextId::MainTeleportTableSortByDistance),
                        l.tr(TextId::MainTeleportTableSortByDiscovery),
                        l.tr(TextId::MainTeleportTableSortByAlphabet),
                    };
                    int32_t currentSort = static_cast<int32_t>(menuTeleportSort);
                    std::string_view filter = menuSearch;
                    if (ImGui::BeginTable("input_table", 2))
                    {
                        ImGui::TableNextColumn();
                        if(menuGrabKeyboard)
                        {
                            ImGui::SetKeyboardFocusHere();
                            menuGrabKeyboard = false;
                        }
                        ImGui::InputText(l.tr(TextId::MainTeleportTableSearch), menuSearch, sizeof(menuSearch));

                        ImGui::TableNextColumn();

                        if (ImGui::Combo(l.tr(TextId::MainTeleportTableSortBy), &currentSort, sortModes, IM_ARRAYSIZE(sortModes)))
                        {
                            menuTeleportSort = static_cast<TeleportSort>(currentSort);
                            menuSortTeleports = true;
                            ImGui::MarkIniSettingsDirty();
                        }

                        ImGui::EndTable();
                    }

                    if (ImGui::BeginTable("teleports_table", 3, ImGuiTableFlags_BordersInnerV))
                    {
                        ImGui::TableSetupColumn(l.tr(TextId::MainTeleportTableRifts));
                        ImGui::TableSetupColumn(l.tr(TextId::MainTeleportTableTowns));
                        ImGui::TableSetupColumn(l.tr(TextId::MainTeleportTablePersonalRift));
                        ImGui::TableHeadersRow();
                        ImGui::TableNextColumn();
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
                        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.0f);
                        ImGui::BeginChild("rifts_column", ImVec2(0, 0));

                        if (mainPlayer)
                        {

                            if(menuSortTeleports)
                            {
                                sortedTeleports.clear();
                                sortedTeleports.reserve(teleportUIDs.size());
                                Vec3f world = GetWorldPosition(GetFootCoords());

                                std::uint32_t idx = 0;
                                for (auto const& uid : teleportUIDs)
                                {
                                    auto it = teleports.find(uid);
                                    if (it == teleports.end())
                                        continue;

                                    const Teleports& tp = it->second;

                                    float dx = world.x - tp.worldPos.x;
                                    float dz = world.z - tp.worldPos.z;

                                    sortedTeleports.push_back({uid, &tp, dx * dx + dz * dz, idx});
                                    idx++;
                                }
                                switch (menuTeleportSort)
                                {
                                    case TeleportSort::Distance:
                                        std::sort(sortedTeleports.begin(), sortedTeleports.end(), [](const auto& a, const auto& b)
                                        {
                                            return a.distance < b.distance;
                                        });
                                        break;

                                    case TeleportSort::Alphabetical:
                                        std::sort(sortedTeleports.begin(), sortedTeleports.end(), [](const auto& a, const auto& b)
                                        {
                                            return a.teleport->name < b.teleport->name;
                                        });
                                        break;

                                    case TeleportSort::Discovery:
                                        std::sort(sortedTeleports.begin(), sortedTeleports.end(), [](const auto& a, const auto& b)
                                        {
                                            return a.idx < b.idx;
                                        });
                                        break;

                                    case TeleportSort::Default:
                                        std::sort(sortedTeleports.begin(), sortedTeleports.end(), [](const auto& a, const auto& b)
                                        {
                                            return a.teleport->order < b.teleport->order;
                                        });
                                        break;
                                }

                                menuSortTeleports = false;
                            }

                            int enabledMatches = 0;
                            const Teleports* singleMatch = nullptr;
                            std::string b = std::string(filter);
                            for(auto& entry : sortedTeleports)
                            {
                                if(!entry.teleport)
                                    continue;
                                const auto& tp = *entry.teleport;
                                if (!filter.empty())
                                {
                                    std::string a = tp.name;
                                    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                                    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
                                    if(a.find(b) == std::string::npos)
                                    {
                                        continue;
                                    }
                                }
                                enabledMatches++;
                                if (enabledMatches == 1)
                                {
                                    singleMatch = &tp;
                                }

                                ImGui::BeginDisabled(!engine || !canUsePersonalTeleport);
                                if (ImGui::Selectable((tp.name + "##rift").c_str()))
                                {
                                    if (!engine)
                                    {
                                        Log("No GameEngine");
                                    }
                                    else if (!MainPlayerCanUsePersonalTeleport())
                                    {
                                        Log("Teleport currently not allowed");
                                    }
                                    else
                                    {
                                        CreateFixedItemTeleport();
                                        InitiateTeleport(tp.worldPos);
                                        closeMenu();
                                    }
                                }
                                ImGui::EndDisabled();
                            }
                            if (enabledMatches == 1 && ImGui::IsKeyPressed(ImGuiKey_Enter) && singleMatch && canUsePersonalTeleport && engine)
                            {
                                if (!engine)
                                {
                                    Log("No GameEngine");
                                }
                                else if (!MainPlayerCanUsePersonalTeleport())
                                {
                                    Log("Teleport currently not allowed");
                                }
                                else
                                {
                                    CreateFixedItemTeleport();
                                    InitiateTeleport(singleMatch->worldPos);
                                    closeMenu();
                                }
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleVar();
                        ImGui::TableNextColumn();

                        if (mainPlayer)
                        {
                            if(menuSortTowns)
                            {
                                sortedTowns.clear();
                                sortedTowns.reserve(teleportUIDs.size());
                                for (auto& uid : teleportUIDs)
                                {
                                    auto townIt = towns.find(uid);
                                    if (townIt == towns.end())
                                        continue;

                                    sortedTowns.push_back({uid, &townIt->second});
                                }
                                std::sort(sortedTowns.begin(), sortedTowns.end(), [](const TownEntry& a, const TownEntry& b)
                                {
                                    return a.town->order < b.town->order;
                                });
                                menuSortTowns = false;
                            }

                            for(const auto& entry : sortedTowns)
                            {
                                const auto& town = *entry.town;
                                ImGui::BeginDisabled(!engine || !canUsePersonalTeleport);
                                if (ImGui::Selectable((town.name + "##town").c_str()))
                                {
                                    if (!engine)
                                    {
                                        Log("No GameEngine");
                                    }
                                    else if (!MainPlayerCanUsePersonalTeleport())
                                    {
                                        Log("Teleport currently not allowed");
                                    }
                                    else
                                    {
                                        CreateFixedItemTeleport();
                                        InitiateTeleport(GetTownPos(town));
                                        closeMenu();
                                    }
                                }
                                ImGui::EndDisabled();
                            }
                        }

                        ImGui::TableNextColumn();

                        {
                            std::lock_guard<std::mutex> lock(playerRiftsMutex);
                            for(auto player : GetPlayerNames())
                            {
                                auto it = playerRifts.find(player.id);
                                ImGui::BeginDisabled(!engine || !canUsePersonalTeleport || it == playerRifts.end());
                                if (ImGui::Selectable(player.name.c_str()))
                                {
                                    if (!engine)
                                    {
                                        Log("No GameEngine");
                                    }
                                    else if (!MainPlayerCanUsePersonalTeleport())
                                    {
                                        Log("Teleport currently not allowed");
                                    }
                                    else if (it == playerRifts.end())
                                    {
                                        Log("Player has no Rift");
                                    }
                                    else
                                    {
                                        InitiateTeleport(GetWorldPosition(it->second));
                                        closeMenu();
                                    }
                                }
                                ImGui::EndDisabled();
                            }
                        }

                        ImGui::PopStyleVar();
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(l.tr(TextId::TabsSettings)))
                {

                    if (ImGui::Checkbox(l.tr(TextId::SettingsCheckEnabledNearestTown), &enabledNearestTown))
                    {
                        ImGui::MarkIniSettingsDirty();
                    }
                    if (ImGui::Checkbox(l.tr(TextId::SettingsCheckEnabledFavoriteTown), &enabledFavoriteTown))
                    {
                        ImGui::MarkIniSettingsDirty();
                    }
                    if (ImGui::Checkbox(l.tr(TextId::SettingsCheckEnabledConvenientTown), &enabledConvenientTown))
                    {
                        ImGui::MarkIniSettingsDirty();
                    }

                    if (menuFavoriteTowns)
                    {
                        townModes.clear();
                        townModesStrings.clear();
                        sortedFavoriteTown.clear();
                        sortedFavoriteTown.reserve(towns.size());
                        for (auto& town : towns)
                        {
                            auto tpIt = teleports.find(town.first);
                            if (tpIt == teleports.end())
                                continue;

                            sortedFavoriteTown.push_back({town.first, &town.second});
                        }
                        townModes.reserve(sortedFavoriteTown.size());
                        townModesStrings.reserve(sortedFavoriteTown.size());
                        std::sort(sortedFavoriteTown.begin(), sortedFavoriteTown.end(), [](const TownEntry& a, const TownEntry& b)
                        {
                            return a.town->order < b.town->order;
                        });
                        for (const auto& entry : sortedFavoriteTown)
                        {
                            if(std::find(teleportUIDs.begin(), teleportUIDs.end(), entry.uid) != teleportUIDs.end())
                                townModesStrings.push_back(entry.town->name);
                            else
                                townModesStrings.push_back(entry.town->name + l.tr(TextId::SettingsTextFavoriteTownUndiscovered));
                            townModes.push_back(townModesStrings.back().c_str());
                        }
                        for (int i = 0; i < static_cast<int>(sortedFavoriteTown.size()); i++)
                        {
                            if (sortedFavoriteTown[i].uid == favoriteTown)
                            {
                                currentFavoriteTown = i;
                                break;
                            }
                        }
                        menuFavoriteTowns = false;
                    }
                    ImGui::BeginDisabled(!enabledFavoriteTown);
                    if (ImGui::Combo(l.tr(TextId::SettingsComboFavoriteTown), &currentFavoriteTown, townModes.data(), townModes.size()))
                    {
                        favoriteTown = sortedFavoriteTown.at(currentFavoriteTown).uid;
                        ImGui::MarkIniSettingsDirty();
                    }
                    ImGui::EndDisabled();

                    ImGui::Separator();

                    if (ImGui::Checkbox(l.tr(TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal), &enabledEndlessDungeonNotifyPortal))
                    {
                        ImGui::MarkIniSettingsDirty();
                    }

                    ImGui::BeginDisabled(!enabledEndlessDungeonNotifyPortal);
                    ImGui::TextWrapped(l.tr(TextId::SettingsSliderEndlessDungeonNotifyYPos));
                    int yPosPercent = static_cast<int>(endlessDungeonNotifyYPos * 100.0f);
                    if (ImGui::SliderInt("##EndlessDungeonNotifyYPos", &yPosPercent, 0, 100, "%d%%"))
                    {
                        endlessDungeonNotifyYPos = yPosPercent / 100.0f;
                        ImGui::MarkIniSettingsDirty();
                    }
                    if (ImGui::IsItemActive())
                    {
                        DrawEndlessDungeonNotifyPortal();
                    }
                    ImGui::EndDisabled();

                    ImGui::Separator();

                    if (ImGui::Checkbox(l.tr(TextId::SettingsCheckEnabledQuestTrackRestore), &enabledQuestTrackRestore))
                    {
                        ImGui::MarkIniSettingsDirty();
                    }

                    ImGui::Separator();

                    static int32_t currentFont = userFont ?
                        static_cast<int32_t>(std::distance(
                                    fontNames.begin(),
                                    std::find_if(fontNames.begin(), fontNames.end(),
                                        [](const char* s) { return std::strcmp(s, userFont->name.c_str()) == 0; })
                                    ))
                        : 0;
                    if (ImGui::Combo(l.tr(TextId::SettingsFont), &currentFont, fontNames.data(), static_cast<int32_t>(fontNames.size())))
                    {
                        if (currentFont > 0 && currentFont <= availableFonts.size())
                            userFont = &availableFonts.at(currentFont-1);
                        else
                            userFont = nullptr;
                        fontChanged = true;
                        ImGui::MarkIniSettingsDirty();
                    }

                    if (ImGui::SliderInt(l.tr(TextId::SettingsFontSize), &fontSize, 4, 30, "%d px"))
                    {
                        fontChanged = true;
                        ImGui::MarkIniSettingsDirty();
                    }

                    ImGui::Separator();

                    int32_t currentLanguage = static_cast<int32_t>(uiLanguage.load());
                    static const auto languages = []()
                    {
                        std::array<const char*, static_cast<size_t>(UiLanguages::END)> names{};
                        for (size_t i = 0; i < names.size(); ++i)
                            names[i] = ToString(static_cast<UiLanguages>(i));
                        return names;
                    }();
                    if (ImGui::Combo(l.tr(TextId::SettingsLanguage), &currentLanguage, languages.data(), static_cast<int32_t>(languages.size())))
                    {
                        uiLanguage = static_cast<UiLanguages>(currentLanguage);
                        languageChanged = true;
                        ImGui::MarkIniSettingsDirty();
                    }

                    ImGui::Text(l.tr(TextId::SettingsTextChangeKeybind));
                    ImGui::SameLine();
                    if (!g_waitingForKey)
                    {
                        std::string keyName = GetKeyName(g_toggleKey);
                        ImGui::Text("%s", keyName.c_str());
                        ImGui::SameLine();
                        if (ImGui::Button(l.tr(TextId::SettingsButtonChangeKeybind)))
                        {
                            g_waitingForKey = true;
                            g_captureArmed = false;
                        }
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), l.tr(TextId::SettingsTextChangeKeybindPressAnyKey));
                        ImGui::SameLine();
                        if (ImGui::Button(l.tr(TextId::SettingsCancel)))
                        {
                            g_waitingForKey = false;
                            g_captureArmed = false;
                        }
                        if (!g_captureArmed)
                        {
                            if (!AnyKeyDown())
                            {
                                g_captureArmed = true;
                            }
                        }
                        else
                        {
                            UINT key = GetPressedKey();
                            if (key != 0)
                            {
                                if (key == VK_ESCAPE)
                                {
                                    g_waitingForKey = false;
                                    g_captureArmed = false;
                                    lastEscState = true;
                                }
                                else
                                {
                                    g_toggleKey = key;
                                    g_waitingForKey = false;
                                    g_captureArmed = false;
                                    ImGui::MarkIniSettingsDirty();
                                    lastToggleState = true;
                                }
                            }
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
    }
    ImGui::Render();
    g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (languageChanged)
    {
        if (uiLanguage == UiLanguages::Automatic)
            loadLanguage(GetLanguageName());
        else
            loadLanguage(ToString(uiLanguage));
        languageChanged = false;
    }
    if (fontChanged)
    {
        LoadImGuiFont(userFont, availableFonts);
        fontChanged = false;
    }

    return OriginalPresent(swap, sync, flags);
}

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        switch (msg)
        {
            case WM_MOUSEMOVE:
            case WM_MOUSEWHEEL:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                return 0;
        }
    }

    if (io.WantCaptureKeyboard)
    {
        switch (msg)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return 0;
        }
    }

    static bool once = false;
    if (!once)
    {
        Log("WndProc hook active");
        once = true;
    }

    return CallWindowProc(g_OriginalWndProc, hwnd, msg, wParam, lParam);
}

void InitImGui(IDXGISwapChain* swap)
{
    if(imguiInitialized)
        return;

    if(ImGui::GetCurrentContext()==nullptr)
    {
        Log("Creating own ImGui context");
        swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device));
        g_device->GetImmediateContext(&g_context);
        DXGI_SWAP_CHAIN_DESC desc;
        swap->GetDesc(&desc);
        g_hwnd = desc.OutputWindow;
        g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookWndProc)));
        ID3D11Texture2D* backBuffer=nullptr;
        swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
        backBuffer->Release();

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_device, g_context);
    }

    availableFonts = EnumerateWindowsFonts();
    fontNames.clear();
    fontNames.reserve(availableFonts.size() + 1);
    fontNames.push_back("Default ProggyClean");
    for (const FontInfo& font : availableFonts)
        fontNames.push_back(font.name.c_str());

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "RiftgateCompanion.ini";
    {
        ImGuiSettingsHandler ini_handler{};
        ini_handler.TypeName = "Settings";
        ini_handler.TypeHash = ImHashStr("Settings");
        ini_handler.ReadOpenFn = ReadSettingsOpen;
        ini_handler.ReadLineFn = ReadSettingsLine;
        ini_handler.WriteAllFn = WriteSettings;
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
    }
    ImGui::LoadIniSettingsFromDisk(io.IniFilename);

    LoadImGuiFont(userFont, availableFonts);

    if (uiLanguage == UiLanguages::Automatic)
        loadLanguage(GetLanguageName());
    else
        loadLanguage(ToString(uiLanguage));
    imguiInitialized=true;

    Log("ImGui initialized");
}

void fillFunctionAddresses(const std::unordered_map<std::string_view, std::string>& symbols, HMODULE module, std::unordered_map<std::string_view, uintptr_t>& addresses)
{
    if (!module)
        return;

    for (const auto& [name, symbol] : symbols)
    {
        FARPROC proc = GetProcAddress(module, symbol.c_str());
        if (!proc)
        {
            Log("Failed to resolve %s with symbol %s", name.data(), symbol.c_str());
            updateReq = true;
        }
        else
        {
            addresses[name] = reinterpret_cast<uintptr_t>(proc);
        }
    }
}

bool InstallHook(const char* name, LPVOID detour, LPVOID* original)
{
    auto it = g_GameAddresses.find(name);
    if (it == g_GameAddresses.end())
    {
        it = g_EngineAddresses.find(name);
        if (it == g_EngineAddresses.end())
        {
            Log("Hook address not found: %s", name);
            return false;
        }
    }
    uintptr_t addr = it->second;
    if (MH_CreateHook(reinterpret_cast<LPVOID>(addr), detour, original) != MH_OK)
    {
        Log("MH_CreateHook failed: %s", name);
        return false;
    }
    if (MH_EnableHook(reinterpret_cast<LPVOID>(addr)) != MH_OK)
    {
        Log("MH_EnableHook failed: %s", name);
        return false;
    }
    Log("Hook installed: %s", name);
    return true;
}

bool InstallHookApi(LPCWSTR module, LPCSTR name, LPVOID detour, LPVOID* original, LPVOID target)
{
    if (MH_CreateHookApi(module, name, detour, original) != MH_OK)
    {
        Log("MH_CreateHookApi failed: %s", name);
        return false;
    }
    if (MH_EnableHook(target) != MH_OK)
    {
        Log("MH_EnableHookApi failed: %s", name);
        return false;
    }
    Log("HookApi installed: %s", name);
    return true;
}

bool InstallHooks(void* present)
{
    Log("Installing D3D11 hook");

    if (!present)
        return false;

    MH_Initialize();
    if (MH_CreateHook(present, reinterpret_cast<LPVOID>(&HookPresent), reinterpret_cast<LPVOID*>(&OriginalPresent)) != MH_OK)
    {
        Log("Failed to create present hook");
        return false;
    }
    if (MH_EnableHook(present) != MH_OK)
    {
        Log("Failed to enable present hook");
        return false;
    }
    Log("Present hook enabled");

    HMODULE dxgi = nullptr;
    HMODULE d3d11 = nullptr;
    HMODULE game = nullptr;
    HMODULE engine = nullptr;
    while(true)
    {
        dxgi = GetModuleHandleA("dxgi.dll");
        d3d11 = GetModuleHandleA("d3d11.dll");
        game = GetModuleHandleA("Game.dll");
        engine = GetModuleHandleA("Engine.dll");
        if(dxgi && d3d11 && game && engine)
            break;
        Sleep(100);
    }
    Log("All modules ready");

    fillFunctionAddresses(g_GameSymbols, GameModule(), g_GameAddresses);
    fillFunctionAddresses(g_EngineSymbols, EngineModule(), g_EngineAddresses);

    if(!updateReq)
    {
        // Teleport
        InstallHook(
                "sym.Game.dll__CreateFixedItemTeleportNetHook_GameEngine_GAME__QEAAXAEBVWorldCoords_2_IIAEBV__basic_string_DU__char_traits_D_std__V__allocator_D_2__std___Z",
                reinterpret_cast<LPVOID>(&hkCreateFixedItemTeleportNetHook),
                reinterpret_cast<LPVOID*>(&oCreateFixedItemTeleportNetHook)
                );
        InstallHookApi(
                L"user32",
                "GetRawInputData",
                reinterpret_cast<LPVOID>(hkGetRawInputData),
                reinterpret_cast<LPVOID*>(&oGetRawInputData),
                reinterpret_cast<LPVOID>(&GetRawInputData));
#ifndef NOLOG
        // Riftgate UID logging
        InstallHook(
                "sym.Engine.dll__Attach_Actor_GAME__UEAAXPEAVEntity_2_AEBVCoords_2_AEBVName_2__N333_Z",
                reinterpret_cast<LPVOID>(&hkActorAttach),
                reinterpret_cast<LPVOID*>(&oActorAttach)
                );
#endif
        // Language
        InstallHook(
                "sym.Engine.dll__ReloadLanguage_LocalizationManager_GAME__QEAAXPEBD_Z",
                reinterpret_cast<LPVOID>(&hkReloadLanguage),
                reinterpret_cast<LPVOID*>(&oReloadLanguage)
                );
        // Shattered Realm notification
        InstallHook(
                "sym.Game.dll__RTTI_new_FixedItemDungeonTeleport_GAME__KAPEAXXZ",
                reinterpret_cast<LPVOID>(&hkRTTI_new_FixedItemDungeonTeleport),
                reinterpret_cast<LPVOID*>(&oRTTI_new_FixedItemDungeonTeleport)
                );
        InstallHook(
                "sym.Game.dll__SyncDungeonProgress_GameEngine_GAME__QEAAXI_Z",
                reinterpret_cast<LPVOID>(&hkSyncDungeonProgress),
                reinterpret_cast<LPVOID*>(&oSyncDungeonProgress)
                );
        InstallHook(
                "sym.Game.dll__Clear_EndlessDungeon_Generator_GAME__QEAAXXZ",
                reinterpret_cast<LPVOID>(&hkClearEndlessDungeonGenerator),
                reinterpret_cast<LPVOID*>(&oClearEndlessDungeonGenerator)
                );
        InstallHook(
                "sym.Game.dll__RequestToUse_FixedItemDungeonTeleport_GAME__UEAAXI_Z",
                reinterpret_cast<LPVOID>(&hkRequestToUse_FixedItemDungeonTeleport),
                reinterpret_cast<LPVOID*>(&oRequestToUse_FixedItemDungeonTeleport)
                );
        // Quest tracking
        InstallHook(
                "sym.Game.dll__SetTracked_Quest2_GAME__QEAAX_N_Z",
                reinterpret_cast<LPVOID>(&hkSetTracked),
                reinterpret_cast<LPVOID*>(&oSetTracked)
                );
        InstallHook(
                "sym.Game.dll__SetMainPlayer_PlayerManagerClient_GAME__QEAAXI_Z",
                reinterpret_cast<LPVOID>(&hkPlayerManagerSetMainPlayer),
                reinterpret_cast<LPVOID*>(&oPlayerManagerSetMainPlayer)
                );
    }

    return true;
}

static LRESULT CALLBACK TmpWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
void* GetPresentAddress()
{
    // Create tmp window for tmp d3d11 device
    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = TmpWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "RiftgateCompanionTmp";
    if (!RegisterClassA(&wc))
    {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            Log("RegisterClass failed: %lu", error);
            return nullptr;
        }
    }
    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, "RiftgateCompanionTmp", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, nullptr, nullptr, instance, nullptr);
    if (!hwnd)
    {
        Log("CreateWindowEx failed: %lu", GetLastError());
        UnregisterClassA(wc.lpszClassName, instance);
        return nullptr;
    }
    // Create tmp d3d11 device to obtain games swapchain
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 1;
    desc.BufferDesc.Width = 1;
    desc.BufferDesc.Height = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &desc, &swapChain, &device, &featureLevel, &context);
    if (FAILED(hr))
    {
        Log("D3D11CreateDeviceAndSwapChain failed: 0x%08lX", static_cast<unsigned long>(hr));
        if (context) context->Release();
        if (device) device->Release();
        if (swapChain) swapChain->Release();
        DestroyWindow(hwnd);
        UnregisterClassA(wc.lpszClassName, instance);
        return nullptr;
    }
    if (!swapChain || !device || !context)
    {
        Log("D3D11CreateDeviceAndSwapChain invalid device and swapchain creation");
        if (context) context->Release();
        if (device) device->Release();
        if (swapChain) swapChain->Release();
        DestroyWindow(hwnd);
        UnregisterClassA(wc.lpszClassName, instance);
        return nullptr;
    }
    Log("Obtained tmp swapchain: %p", swapChain);
    void** vtable = *reinterpret_cast<void***>(swapChain);
    void* present = vtable[8];
    Log("Present address: %p", present);

    if (context) context->Release();
    if (device) device->Release();
    if (swapChain) swapChain->Release();
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, instance);
    return present;
}

void InstallD3D11Hook()
{
    for (int i = 0; i < 20; i++)
    {
        if (void* present = GetPresentAddress())
        {
            if (InstallHooks(present))
                break;
        }
        Sleep(1000);
    }
}
