#pragma once

#include <unordered_map>
#include <string>

static const std::unordered_map<std::string_view, std::string> g_GameSymbols = {
    // Teleport
    {"sym.Game.dll__CreateFixedItemTeleportNetHook_GameEngine_GAME__QEAAXAEBVWorldCoords_2_IIAEBV__basic_string_DU__char_traits_D_std__V__allocator_D_2__std___Z", "?CreateFixedItemTeleportNetHook@GameEngine@GAME@@QEAAXAEBVWorldCoords@2@IIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"},
    {"sym.Game.dll__GetMainPlayer_GameEngine_GAME__QEBAPEAVPlayer_2_XZ", "?GetMainPlayer@GameEngine@GAME@@QEBAPEAVPlayer@2@XZ"},
    {"sym.Game.dll__MainPlayerCanUsePersonalTeleport_GameEngine_GAME__QEBA_NXZ", "?MainPlayerCanUsePersonalTeleport@GameEngine@GAME@@QEBA_NXZ"},
    {"sym.Game.dll__CreateFixedItemTeleport_GameEngine_GAME__QEAAXXZ", "?CreateFixedItemTeleport@GameEngine@GAME@@QEAAXXZ"},
    {"sym.Game.dll__InitiatePlayerTeleport_GameEngine_GAME__QEAAXHHHW4TeleportEffect_2__N_Z", "?InitiatePlayerTeleport@GameEngine@GAME@@QEAAXHHHW4TeleportEffect@2@_N@Z"},
    {"sym.Game.dll__GetPlayerManagerClient_GameEngine_GAME__QEBAPEAVPlayerManagerClient_2_XZ", "?GetPlayerManagerClient@GameEngine@GAME@@QEBAPEAVPlayerManagerClient@2@XZ"},
    {"sym.Game.dll__GetPlayerName_PlayerManagerClient_GAME__QEBA_AV__basic_string_GU__char_traits_G_std__V__allocator_G_2__std__I_Z", "?GetPlayerName@PlayerManagerClient@GAME@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@I@Z"},
    {"sym.Game.dll__GetTeleportUIDs_Player_GAME__QEBAAEBV__vector_VUniqueId_GAME___mem__XZ", "?GetTeleportUIDs@Player@GAME@@QEBAAEBV?$vector@VUniqueId@GAME@@@mem@@XZ"},
    {"sym.Game.dll__GetFootCoords_Player_GAME__UEAA_AVWorldCoords_2__N_Z", "?GetFootCoords@Player@GAME@@UEAA?AVWorldCoords@2@_N@Z"},
    {"sym.Game.dll__GetAllPlayersInGame_PlayerManagerClient_GAME__QEBAAEBV__vector_I_mem__XZ", "?GetAllPlayersInGame@PlayerManagerClient@GAME@@QEBAAEBV?$vector@I@mem@@XZ"},
    {"GameEngine", "?gGameEngine@GAME@@3PEAVGameEngine@1@EA"},
    // Shattered Realm notification
    {"sym.Game.dll__InEndlessDungeon_Character_GAME__QEBA_NXZ", "?InEndlessDungeon@Character@GAME@@QEBA_NXZ"},
    {"sym.Game.dll__RTTI_new_FixedItemDungeonTeleport_GAME__KAPEAXXZ", "?RTTI_new@FixedItemDungeonTeleport@GAME@@KAPEAXXZ"},
    {"sym.Game.dll__RequestToUse_FixedItemDungeonTeleport_GAME__UEAAXI_Z", "?RequestToUse@FixedItemDungeonTeleport@GAME@@UEAAXI@Z"},
    {"sym.Game.dll__SyncDungeonProgress_GameEngine_GAME__QEAAXI_Z", "?SyncDungeonProgress@GameEngine@GAME@@QEAAXI@Z"},
    {"sym.Game.dll__Clear_EndlessDungeon_Generator_GAME__QEAAXXZ", "?Clear@EndlessDungeon_Generator@GAME@@QEAAXXZ"},
    // Quest tracking
    {"sym.Game.dll__Get___Singleton_VQuest2Repository_GAME___GAME__SAPEAVQuest2Repository_2_XZ", "?Get@?$Singleton@VQuest2Repository@GAME@@@GAME@@SAPEAVQuest2Repository@2@XZ"},
    {"sym.Game.dll__GetQuests_Quest2Repository_GAME__QEAAXAEAV__vector_PEAVQuest2_GAME___mem__W4Filter_12__Z", "?GetQuests@Quest2Repository@GAME@@QEAAXAEAV?$vector@PEAVQuest2@GAME@@@mem@@W4Filter@12@@Z"},
    {"sym.Game.dll__GetGameDifficulty_GameEngine_GAME__QEBA_AW4GameDifficulty_2_XZ", "?GetGameDifficulty@GameEngine@GAME@@QEBA?AW4GameDifficulty@2@XZ"},
    {"sym.Game.dll__IsTracked_Quest2_GAME__QEBA_NXZ", "?IsTracked@Quest2@GAME@@QEBA_NXZ"},
    {"sym.Game.dll__SetTracked_Quest2_GAME__QEAAX_N_Z", "?SetTracked@Quest2@GAME@@QEAAX_N@Z"},
    {"sym.Game.dll__SetMainPlayer_PlayerManagerClient_GAME__QEAAXI_Z", "?SetMainPlayer@PlayerManagerClient@GAME@@QEAAXI@Z"},
    {"sym.Game.dll__GetPlayerName_Player_GAME__QEBAPEBGXZ", "?GetPlayerName@Player@GAME@@QEBAPEBGXZ"},
};

