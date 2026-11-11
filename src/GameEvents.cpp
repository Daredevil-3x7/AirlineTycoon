//============================================================================================
// GameEvents.cpp : Optional events in the free game: holidays, trade fairs, kerosene price
// shocks and storms. They are announced in the newspapers one day ahead.
//
// The events are not stored anywhere. Which event happens when follows from the start time of
// the game alone, so savegames stay compatible and a loaded game continues with the same events.
// Everything here is inactive unless gGameplayExtras is set, and in missions and network games.
//============================================================================================
#include "global.h"
#include "Proto.h"

namespace {
constexpr SLONG kDaysPerBlock = 6; // at most one event in every block of days

ATGameEvent EventOfBlock(SLONG block) {
    ATGameEvent event;
    if (block < 0 || Cities.AnzEntries() <= 0) {
        return event;
    }

    TEAKRAND rnd(static_cast<ULONG>(Sim.StartTime) + static_cast<ULONG>(block) * 7919U + 17U);
    rnd.Rand(100); // the first numbers of neighbouring seeds are alike
    rnd.Rand(100);

    const SLONG kind = rnd.Rand(100);
    if (kind < 30) {
        event.type = ATEVENT_HOLIDAY;
        event.days = 3;
    } else if (kind < 55) {
        event.type = ATEVENT_FAIR;
        event.days = 2;
    } else if (kind < 70) {
        event.type = ATEVENT_KEROSIN_UP;
        event.days = 1;
    } else if (kind < 80) {
        event.type = ATEVENT_KEROSIN_DOWN;
        event.days = 1;
    } else {
        event.type = ATEVENT_STORM;
        event.days = 1;
    }

    // never on the first two days of a game, and always announced a day ahead
    event.start = block * kDaysPerBlock + 1 + rnd.Rand(3) + ((block == 0) ? 1 : 0);

    event.city = Cities.GetRandomUsedIndex(1 + rnd.Rand(4), &rnd);
    if (event.city < 0 || event.city >= Cities.AnzEntries() || Cities.GetIdFromIndex(event.city) == static_cast<ULONG>(Sim.HomeAirportId)) {
        if (event.type == ATEVENT_FAIR) {
            event.type = ATEVENT_HOLIDAY; // no suitable city this time
            event.days = 3;
        }
        event.city = -1;
    }
    return event;
}
} // namespace

const char *AtText(const char *german, const char *english) { return (gLanguage == LANGUAGE_D) ? german : english; }

bool AtExtrasActive() { return gGameplayExtras && Sim.bNetwork == 0 && Sim.Difficulty == DIFF_FREEGAME; }

bool AtEventOnDay(SLONG day, ATGameEvent &event) {
    if (!AtExtrasActive() || day < 0) {
        return false;
    }
    event = EventOfBlock(day / kDaysPerBlock);
    return event.type != ATEVENT_NONE && day >= event.start && day < event.start + event.days;
}

bool AtEventStartingOn(SLONG day, ATGameEvent &event) { return AtEventOnDay(day, event) && event.start == day; }

bool AtEventStormToday() {
    ATGameEvent event;
    return AtEventOnDay(Sim.Date, event) && event.type == ATEVENT_STORM;
}

//--------------------------------------------------------------------------------------------
// Called once at the start of a day, after the regular kerosene price was computed
//--------------------------------------------------------------------------------------------
void AtEventsNewDay() {
    ATGameEvent event;
    if (!AtEventStartingOn(Sim.Date, event)) {
        return;
    }

    if (event.type == ATEVENT_KEROSIN_UP) {
        Sim.Kerosin += Sim.Kerosin * 3 / 10;
    } else if (event.type == ATEVENT_KEROSIN_DOWN) {
        Sim.Kerosin -= Sim.Kerosin / 4;
    }
    Limit(SLONG(300), Sim.Kerosin, SLONG(700));

    // the stock market reacts to the news
    double factor = 1.0;
    switch (event.type) {
    case ATEVENT_KEROSIN_UP:
        factor = 0.95;
        break;
    case ATEVENT_KEROSIN_DOWN:
        factor = 1.03;
        break;
    case ATEVENT_STORM:
        factor = 0.98;
        break;
    case ATEVENT_HOLIDAY:
    case ATEVENT_FAIR:
        factor = 1.02;
        break;
    default:
        break;
    }
    for (SLONG c = 0; c < Sim.Players.Players.AnzEntries(); c++) {
        if (Sim.Players.Players[c].IsOut == 0) {
            Sim.Players.Players[c].Kurse[0] *= factor;
        }
    }

    if (Sim.localPlayer >= 0 && Sim.localPlayer < Sim.Players.Players.AnzEntries()) {
        Sim.Players.Players[Sim.localPlayer].Messages.AddMessage(BERATERTYP_GIRL, AtEventHeadline(event, false), MESSAGE_URGENT);
    }
}

