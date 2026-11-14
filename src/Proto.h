//============================================================================================
// Proto.h - Funktionsprototypen:
//============================================================================================
#pragma once

#include "class.h"
#include "defines.h"
#include "Sbbm.h"
#include "sbl.h"
#include "TeakLibW.h"

#include <SDL_messagebox.h>
#include <cstdio>

#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------------------------------------
// AtNet.Cpp:
//--------------------------------------------------------------------------------------------
void SetNetworkBitmap(SLONG Number, SLONG WaitingType = 0);
void DisplayBroadcastMessage(CString String, SLONG FromPlayer = -1);
void PumpBroadcastBitmap(bool bJustForEmergency = false);
void PumpNetwork(void);
void NetWaitWatchdog(void);
CString GetMediumName(SLONG Medium);

//--------------------------------------------------------------------------------------------
// ColorFx.Cpp:
//--------------------------------------------------------------------------------------------
void RemapColor(SB_CBitmapCore *pBitmap, const CRect &HighRect, UWORD OldFontColor, ULONG NewFontColor);

//--------------------------------------------------------------------------------------------
// DxBm.Cpp:
//--------------------------------------------------------------------------------------------
void LoadLBMPalette(const char *Filename, SDL_Color *Palette);

//--------------------------------------------------------------------------------------------
// GameFrame.Cpp:
//--------------------------------------------------------------------------------------------
void MessagePump(void);
// GameEvents.cpp: optional events in the free game (see gGameplayExtras)
enum ATEventType { ATEVENT_NONE, ATEVENT_HOLIDAY, ATEVENT_FAIR, ATEVENT_KEROSIN_UP, ATEVENT_KEROSIN_DOWN, ATEVENT_STORM };
struct ATGameEvent {
    SLONG type{ATEVENT_NONE};
    SLONG start{0}; // first day
    SLONG days{0};
    SLONG city{-1}; // index into Cities (ATEVENT_FAIR)
};
bool AtExtrasActive(void);
bool AtEventOnDay(SLONG day, ATGameEvent &event);
bool AtEventStartingOn(SLONG day, ATGameEvent &event);
bool AtEventStormToday(void);
void AtEventsNewDay(void);
void AtEventAdjustOrderCities(ULONG &VonCity, ULONG &NachCity, TEAKRAND *pRandom);
SLONG AtEventAdjustPremium(SLONG Praemie, ULONG VonCity, ULONG NachCity);
CString AtEventHeadline(const ATGameEvent &event, bool announcement);
CString AtKerosinHeadline(void);
bool AtOrderForSmallPlane(TEAKRAND *pRandom);
bool AtOrderTooLarge(SLONG personen, SLONG distance, bool forSmallPlane);
void AtQuestNewDay(void);

// Deals with the computer players (GameRivals.cpp)
struct ATRival {
    SLONG pactUntil[4]{};   // no sabotage between the two airlines up to this day
    SLONG cartelUntil[4]{}; // price agreement up to this day
    SLONG lastSmear{-100};
    SLONG lastFuel{-100};
    SLONG lastOrderDeal{-100};
    SLONG questsDone{0}; // finished side jobs
};
extern ATRival gRival[4];
constexpr SLONG kAtPactDays = 7;
constexpr __int64 kAtPactPrice = 150000;
constexpr SLONG kAtCartelDays = 5;
constexpr SLONG kAtCartelPercent = 10;
constexpr SLONG kAtCartelRiskPercent = 8; // per day
constexpr __int64 kAtCartelFine = 250000;
constexpr SLONG kAtFuelRebatePercent = 15;
constexpr SLONG kAtOrderCommissionPercent = 30;
constexpr SLONG kAtOrderFeePercent = 15;
constexpr __int64 kAtSmearPrice = 50000;
void AtRivalReset(void);
void AtRivalSave(FILE *file);
void AtRivalLoad(FILE *file);
void AtRivalNewDay(void);
bool AtRivalPact(SLONG a, SLONG b);
bool AtRivalCartel(SLONG player);
CString AtRivalDealPact(PLAYER &qPlayer, SLONG rival, bool &ok);
CString AtRivalDealCartel(PLAYER &qPlayer, SLONG rival, bool &ok);
CString AtRivalDealFuel(PLAYER &qPlayer, SLONG rival, bool &ok);
__int64 AtRivalOrderCommission(const CAuftrag &order);
CString AtRivalDealGiveOrder(PLAYER &qPlayer, SLONG rival, SLONG index, bool &ok);
CAuftrag AtRivalOfferedOrder(SLONG rival);
CString AtRivalDealTakeOrder(PLAYER &qPlayer, SLONG rival, bool &ok);
CString AtRivalDealSmear(PLAYER &qPlayer, SLONG rival, SLONG target, bool &ok);
CString AtStatCalcLabel(SLONG code);
CString AtStatCalc(SLONG player, SLONG viewer, SLONG code);
void AtImageNewDay(void);
CString AtBriefing(SLONG part);
CString AtImageHelp(void);