static const std::unordered_map<std::string_view, std::string> g_EngineSymbols = {
    // Teleport
    {"sym.Engine.dll__GetWorldPosition_WorldVec3_GAME__QEBA_AVVec3_2_XZ", "?GetWorldPosition@WorldVec3@GAME@@QEBA?AVVec3@2@XZ"},
    // Language
    {"sym.Engine.dll__ReloadLanguage_LocalizationManager_GAME__QEAAXPEBD_Z", "?ReloadLanguage@LocalizationManager@GAME@@QEAAXPEBD@Z"},
    {"sym.Engine.dll__Instance_LocalizationManager_GAME__SAAEAV12_XZ", "?Instance@LocalizationManager@GAME@@SAAEAV12@XZ"},
    {"sym.Engine.dll__GetCurrentLanguage_LocalizationManager_GAME__QEAAIXZ", "?GetCurrentLanguage@LocalizationManager@GAME@@QEAAIXZ"},
    {"sym.Engine.dll__GetLanguageName_LocalizationManager_GAME__QEBAAEBV__basic_string_DU__char_traits_D_std__V__allocator_D_2__std__W4Language_2__Z", "?GetLanguageName@LocalizationManager@GAME@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4Language@2@@Z"},
    {"sym.Engine.dll__Localize_LocalizationManager_GAME__QEAAPEBGPEBDZZ", "?Localize@LocalizationManager@GAME@@QEAAPEBGPEBDZZ"},
    // Quest tracking
    {"sym.Engine.dll__Send_EventManager_GAME__QEAAXPEBUGameEvent_2_I_Z", "?Send@EventManager@GAME@@QEAAXPEBUGameEvent@2@I@Z"},
    {"sym.Engine.dll__Get___Singleton_VEventManager_GAME___GAME__SAPEAVEventManager_2_XZ", "?Get@?$Singleton@VEventManager@GAME@@@GAME@@SAPEAVEventManager@2@XZ"},
#ifndef NOLOG
    // Riftgate UID logging
    {"sym.Engine.dll__Attach_Actor_GAME__UEAAXPEAVEntity_2_AEBVCoords_2_AEBVName_2__N333_Z", "?Attach@Actor@GAME@@UEAAXPEAVEntity@2@AEBVCoords@2@AEBVName@2@_N333@Z"},
    {"sym.Engine.dll__GetDescriptionTag_Actor_GAME__QEBAPEBDXZ", "?GetDescriptionTag@Actor@GAME@@QEBAPEBDXZ"},
    {"sym.Engine.dll__GetUniqueID_Entity_GAME__QEAAAEBVUniqueId_2_XZ", "?GetUniqueID@Entity@GAME@@QEAAAEBVUniqueId@2@XZ"},
    {"sym.Engine.dll__GetCoords_Entity_GAME__QEBA_AVWorldCoords_2_XZ", "?GetCoords@Entity@GAME@@QEBA?AVWorldCoords@2@XZ"},
#endif
};