//--------------------------------------------------------------------------------------------
// New flight orders: during a trade fair every third order goes to the city of the fair
//--------------------------------------------------------------------------------------------
void AtEventAdjustOrderCities(ULONG &VonCity, ULONG &NachCity, TEAKRAND *pRandom) {
    ATGameEvent event;
    if (!AtEventOnDay(Sim.Date, event) || event.type != ATEVENT_FAIR || event.city < 0 || pRandom == nullptr) {
        return;
    }
    if (pRandom->Rand(3) == 0) {
        const ULONG fairCity = Cities.GetIdFromIndex(event.city);
        if (VonCity != fairCity) {
            NachCity = fairCity;
        }
    }
}

SLONG AtEventAdjustPremium(SLONG Praemie, ULONG VonCity, ULONG NachCity) {
    ATGameEvent event;
    if (!AtEventOnDay(Sim.Date, event)) {
        return Praemie;
    }
    if (event.type == ATEVENT_HOLIDAY) {
        return Praemie * 5 / 4;
    }
    if (event.type == ATEVENT_FAIR && event.city >= 0) {
        const ULONG fairCity = Cities.GetIdFromIndex(event.city);
        if (VonCity == fairCity || NachCity == fairCity) {
            return Praemie * 3 / 2;
        }
    }
    return Praemie;
}

//--------------------------------------------------------------------------------------------
// Newspaper texts
//--------------------------------------------------------------------------------------------
CString AtEventHeadline(const ATGameEvent &event, bool announcement) {
    CString city;
    if (event.city >= 0 && event.city < Cities.AnzEntries()) {
        city = Cities[event.city].Name;
    }

    switch (event.type) {
    case ATEVENT_HOLIDAY:
        return CString(announcement ? AtText("Ferien beginnen morgen: Reiseb\374ros rechnen mit Ansturm und h\366heren Pr\344mien", "Holidays start tomorrow: travel agencies expect a rush and higher premiums") : AtText("Ferienzeit: Reiseauftr\344ge bringen jetzt ein Viertel mehr", "Holiday season: flight orders now pay a quarter more"));
    case ATEVENT_FAIR:
        return CString(bprintf(announcement ? AtText("Morgen beginnt die Messe in %s - Fl\374ge dorthin sind gefragt", "The trade fair in %s opens tomorrow - flights there are in demand")
                                            : AtText("Messe in %s: Auftr\344ge von und nach dort bringen die H\344lfte mehr", "Trade fair in %s: orders from and to the city pay half as much again"),
                               (LPCTSTR)city));
    case ATEVENT_KEROSIN_UP:
        return CString(announcement ? AtText("Experten warnen: Kerosin wird morgen deutlich teurer", "Experts warn: kerosene will be much more expensive tomorrow") : AtText("\326lkrise: Der Kerosinpreis ist sprunghaft gestiegen", "Oil crisis: the price of kerosene has jumped"));
    case ATEVENT_KEROSIN_DOWN:
        return CString(announcement ? AtText("\334berangebot erwartet: Kerosin wird morgen billiger", "Oversupply expected: kerosene will be cheaper tomorrow") : AtText("Kerosin ist heute deutlich billiger - Tanks f\374llen lohnt sich", "Kerosene is much cheaper today - a good day to fill the tanks"));
    case ATEVENT_STORM:
        return CString(announcement ? AtText("Sturmwarnung f\374r morgen: Schlecht ausger\374stete Maschinen sind gef\344hrdet", "Storm warning for tomorrow: poorly equipped planes are at risk") : AtText("Sturmtief \374ber dem Flughafen: Das Pannenrisiko ist stark erh\366ht", "Storm over the airport: the risk of breakdowns is much higher"));
    default:
        return CString();
    }
}

CString AtKerosinHeadline() {
    const SLONG today = Sim.Kerosin;
    const SLONG before = Sim.KerosinPast[8];
    const char *format = AtText("Kerosin heute bei %li - Preis stabil", "Kerosene at %li today - price steady");
    if (today > before + 15) {
        format = AtText("Kerosin heute bei %li - Tendenz steigend", "Kerosene at %li today - rising");
    } else if (today < before - 15) {
        format = AtText("Kerosin heute bei %li - Tendenz fallend", "Kerosene at %li today - falling");
    }
    return CString(bprintf(format, static_cast<long>(today)));
}
