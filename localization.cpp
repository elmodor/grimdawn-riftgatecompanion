#include "localization.hpp"

#include <string_view>
#include <bitset>
#include <span>

#include "logger.hpp"

namespace
{
    struct TranslationEntry
    {
        TextId id;
        std::string_view value;
    };

    struct Language
    {
        std::string_view name;
        std::span<const TranslationEntry> entries;
    };

    static constexpr std::array EnglishEntries =
    {
        TranslationEntry{ TextId::WindowName,   "RiftgateCompanion - v0.2.1###Main" },
        TranslationEntry{ TextId::MainTextUpdate,   "Incompatible game version! Update required" },
        TranslationEntry{ TextId::MainButtonNearestTown,   "Nearest town: " },
        TranslationEntry{ TextId::MainTeleportTableRifts,   "Rifts" },
        TranslationEntry{ TextId::MainTeleportTableTowns,   "Towns" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift,   "Personal rifts" },
        TranslationEntry{ TextId::MainTeleportTableSearch,   "Search" },
        TranslationEntry{ TextId::MainTeleportTableSortBy,   "Sort rifts by" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault,   "Default" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance,   "Distance" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery,   "Discovery order" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet,   "Alphabetically" },
        TranslationEntry{ TextId::MainButtonFavoriteTown,   "Favorite town: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered,   " (Undiscovered)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown,   "Select favorite town" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown,   "Enable nearest town teleport button" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown,   "Enable favorite town teleport button" },
        TranslationEntry{ TextId::TabsTeleport,   "Teleport" },
        TranslationEntry{ TextId::TabsSettings,   "Settings" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal,   "Portal opened!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal,   "Enable Shattered Realm portal notification" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos,   "Shattered Realm Portal notification Y position" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Restore quest tracking status" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,   "Enable town teleport to a convenient location" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,   "Keybind toggle menu:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,   "Change" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,   "Press a key..." },
        TranslationEntry{ TextId::SettingsCancel,   "Cancel" },
        TranslationEntry{ TextId::SettingsLanguage,   "Language" },
        TranslationEntry{ TextId::SettingsFont,   "Font" },
        TranslationEntry{ TextId::SettingsFontSize,   "Font Size" },
    };

    static constexpr std::array SpanishEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "¡Versión del juego incompatible! Se requiere una actualización" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Ciudad más cercana: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Portales" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Ciudades" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Portales personales" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Buscar" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Ordenar portales por" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Predeterminado" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Distancia" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Descubrimiento" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Alfabéticamente" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Ciudad favorita: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (No descubierta)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Seleccionar ciudad favorita" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Activar botón de teletransporte a la ciudad más cercana" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Activar botón de teletransporte a la ciudad favorita" },
        TranslationEntry{ TextId::TabsTeleport, "Teletransporte" },
        TranslationEntry{ TextId::TabsSettings, "Configuración" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "¡Portal abierto!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Activar notificación del portal del Reino Fragmentado" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Posición Y de la notificación del portal del Reino Fragmentado" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Restaurar el estado del seguimiento de misiones" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Activar el teletransporte a una ubicación conveniente en la ciudad" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Tecla para mostrar/ocultar el menú:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Cambiar" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Pulsa una tecla..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Cancelar" },
        TranslationEntry{ TextId::SettingsLanguage,   "Idioma" },
        TranslationEntry{ TextId::SettingsFont,       "Fuente" },
        TranslationEntry{ TextId::SettingsFontSize,   "Tamaño de fuente" },
    };

