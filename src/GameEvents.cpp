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

//============================================================================================
// Side jobs in the free game. Every few days somebody calls the player with a job for the next
// three days. Nothing has to be accepted and nothing is lost if it fails; a finished job pays
// a bonus and improves the image.
//
// Like the events, the jobs are not stored: which job runs when follows from the start time of
// the game, the goal from what the airline did in the days before, and the progress from the
// statistics the game keeps anyway.
//============================================================================================
namespace {
constexpr SLONG kQuestBlock = 5; // a new job every five days
constexpr SLONG kQuestDays = 3;  // days to finish it

struct ATQuest {
    SLONG type{0};   // 0: passengers, 1: flights, 2: new orders
    SLONG caller{0}; // who is calling
    SLONG start{0};  // first day
    SLONG end{0};    // last day
    SLONG stat{STAT_PASSAGIERE};
};

ATQuest QuestOfBlock(SLONG block) {
    ATQuest quest;
    TEAKRAND rnd(static_cast<ULONG>(Sim.StartTime) + static_cast<ULONG>(block) * 104729U + 4711U);
    rnd.Rand(100);
    rnd.Rand(100);
    quest.type = rnd.Rand(3);
    quest.caller = rnd.Rand(4);
    quest.start = block * kQuestBlock + 2 + rnd.Rand(2);
    quest.end = quest.start + kQuestDays - 1;
    quest.stat = (quest.type == 0) ? STAT_PASSAGIERE : (quest.type == 1) ? STAT_FLUEGE : STAT_AUFTRAEGE;
    return quest;
}

// The total at the end of the given day. This runs at the start of day Sim.Date, before the
// statistics get their entry for the new day.
__int64 QuestTotal(PLAYER &qPlayer, SLONG stat, SLONG day) {
    const SLONG back = Sim.Date - 1 - day;
    if (back < 0 || day < 0) {
        return 0;
    }
    return qPlayer.Statistiken[stat].GetAtPastDay(back);
}

__int64 QuestGoal(PLAYER &qPlayer, const ATQuest &quest) {
    // what the airline did in the three days before the call, plus a third
    const __int64 before = QuestTotal(qPlayer, quest.stat, quest.start - 1) - QuestTotal(qPlayer, quest.stat, quest.start - 4);
    __int64 goal = std::max<__int64>(0, before) * 4 / 3;
    if (quest.type == 0) {
        goal = std::max<__int64>(250, (goal + 9) / 10 * 10);
    } else if (quest.type == 1) {
        goal = std::max<__int64>(6, goal + 1);
    } else {
        goal = std::max<__int64>(4, goal + 1);
    }
    return goal;
}

__int64 QuestReward(const ATQuest &quest, __int64 goal) {
    const __int64 perUnit = (quest.type == 0) ? 150 : (quest.type == 1) ? 12000 : 10000;
    return 40000 + goal * perUnit;
}

const char *QuestCaller(SLONG caller) {
    switch (caller) {
    case 0:
        return AtText("Reiseb\374ro Fernweh", "the Wanderlust travel agency");
    case 1:
        return AtText("Ein Messeveranstalter", "A trade fair organiser");
    case 2:
        return AtText("Die Flughafenleitung", "The airport management");
    default:
        return AtText("Ein Reiseveranstalter", "A tour operator");
    }
}

CString QuestTask(const ATQuest &quest, __int64 goal) {
    const char *text = (quest.type == 0)   ? AtText("%li Passagiere bef\366rdern", "carry %li passengers")
                       : (quest.type == 1) ? AtText("%li Fl\374ge durchf\374hren", "complete %li flights")
                                           : AtText("%li neue Auftr\344ge annehmen", "accept %li new orders");
    return bprintf(text, static_cast<long>(goal));
}
} // namespace