const char *AtText(const char *german, const char *english);
SLONG AtPlanerCarrying(void);
SLONG AtAutoPlanOrders(void);

// GameFinance.cpp: optional bank extensions (fixed deposit, term loan, short sales)
class PLAYER;
struct ATFinance {
    __int64 deposit{0}; // fixed deposit, paid back with interest on depositDue
    SLONG depositDue{0};
    __int64 loan{0}; // term loan, repaid with interest on loanDue
    SLONG loanDue{0};
    SLONG shortShares[4]{}; // short sales per airline
    double shortPrice[4]{};
    SLONG shortDue[4]{};
};
extern ATFinance gFinance[4];
constexpr SLONG kAtDepositDays = 7;
constexpr SLONG kAtDepositPercent = 2;
constexpr SLONG kAtLoanDays = 20;
constexpr SLONG kAtLoanPercent = 3;
constexpr SLONG kAtShortDays = 5;
void AtFinanceReset(void);
void AtFinanceSave(SLONG slot);
void AtFinanceLoad(SLONG slot);
void AtFinanceNewDay(void);
bool AtFinanceOpenDeposit(PLAYER &qPlayer, __int64 amount);
bool AtFinanceOpenLoan(PLAYER &qPlayer, __int64 amount);
bool AtFinanceOpenShort(PLAYER &qPlayer, SLONG target, SLONG shares);
__int64 AtFinanceCloseShort(PLAYER &qPlayer, SLONG target);
CString AtFinanceSummary(SLONG player);
CString AtStockHeadline(void);
CString AtTakeoverHeadline(void);

extern bool gMainLoopPump; // MessagePump() is being called by the main loop, not from inside a room or a loading screen
void AtSaveOnBackground(void);
bool AtQuickSave(void); // slot 12, which the options menu does not show
bool AtQuickLoad(void);
void AtSetTextEntry(SLONG source, bool wanted); // Android: on-screen keyboard while a text field is active
void SetToolTip(SLONG TipId, XY Pos);
void SetMouseLook(SLONG Look, SLONG TipId, SLONG ClickArea, SLONG ClickId, SLONG ClickPar1 = 0, SLONG ClickPar2 = 0);
void SetMouseLook(SLONG Look, SLONG TipId, const CString &Description, SLONG ClickArea, SLONG ClickId, SLONG ClickPar1 = 0, SLONG ClickPar2 = 0);
void DefaultOnLButtonDown(void);
void DefaultOnLButtonUp(void);
void DefaultOnRButtonDown(void);
void DefaultOnRButtonUp(void);

//--------------------------------------------------------------------------------------------
// HLines.Cpp:
//--------------------------------------------------------------------------------------------
void UpdateHLinePool(void);

//--------------------------------------------------------------------------------------------
// Init.Cpp:
//--------------------------------------------------------------------------------------------
void InitSoundSystem(SDL_Window *AppWnd);
void PaintStarAt(SBBM &Bitmap, XY Pos);
void InitPathVars(void);
void InitFonts(void);
void InitStatusLines(void);
void InitItems(void);
void InitTipBms(void);
void InitGlobeMapper(void);