    static constexpr std::array FrenchEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Version du jeu incompatible! Mise à jour requise" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Ville la plus proche: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Portails" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Villes" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Portails personnels" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Rechercher" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Trier les portails par" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Par défaut" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Distance" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Découverte" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Alphabétiquement" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Ville favorite: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Non découverte)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Sélectionner la ville favorite" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Activer le bouton de téléportation vers la ville la plus proche" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Activer le bouton de téléportation vers la ville favorite" },
        TranslationEntry{ TextId::TabsTeleport, "Téléportation" },
        TranslationEntry{ TextId::TabsSettings, "Paramètres" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portail ouvert!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Activer la notification du portail du Royaume brisé" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Position Y de la notification du portail du Royaume brisé" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Restaurer l’état du suivi des quêtes" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Activer le téléport vers un emplacement pratique en ville" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Touche pour afficher/masquer le menu :" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Modifier" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Appuyez sur une touche..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Annuler" },
        TranslationEntry{ TextId::SettingsLanguage,   "Langue" },
        TranslationEntry{ TextId::SettingsFont,       "Police" },
        TranslationEntry{ TextId::SettingsFontSize,   "Taille de police" },
    };

    static constexpr std::array GermanEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Inkompatible Spielversion! Update erforderlich" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Naheste Stadt: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Rifts" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Städte" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Persönliche Rifts" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Suchen" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Rifts sortieren nach" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Standard" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Entfernung" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Entdeckung" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Alphabetisch" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Favorisierte Stadt: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Unentdeckt)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Favorisierte Stadt auswählen" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Teleport Button für naheste Stadt aktivieren" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Teleport Button für favorisierte Stadt aktivieren" },
        TranslationEntry{ TextId::TabsTeleport, "Teleport" },
        TranslationEntry{ TextId::TabsSettings, "Einstellungen" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portal geöffnet!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Portalbenachrichtigung für das Erschütterte Reich aktivieren" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Y-Position der Portalbenachrichtigung im Erschütterten Reich" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Questverfolgungsstatus wiederherstellen" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Stadt teleport zu einem nützlichen Ort aktivieren" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Menü-Taste:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Ändern" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Taste drücken..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Abbrechen" },
        TranslationEntry{ TextId::SettingsLanguage,   "Sprache" },
        TranslationEntry{ TextId::SettingsFont,       "Schriftart" },
        TranslationEntry{ TextId::SettingsFontSize,   "Schriftgröße" },
    };

    static constexpr std::array ItalianEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Versione del gioco incompatibile! Aggiornamento richiesto" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Città più vicina: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Portali" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Città" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Portali personali" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Cerca" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Ordina i portali per" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Predefinito" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Distanza" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Scoperta" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Ordine alfabetico" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Città preferita: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Non scoperta)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Seleziona città preferita" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Abilita pulsante di teletrasporto alla città più vicina" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Abilita pulsante di teletrasporto alla città preferita" },
        TranslationEntry{ TextId::TabsTeleport, "Teletrasporto" },
        TranslationEntry{ TextId::TabsSettings, "Impostazioni" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portale aperto!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Abilita notifica del portale del Regno Frantumato" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Posizione Y della notifica del portale del Regno Frantumato" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Ripristina lo stato del tracciamento delle missioni" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Abilita il teletrasporto in città verso una posizione comoda" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Tasto per il menu:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Cambia" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Premi un tasto..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Annulla" },
        TranslationEntry{ TextId::SettingsLanguage,   "Lingua" },
        TranslationEntry{ TextId::SettingsFont,       "Carattere" },
        TranslationEntry{ TextId::SettingsFontSize,   "Dimensione carattere" },
    };

    static constexpr std::array CzechEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Nekompatibilní verze hry! Je vyžadována aktualizace" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Nejbližší město: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Trhliny" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Města" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Osobní trhliny" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Hledat" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Seřadit trhliny podle" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Výchozí" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Vzdálenost" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Objevení" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Abecedně" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Oblíbené město: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Neobjeveno)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Vybrat oblíbené město" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Povolit tlačítko teleportu do nejbližšího města" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Povolit tlačítko teleportu do oblíbeného města" },
        TranslationEntry{ TextId::TabsTeleport, "Teleport" },
        TranslationEntry{ TextId::TabsSettings, "Nastavení" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portál otevřen!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Povolit upozornění na portál Rozbité říše" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Y pozice upozornění portálu Rozbité říše" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Obnovit stav sledování úkolů" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Povolit teleport do města na vhodné místo" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Klávesa pro přepnutí nabídky:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Změnit" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Stiskněte klávesu..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Zrušit" },
        TranslationEntry{ TextId::SettingsLanguage,   "Jazyk" },
        TranslationEntry{ TextId::SettingsFont,       "Písmo" },
        TranslationEntry{ TextId::SettingsFontSize,   "Velikost písma" },
    };

    static constexpr std::array JapaneseEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "ゲームのバージョンに互換性がありません。アップデートが必要です" },
        TranslationEntry{ TextId::MainButtonNearestTown, "最寄りの街: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "リフトゲート" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "町" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "個人用リフトゲート" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "検索" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "リフトゲートの並び替え" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "デフォルト" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "距離" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "発見状況" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "五十音順" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "お気に入りの街: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (未発見)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "お気に入りの街を選択" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "最寄りの街へのテレポートボタンを有効化" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "お気に入りの街へのテレポートボタンを有効化" },
        TranslationEntry{ TextId::TabsTeleport, "テレポート" },
        TranslationEntry{ TextId::TabsSettings, "設定" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "ポータルが開きました！" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "シャッタード・レルムのポータル通知を有効化" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "シャッタード・レルム ポータル通知のY位置" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "クエスト追跡状態を復元する" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "便利な場所への街へのテレポートを有効にする" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "メニュー切り替えキー:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "変更" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "キーを押してください..." },
        TranslationEntry{ TextId::SettingsCancel,                          "キャンセル" },
        TranslationEntry{ TextId::SettingsLanguage,   "言語" },
        TranslationEntry{ TextId::SettingsFont,       "フォント" },
        TranslationEntry{ TextId::SettingsFontSize,   "フォントサイズ" },
    };

    static constexpr std::array KoreanEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "호환되지 않는 게임 버전입니다! 업데이트가 필요합니다" },
        TranslationEntry{ TextId::MainButtonNearestTown, "가장 가까운 마을: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "균열 관문" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "마을" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "개인 균열 관문" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "검색" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "균열 관문 정렬 기준" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "기본" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "거리" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "발견" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "가나다순" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "즐겨찾는 마을: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (미발견)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "즐겨찾는 마을 선택" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "가장 가까운 마을 순간이동 버튼 활성화" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "즐겨찾는 마을 순간이동 버튼 활성화" },
        TranslationEntry{ TextId::TabsTeleport, "순간이동" },
        TranslationEntry{ TextId::TabsSettings, "설정" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "포털이 열렸습니다!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "산산조각난 영역 포털 알림 활성화" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "산산조각난 영역 포털 알림 Y 위치" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "퀘스트 추적 상태 복원" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "편리한 위치로 마을 순간이동 활성화" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "메뉴 전환 키:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "변경" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "키를 누르세요..." },
        TranslationEntry{ TextId::SettingsCancel,                          "취소" },
        TranslationEntry{ TextId::SettingsLanguage,   "언어" },
        TranslationEntry{ TextId::SettingsFont,       "글꼴" },
        TranslationEntry{ TextId::SettingsFontSize,   "글꼴 크기" },
    };

    static constexpr std::array PolishEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Niezgodna wersja gry! Wymagana aktualizacja" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Najbliższe miasto: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Szczeliny" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Miasta" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Osobiste szczeliny" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Szukaj" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Sortuj szczeliny według" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Domyślnie" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Odległość" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Odkrycie" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Alfabetycznie" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Ulubione miasto: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Nieodkryte)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Wybierz ulubione miasto" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Włącz przycisk teleportacji do najbliższego miasta" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Włącz przycisk teleportacji do ulubionego miasta" },
        TranslationEntry{ TextId::TabsTeleport, "Teleportacja" },
        TranslationEntry{ TextId::TabsSettings, "Ustawienia" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portal otwarty!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Włącz powiadomienie o portalu Krainy Rozbitego Wymiaru" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Pozycja Y powiadomienia o portalu Krainy Rozbitego Wymiaru" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Przywróć stan śledzenia zadań" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Włącz teleportację do miasta w dogodne miejsce" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Klawisz przełączania menu:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Zmień" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Naciśnij klawisz..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Anuluj" },
        TranslationEntry{ TextId::SettingsLanguage,   "Język" },
        TranslationEntry{ TextId::SettingsFont,       "Czcionka" },
        TranslationEntry{ TextId::SettingsFontSize,   "Rozmiar czcionki" },
    };

    static constexpr std::array PortugueseEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Versão do jogo incompatível! Atualização necessária" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Cidade mais próxima: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Portais" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Cidades" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Portais pessoais" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Pesquisar" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Ordenar portais por" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Padrão" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Distância" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Descoberta" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Ordem alfabética" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Cidade favorita: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Não descoberta)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Selecionar cidade favorita" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Ativar botão de teleporte para a cidade mais próxima" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Ativar botão de teleporte para a cidade favorita" },
        TranslationEntry{ TextId::TabsTeleport, "Teleporte" },
        TranslationEntry{ TextId::TabsSettings, "Configurações" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Portal aberto!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Ativar notificação do portal do Reino Fragmentado" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Posição Y da notificação do portal do Reino Fragmentado" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Restaurar o estado do rastreamento de missões" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Ativar o teletransporte para um local conveniente na cidade" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Tecla para abrir/fechar o menu:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Alterar" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Pressione uma tecla..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Cancelar" },
        TranslationEntry{ TextId::SettingsLanguage,   "Idioma" },
        TranslationEntry{ TextId::SettingsFont,       "Fonte" },
        TranslationEntry{ TextId::SettingsFontSize,   "Tamanho da fonte" },
    };

    static constexpr std::array RussianEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Несовместимая версия игры! Требуется обновление" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Ближайший город: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Разломы" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Города" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Личные разломы" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Поиск" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Сортировать разломы по" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "По умолчанию" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Расстояние" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Открытие" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "По алфавиту" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Любимый город: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Не открыт)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Выбрать любимый город" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Включить кнопку телепорта в ближайший город" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Включить кнопку телепорта в любимый город" },
        TranslationEntry{ TextId::TabsTeleport, "Телепорт" },
        TranslationEntry{ TextId::TabsSettings, "Настройки" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Портал открыт!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Включить уведомление о портале Расколотого царства" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Позиция Y уведомления о портале Расколотого царства" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Восстанавливать состояние отслеживания заданий" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Включить телепортацию в город в удобное место" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Клавиша переключения меню:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Изменить" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Нажмите клавишу..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Отмена" },
        TranslationEntry{ TextId::SettingsLanguage,   "Язык" },
        TranslationEntry{ TextId::SettingsFont,       "Шрифт" },
        TranslationEntry{ TextId::SettingsFontSize,   "Размер шрифта" },
    };

    static constexpr std::array VietnameseEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "Phiên bản trò chơi không tương thích! Cần cập nhật" },
        TranslationEntry{ TextId::MainButtonNearestTown, "Thị trấn gần nhất: " },
        TranslationEntry{ TextId::MainTeleportTableRifts, "Cổng dịch chuyển" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "Thị trấn" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "Cổng dịch chuyển cá nhân" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "Tìm kiếm" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "Sắp xếp cổng theo" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "Mặc định" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "Khoảng cách" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "Đã khám phá" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "Theo bảng chữ cái" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "Thị trấn yêu thích: " },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, " (Chưa khám phá)" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "Chọn thị trấn yêu thích" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "Bật nút dịch chuyển đến thị trấn gần nhất" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "Bật nút dịch chuyển đến thị trấn yêu thích" },
        TranslationEntry{ TextId::TabsTeleport, "Dịch chuyển" },
        TranslationEntry{ TextId::TabsSettings, "Cài đặt" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "Đã mở cổng!" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "Bật thông báo cổng Shattered Realm" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "Vị trí Y thông báo cổng Shattered Realm" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "Khôi phục trạng thái theo dõi nhiệm vụ" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "Bật dịch chuyển về thành phố đến vị trí thuận tiện" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "Phím bật/tắt menu:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "Thay đổi" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "Nhấn một phím..." },
        TranslationEntry{ TextId::SettingsCancel,                          "Hủy" },
        TranslationEntry{ TextId::SettingsLanguage,   "Ngôn ngữ" },
        TranslationEntry{ TextId::SettingsFont,       "Phông chữ" },
        TranslationEntry{ TextId::SettingsFontSize,   "Cỡ chữ" },
    };

    static constexpr std::array ChineseEntries =
    {
        TranslationEntry{ TextId::MainTextUpdate, "游戏版本不兼容！需要更新" },
        TranslationEntry{ TextId::MainButtonNearestTown, "最近的城镇：" },
        TranslationEntry{ TextId::MainTeleportTableRifts, "裂隙之门" },
        TranslationEntry{ TextId::MainTeleportTableTowns, "城镇" },
        TranslationEntry{ TextId::MainTeleportTablePersonalRift, "个人裂隙之门" },
        TranslationEntry{ TextId::MainTeleportTableSearch, "搜索" },
        TranslationEntry{ TextId::MainTeleportTableSortBy, "裂隙之门排序方式" },
        TranslationEntry{ TextId::MainTeleportTableSortByDefault, "默认" },
        TranslationEntry{ TextId::MainTeleportTableSortByDistance, "距离" },
        TranslationEntry{ TextId::MainTeleportTableSortByDiscovery, "发现情况" },
        TranslationEntry{ TextId::MainTeleportTableSortByAlphabet, "按字母顺序" },
        TranslationEntry{ TextId::MainButtonFavoriteTown, "最喜欢的城镇：" },
        TranslationEntry{ TextId::SettingsTextFavoriteTownUndiscovered, "（未发现）" },
        TranslationEntry{ TextId::SettingsComboFavoriteTown, "选择最喜欢的城镇" },
        TranslationEntry{ TextId::SettingsCheckEnabledNearestTown, "启用最近城镇传送按钮" },
        TranslationEntry{ TextId::SettingsCheckEnabledFavoriteTown, "启用最喜欢城镇传送按钮" },
        TranslationEntry{ TextId::TabsTeleport, "传送" },
        TranslationEntry{ TextId::TabsSettings, "设置" },
        TranslationEntry{ TextId::GameEndlessDungeonNotifyPortal, "传送门已开启！" },
        TranslationEntry{ TextId::SettingsCheckEnabledEndlessDungeonNotifyPortal, "启用破碎领域传送门通知" },
        TranslationEntry{ TextId::SettingsSliderEndlessDungeonNotifyYPos, "破碎领域传送门通知 Y 位置" },
        TranslationEntry{ TextId::SettingsCheckEnabledQuestTrackRestore,   "恢复任务追踪状态" },
        TranslationEntry{ TextId::SettingsCheckEnabledConvenientTown,       "启用传送到城镇的便捷位置" },
        TranslationEntry{ TextId::SettingsTextChangeKeybind,              "菜单切换键:" },
        TranslationEntry{ TextId::SettingsButtonChangeKeybind,             "更改" },
        TranslationEntry{ TextId::SettingsTextChangeKeybindPressAnyKey,    "请按下一个按键..." },
        TranslationEntry{ TextId::SettingsCancel,                          "取消" },
        TranslationEntry{ TextId::SettingsLanguage,   "语言" },
        TranslationEntry{ TextId::SettingsFont,       "字体" },
        TranslationEntry{ TextId::SettingsFontSize,   "字号" },
    };

    static constexpr Language Languages[] =
    {
        { "English", EnglishEntries },
        { "Spanish", SpanishEntries },
        { "French", FrenchEntries },
        { "German", GermanEntries },
        { "Italian", ItalianEntries },
        { "Czech", CzechEntries },
        { "Japanese", JapaneseEntries },
        { "Korean", KoreanEntries },
        { "Polish", PolishEntries },
        { "Portuguese", PortugueseEntries },
        { "Russian", RussianEntries },
        { "Vietnamese", VietnameseEntries },
        { "Chinese", ChineseEntries },
    };

    constexpr std::array<std::string_view,
              static_cast<size_t>(TextId::Count)> Keys =
              {
#define X(id, key) key,
#include "localization_keys.inc"
#undef X
              };
}

bool Localization::Load(std::string_view language)
{
    Log("Loading localization language %.*s", static_cast<int>(language.size()), language.data());
    for (auto& text : m_table)
        text = "<missing>";
    std::bitset<Count> loaded;

    for (const auto& lang : Languages)
    {
        if (lang.name != language)
            continue;

        for (const auto& entry : lang.entries)
        {
            auto index = static_cast<size_t>(entry.id);
            m_table[index] = entry.value;
            loaded.set(index);
        }
        break;
    }

    if (language != "English")
    {
        for (const auto& lang : Languages)
        {
            if (lang.name != "English")
                continue;

            for (const auto& entry : lang.entries)
            {
                auto index = static_cast<size_t>(entry.id);
                if (!loaded.test(index))
                {
                    m_table[index] = entry.value;
                    loaded.set(index);
                }
            }
            break;
        }
    }

    Log("Loading language successfull");
    return true;
}