void AtQuestNewDay() {
    if (!AtExtrasActive() || Sim.localPlayer < 0 || Sim.localPlayer >= Sim.Players.Players.AnzEntries()) {
        return;
    }
    PLAYER &qPlayer = Sim.Players.Players[Sim.localPlayer];
    if (qPlayer.IsOut != 0) {
        return;
    }

    // the job that ended yesterday
    for (SLONG block = Sim.Date / kQuestBlock; block >= 0 && block >= Sim.Date / kQuestBlock - 1; block--) {
        const ATQuest quest = QuestOfBlock(block);
        if (quest.end + 1 != Sim.Date) {
            continue;
        }
        const __int64 goal = QuestGoal(qPlayer, quest);
        const __int64 done = QuestTotal(qPlayer, quest.stat, quest.end) - QuestTotal(qPlayer, quest.stat, quest.start - 1);
        if (done >= goal) {
            const __int64 reward = QuestReward(quest, goal);
            qPlayer.ChangeMoney(reward, 3607, "");
            gRival[Sim.localPlayer].questsDone++;
            qPlayer.Image = std::min<SLONG>(1000, qPlayer.Image + 15);
            qPlayer.Messages.AddMessage(BERATERTYP_GIRL,
                                        bprintf(AtText("Nebenauftrag erf\374llt! %s zahlt %s und empfiehlt uns weiter.", "Side job done! %s pays %s and recommends us."),
                                                QuestCaller(quest.caller), (LPCTSTR)Insert1000erDots64(reward)),
                                        MESSAGE_URGENT);
        } else {
            qPlayer.Messages.AddMessage(BERATERTYP_GIRL,
                                        bprintf(AtText("Nebenauftrag nicht geschafft: %li von %li. Eine Strafe gibt es nicht.", "Side job not finished: %li of %li. There is no penalty."),
                                                static_cast<long>(std::max<__int64>(0, done)), static_cast<long>(goal)),
                                        MESSAGE_URGENT);
        }
    }

    // a job that starts or runs today
    const ATQuest quest = QuestOfBlock(Sim.Date / kQuestBlock);
    if (Sim.Date < quest.start || Sim.Date > quest.end) {
        return;
    }
    const __int64 goal = QuestGoal(qPlayer, quest);
    if (Sim.Date == quest.start) {
        qPlayer.Messages.AddMessage(BERATERTYP_GIRL,
                                    bprintf(AtText("Anruf f\374r Sie: %s bietet einen Nebenauftrag. In %li Tagen %s - Bonus %s.", "A call for you: %s offers a side job. Within %li days %s - bonus %s."),
                                            QuestCaller(quest.caller), static_cast<long>(kQuestDays), (LPCTSTR)QuestTask(quest, goal),
                                            (LPCTSTR)Insert1000erDots64(QuestReward(quest, goal))),
                                    MESSAGE_URGENT);
    } else {
        const __int64 done = QuestTotal(qPlayer, quest.stat, Sim.Date - 1) - QuestTotal(qPlayer, quest.stat, quest.start - 1);
        qPlayer.Messages.AddMessage(BERATERTYP_GIRL,
                                    bprintf(AtText("Nebenauftrag: %li von %li, noch %li Tage.", "Side job: %li of %li, %li days left."),
                                            static_cast<long>(std::max<__int64>(0, done)), static_cast<long>(goal),
                                            static_cast<long>(quest.end - Sim.Date + 1)),
                                    MESSAGE_NORMAL);
    }
}

//============================================================================================
// Image in the free game: besides advertising, the image follows how satisfied the passengers
// of the day before were. The secretary says what moved it.
//============================================================================================
void AtImageNewDay() {
    if (!AtExtrasActive()) {
        return;
    }
    // runs before the statistics get their entry for the new day: 0 = yesterday, 1 = the day before
    for (SLONG c = 0; c < Sim.Players.Players.AnzEntries(); c++) {
        PLAYER &qPlayer = Sim.Players.Players[c];
        if (qPlayer.IsOut != 0) {
            continue;
        }
        const __int64 happy = qPlayer.Statistiken[STAT_ZUFR_PASSAGIERE].GetAtPastDay(0) - qPlayer.Statistiken[STAT_ZUFR_PASSAGIERE].GetAtPastDay(1);
        const __int64 unhappy = qPlayer.Statistiken[STAT_UNZUFR_PASSAGIERE].GetAtPastDay(0) - qPlayer.Statistiken[STAT_UNZUFR_PASSAGIERE].GetAtPastDay(1);
        if (happy < 0 || unhappy < 0 || happy + unhappy < 50) {
            continue; // too few passengers to talk about
        }
        const SLONG share = static_cast<SLONG>(happy * 100 / (happy + unhappy));
        // Satisfaction lifts the image slowly and only up to a level that depends on the share:
        // 80 percent carry it to 100, 90 percent to 300, 100 percent to 500. Beyond that it takes advertising.
        const SLONG ceiling = (share - 75) * 20;
        SLONG delta = 0;
        if (share < 40) {
            delta = -4;
        } else if (share < 55) {
            delta = -2;
        } else if (qPlayer.Image < ceiling) {
            delta = (share >= 95) ? 2 : 1;
        }
        if (delta == 0) {
            continue;
        }
        qPlayer.Image += delta;
        Limit(SLONG(-1000), qPlayer.Image, SLONG(1000));
        if (c == Sim.localPlayer) {
            qPlayer.Messages.AddMessage(BERATERTYP_GIRL,
                                        bprintf(AtText("Image %s%li: gestern waren %li Prozent der Passagiere zufrieden. Image jetzt %li von 1000.", "Image %s%li: %li percent of yesterday's passengers were satisfied. Image now %li of 1000."),
                                                (delta > 0) ? "+" : "", static_cast<long>(delta), static_cast<long>(share), static_cast<long>(qPlayer.Image)),
                                        MESSAGE_NORMAL);
        }
    }
}

// What the advertising agency tells about the image
CString AtImageHelp() {
    return AtText("Das Image reicht von -1000 bis 1000. Es steigt durch Werbekampagnen, durch zufriedene Passagiere (langsam und je nach Quote nur bis h\366chstens 500), gutes Personal an Bord und erf\374llte Nebenauftr\344ge. Es sinkt durch unzufriedene Passagiere, zu hohe Ticketpreise, Pannen und Sabotage.", "The image ranges from -1000 to 1000. It rises with advertising campaigns, satisfied passengers (slowly, and depending on the share only up to 500 at most), good staff on board and finished side jobs. It falls with unhappy passengers, overpriced tickets, breakdowns and sabotage.");
}