//--------------------------------------------------------------------------------------------
// Misc.Cpp:
//--------------------------------------------------------------------------------------------
SLONG ReadLine(BUFFER_V<UBYTE> &Buffer, SLONG BufferStart, char *Line, SLONG LineLength);
CString RemoveAccents(CString str);
SLONG CountLines(BUFFER_V<UBYTE> &Buffer, SLONG BufferStart);
SLONG GetAnzBits(ULONG Flags);
DOUBLE GetAlphaFromXY(XY Delta);
void DrawChart(SBBM &TipBm, ULONG Color, const BUFFER_V<SLONG> &Values, SLONG MinValue, SLONG MaxValue, const XY &TopLeft, const XY &TopRight,
               const XY &BottomLeft, const XY &BottomRight);
CString KorrigiereUmlaute(CString &Originaltext);
// SLONG   ReadTimeStampCounter (void);
__int64 StringToInt64(const CString &String);
BOOL IsRoomBusy(UWORD RoomId, SLONG ExceptPlayer);
BOOL EarthProjectize(const XY &NaturalPos, UWORD EarthAlpha, XY *PixelPos);
BOOL EarthProjectize(const FXY &NaturalPos, UWORD EarthAlpha, XY *PixelPos);
SLONG AddToNthDigit(SLONG Value, SLONG Digit, SLONG Add);

#define MESSAGEBOX_ERROR SDL_MESSAGEBOX_ERROR
#define MESSAGEBOX_INFO SDL_MESSAGEBOX_INFORMATION
void SimpleMessageBox(ULONG Type, LPCTSTR Title, LPCTSTR String, ...);
int AbortMessageBox(ULONG Type, LPCTSTR Title, LPCTSTR String, ...);

CString ConvertDate2String(UWORD Date);
UWORD ConvertString2Date(char *String);
void DoAppPath(void);
fs::path BuildPathCaseInsensitive(const fs::path &PathString, const fs::path &Filename);
fs::path FullFilesystemPath(const fs::path &Filename, const fs::path &PathString);
CString FullFilename(const CString &Filename, const CString &PathString);
CString FullFilename(const CString &Filename, const CString &PathString, SLONG Num);
SLONG CalculateFlightKerosin(SLONG VonCity, SLONG NachCity, SLONG Verbrauch, SLONG Geschwindigkeit);
SLONG CalculateFlightCost(SLONG VonCity, SLONG NachCity, SLONG Verbrauch, SLONG Geschwindigkeit, SLONG Player);
SLONG CalculateFlightCostRechnerisch(SLONG VonCity, SLONG NachCity, SLONG Verbrauch, SLONG Geschwindigkeit, SLONG PlayerNum);
SLONG CalculateFlightCostNoTank(SLONG VonCity, SLONG NachCity, SLONG Verbrauch, SLONG Geschwindigkeit);
// Ticket price presets of the route window (0 = cheapest ... ROUTE_PRICE_PRESET_COUNT-1 = most expensive)
const SLONG ROUTE_PRICE_PRESET_COUNT = 4;
const SLONG ROUTE_PRICE_PRESET_DEFAULT = 1;
SLONG CalculateRoutePricePreset(SLONG VonCity, SLONG NachCity, SLONG Preset);
SLONG CalculateRoutePricePresetFC(SLONG VonCity, SLONG NachCity, SLONG Preset);
void InitEinheiten(const CString &Filename);
CString Insert1000erDots(SLONG Value);
CString Insert1000erDots64(__int64 Value);
CString Shorten1000erDots(SLONG Value);
CRect PaintTextBubble(SBBM &OffscreenBm, const XY &p1, const XY &p2, const XY &Entry);
BOOL CheckCursorHighlight(const CRect &rect, UWORD FontColor, SLONG Look = CURSOR_HOT, SLONG TipId = 0, SLONG ClickArea = 0, SLONG ClickId = 0,
                          SLONG ClickPar1 = 0, SLONG ClickPar2 = 0);
BOOL CheckCursorHighlight(const XY &CursorPos, const CRect &rect, UWORD FontColor, SLONG Look = CURSOR_HOT, SLONG TipId = 0, SLONG ClickArea = 0,
                          SLONG ClickId = 0, SLONG ClickPar1 = 0, SLONG ClickPar2 = 0);
void CheckCString(CString *String);
void GetMatchingFilelist(const CString &DirAndWildcards, BUFFER_V<CString> &Array);
CString GetMatchingNext(const CString &DirAndWildcards, const CString &CurrentFilename, SLONG Add = 1);
SLONG CountMatchingFilelist(const CString &DirAndWildcards);
CString CreateNumeratedFreeFilename(const CString &DirAndFilename);
CString GetFilenameFromFullFilename(CString FullFilename);
#ifndef WIN32
BOOL OffsetRect(RECT *pRect, SLONG dx, SLONG dy);
#endif
DWORD AtGetTime(void);
DWORD AtGetSeedTime(void);
SLONG AtGetAsyncKeyState(SLONG vKey);
DWORD AtGetTickCount(void);
CString getCurrentDayString();
SLONG strchrcount(CString Text, char chr);
SLONG strchrcount(char *str, const char delimiters[]);
void printPostGameInfo();
//--------------------------------------------------------------------------------------------
// Zählt die Häufigkeit des Auftretens von Enumerationswerten in einem Zeichenfolgenwert,
// basierend auf einer gegebenen Zuordnung von Zeichenfolgen zu Enumerationswerten.
//--------------------------------------------------------------------------------------------
template <typename TEnum>
std::vector<TEnum> findEnumsInString(const std::unordered_map<std::string, TEnum> &enumMap, const std::string &value, const std::string &delimiters) {
    std::vector<TEnum> foundEnums;

    size_t start = 0, end = 0;
    while ((end = value.find_first_of(delimiters, start)) != std::string::npos) {
        std::string token = value.substr(start, end - start);
        if (!token.empty()) {
            auto it = enumMap.find(token);
            if (it != enumMap.end()) {
                foundEnums.push_back(it->second);
            }
        }
        start = end + 1;
    }

    std::string lastToken = value.substr(start);
    if (!lastToken.empty()) {
        auto it = enumMap.find(lastToken);
        if (it != enumMap.end()) {
            foundEnums.push_back(it->second);
        }
    }

    return foundEnums;
}

//--------------------------------------------------------------------------------------------
// Planer.Cpp:
//--------------------------------------------------------------------------------------------
void PaintGlobe(const TECBM &SourceBm, SBBM *TargetBm, UWORD EarthAlpha, const XY &Offset);

//--------------------------------------------------------------------------------------------
// Player.Cpp:
//--------------------------------------------------------------------------------------------
bool GlobalUse(SLONG FeatureId);

//--------------------------------------------------------------------------------------------
// SBBM.Cpp:
//--------------------------------------------------------------------------------------------
__int64 GetIdFromString(CString Texts);

//--------------------------------------------------------------------------------------------
// SmackPrs.Cpp:
//--------------------------------------------------------------------------------------------
void CalculatePalettemapper(UBYTE *pPalette, UWORD *pPaletteMapper);
void ConvertBitmapTo16Bit(UBYTE *SourcePic, SBBM *pBitmap, UWORD *pPaletteMapper);

//--------------------------------------------------------------------------------------------
// Stdraum.Cpp:
//--------------------------------------------------------------------------------------------
void UpdateStatusBar(void);
void SetRoomVisited(SLONG PlayerNum, UBYTE RoomId);
BOOL WasRoomVisited(SLONG PlayerNum, UBYTE RoomId);
BOOL WasRoomNumberVisited(SLONG PlayerNum, SLONG RoomNumber);
UBYTE GetRoomIdFromNumber(SLONG PlayerNum, SLONG Number);

//--------------------------------------------------------------------------------------------
// Sound.Cpp:
//--------------------------------------------------------------------------------------------
CString RemoveSpeechFilename(CString String);
CString GetSpeechFilename(const CString &String, SLONG Index = 0, CString *pTextFollows = NULL);
BOOL CreateSpeechSBFX(const CString &String, SBFX *pFx, SLONG Player, BOOL *bAnyMissing = NULL);

void PlayUniversalFx(const CString &Filename, SLONG Volume);
void PlayFanfare(void);
SLONG Prozent2Dezibel(SLONG Prozent);
BOOL IsMidiAvailable(void);
void ScanMusicTracks(void);
SLONG GetNumMusicTracks(void);
SLONG GetSelectedMusicTrack(void);
CString GetMusicTrackName(SLONG Index);
void NextMidi(void);
void PlayMidi(const CString &Filename);
void PlayMidiFrom(const CString &Filename, SLONG StartPosition);
void StopMidi(void);
void SetMidiVolume(SLONG Volume);
void SetWaveVolume(SLONG volume);
void PauseMidi(void);
void ResumeMidi(void);

//--------------------------------------------------------------------------------------------
// Tips.Cpp:
//--------------------------------------------------------------------------------------------
void DrawAutoflugTip(SBBM &TipBm, SLONG Costs, SLONG VonCity, SLONG NachCity);
void DrawCityTip(SBBM &TipBm, ULONG CityId);
void DrawPlaneTip(SBBM &TipBm, SBBM *pTipOriginBm, CPlaneType *PlaneType, CPlane *Plane);
void DrawRouteTip(SBBM &TipBm, SLONG NumPlayer, ULONG RouteId, SLONG Passagiere, SLONG Costs);
void DrawAuftragTip(SLONG Player, SBBM &TipBm, SBBMS *pPlaneTips, SBBM *pTipOriginBm, CAuftrag *Auftrag, SLONG Costs, SLONG Okay, XY MapOffset);
void DrawKursTipContents(SBBM &TipBm, SLONG PlayerView, SLONG PlayerAktie, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawItemTipContents(SBBM &TipBm, SLONG Item, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawMoneyTip(SBBM &TipBm, SLONG PlayerNum, SLONG Page);
void DrawTankTip(SBBM &TipBm, SLONG PlayerNum);
void DrawPlanesNotepad(SBBM &NotepadBm, CDataTable *Table, SLONG CountFrom, SBBMS *pMenuBms, BOOL DisplayWartungskosten = FALSE);
void DrawFrachtTip(SLONG Player, SBBM &TipBm, SBBMS *pPlaneTips, SBBM *pTipOriginBm, CFracht *Fracht, SLONG TonsThis, SLONG Costs, SLONG Income, SLONG Okay,
                   XY MapOffset);
void DrawFrachtTipContents(SLONG PlayerNum, SBBM &TipBm, SBBMS *pPlaneTips, CFracht *Fracht, SLONG TonsThis, SLONG Costs, SLONG Income, SLONG Okay, XY Headline,
                           XY Contents, XY MapOffset, SB_CFont *pHeadFont, SB_CFont *pFont, BOOL Unlocked = TRUE);

void DrawAutoflugTipContents(SBBM &TipBm, SLONG Costs, SLONG NotPassengers, SLONG VonCity, SLONG NachCity, XY Headline, XY Contents, SB_CFont *pHeadFont,
                             SB_CFont *pFont, BOOL Unlocked);
void DrawCityTipContents(SBBM &TipBm, ULONG CityId, XY Headline, XY Contents, XY Map, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawPlaneTipContents(SBBM &TipBm, CPlaneType *PlaneType, CPlane *Plane, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont,
                          BOOL OwnPlane = FALSE, SLONG OwnedByComputer = FALSE);
void DrawXPlaneTipContents(SBBM &TipBm, const CString &Planename, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawRouteTipContents(SBBM &TipBm, SLONG PlayerNum, ULONG RouteId, SLONG Gate, SLONG Passagiere, SLONG PassagiereFC, SLONG Costs, SLONG Ticketpreis,
                          SLONG TicketpreisFC, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont, BOOL Unlocked = TRUE);
void DrawAuftragTipContents(SLONG Player, SBBM &TipBm, SBBMS *pPlaneTips, CAuftrag *Auftrag, SLONG Gate, SLONG Costs, SLONG Okay, XY Headline, XY Contents,
                            XY MapOffset, SB_CFont *pHeadFont, SB_CFont *pFont, BOOL Unlocked = TRUE);
void DrawKursTipContents(SBBM &TipBm, SLONG PlayerView, SLONG PlayerAktie, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawItemTipContents(SBBM &TipBm, SLONG Item, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawMoneyTipContents(SBBM &TipBm, SLONG PlayerNum, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont);
void DrawTankTipContents(SBBM &TipBm, SLONG PlayerNum, XY Headline, XY Contents, SB_CFont *pHeadFont, SB_CFont *pFont);
