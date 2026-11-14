//============================================================================================
// GameRivals.cpp : Optional deals with the computer players in the free game (pact, price
// agreement, joint fuel purchase, passing orders on, spreading rumours) and the extra figures
// of the statistics screen.
//
// The state is stored at the end of the sidecar file of the bank extensions (game<N>.fin).
// Everything here is inactive unless gGameplayExtras is set, and in missions and network games.
//============================================================================================
#include "GameMechanic.h"
#include "global.h"
#include "Proto.h"

ATRival gRival[4];

namespace {
bool ValidPlayer(SLONG p) { return p >= 0 && p < 4 && p < Sim.Players.Players.AnzEntries(); }

void Tell(SLONG player, const CString &text) {
    if (player == Sim.localPlayer && ValidPlayer(player)) {
        Sim.Players.Players[player].Messages.AddMessage(BERATERTYP_GIRL, text, MESSAGE_URGENT);
    }
}

SLONG CountRoutes(PLAYER &qPlayer) {
    SLONG count = 0;
    for (SLONG c = 0; c < qPlayer.RentRouten.RentRouten.AnzEntries(); c++) {
        if (Routen.IsInAlbum(c) != 0 && qPlayer.RentRouten.RentRouten[c].Rang != 0U) {
            count++;
        }
    }
    return count;
}
} // namespace

void AtRivalReset() {
    for (auto &r : gRival) {
        r = ATRival();
    }
}

void AtRivalSave(FILE *file) {
    fprintf(file, "RIVAL 1\n");
    for (const auto &r : gRival) {
        fprintf(file, "%d %d %d %d %d %d %d %d %d %d %d %d\n", static_cast<int>(r.pactUntil[0]), static_cast<int>(r.pactUntil[1]),
                static_cast<int>(r.pactUntil[2]), static_cast<int>(r.pactUntil[3]), static_cast<int>(r.cartelUntil[0]), static_cast<int>(r.cartelUntil[1]),
                static_cast<int>(r.cartelUntil[2]), static_cast<int>(r.cartelUntil[3]), static_cast<int>(r.lastSmear), static_cast<int>(r.lastFuel),
                static_cast<int>(r.lastOrderDeal), static_cast<int>(r.questsDone));
    }
}

void AtRivalLoad(FILE *file) {
    AtRivalReset();
    int version = 0;
    if (fscanf(file, " RIVAL %d", &version) != 1 || version != 1) {
        return; // saved by an older version
    }
    for (auto &r : gRival) {
        int v[12] = {};
        if (fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11]) != 12) {
            AtRivalReset();
            return;
        }
        for (SLONG c = 0; c < 4; c++) {
            r.pactUntil[c] = v[c];
            r.cartelUntil[c] = v[4 + c];
        }
        r.lastSmear = v[8];
        r.lastFuel = v[9];
        r.lastOrderDeal = v[10];
        r.questsDone = v[11];
    }
}

bool AtRivalPact(SLONG a, SLONG b) {
    if (!AtExtrasActive() || !ValidPlayer(a) || !ValidPlayer(b) || a == b) {
        return false;
    }
    return gRival[a].pactUntil[b] >= Sim.Date || gRival[b].pactUntil[a] >= Sim.Date;
}

bool AtRivalCartel(SLONG player) {
    if (!AtExtrasActive() || !ValidPlayer(player)) {
        return false;
    }
    for (SLONG c = 0; c < 4; c++) {
        if (c != player && (gRival[player].cartelUntil[c] >= Sim.Date || gRival[c].cartelUntil[player] >= Sim.Date)) {
            return true;
        }
    }
    return false;
}

// Price agreements can be found out
void AtRivalNewDay() {
    if (!AtExtrasActive()) {
        return;
    }
    for (SLONG p = 0; p < 4; p++) {
        for (SLONG r = 0; r < 4; r++) {
            if (!ValidPlayer(p) || !ValidPlayer(r) || gRival[p].cartelUntil[r] < Sim.Date) {
                continue;
            }
            TEAKRAND rnd(static_cast<ULONG>(Sim.StartTime) + static_cast<ULONG>(Sim.Date) * 613U + static_cast<ULONG>(p * 4 + r));
            rnd.Rand(100);
            if (rnd.Rand(100) >= kAtCartelRiskPercent) {
                continue;
            }
            gRival[p].cartelUntil[r] = 0;
            PLAYER &qPlayer = Sim.Players.Players[p];
            PLAYER &qOther = Sim.Players.Players[r];
            qPlayer.ChangeMoney(-kAtCartelFine, 3608, "");
            qPlayer.Image = std::max<SLONG>(-1000, qPlayer.Image - 40);
            qOther.Image = std::max<SLONG>(-1000, qOther.Image - 40);
            Tell(p, bprintf(AtText("Die Preisabsprache mit %s ist aufgeflogen! Das Kartellamt verh\344ngt %s Strafe, und das Image leidet.", "The price agreement with %s has been found out! The cartel office fines us %s, and the image suffers."),
                            (LPCTSTR)qOther.AirlineX, (LPCTSTR)Insert1000erDots64(kAtCartelFine)));
        }
    }
}

//--------------------------------------------------------------------------------------------
// The deals. Each returns the answer of the rival; ok tells whether the deal was made.
//--------------------------------------------------------------------------------------------
CString AtRivalDealPact(PLAYER &qPlayer, SLONG rival, bool &ok) {
    ok = false;
    PLAYER &qOther = Sim.Players.Players[rival];
    if (AtRivalPact(qPlayer.PlayerNum, rival)) {
        return AtText("Wir haben doch schon eine Abmachung. Ich halte mich daran.", "We already have an agreement. I will stick to it.");
    }
    if (qOther.Sympathie[qPlayer.PlayerNum] < -80) {
        return AtText("Nach allem, was war? Vergessen Sie es.", "After everything that happened? Forget it.");
    }
    if (qPlayer.Money < kAtPactPrice) {
        return AtText("Kommen Sie wieder, wenn Sie das Geld haben.", "Come back when you have the money.");
    }
    qPlayer.ChangeMoney(-kAtPactPrice, 3609, "");
    qOther.ChangeMoney(kAtPactPrice, 3609, "");
    gRival[qPlayer.PlayerNum].pactUntil[rival] = Sim.Date + kAtPactDays;
    qOther.NetAddSympathie(qPlayer.PlayerNum, 10);
    ok = true;
    return bprintf(AtText("Abgemacht. %li Tage lang lassen wir uns gegenseitig in Ruhe.", "Agreed. For %li days we leave each other alone."),
                   static_cast<long>(kAtPactDays));
}

CString AtRivalDealCartel(PLAYER &qPlayer, SLONG rival, bool &ok) {
    ok = false;
    PLAYER &qOther = Sim.Players.Players[rival];
    if (gRival[qPlayer.PlayerNum].cartelUntil[rival] >= Sim.Date) {
        return AtText("Unsere Absprache l\344uft noch. Nicht so laut!", "Our agreement is still running. Not so loud!");
    }
    if (CountRoutes(qPlayer) == 0 || CountRoutes(qOther) == 0) {
        return AtText("Preise absprechen? Daf\374r m\374ssten wir beide erst einmal Routen fliegen.", "Agree on prices? We would both have to fly routes first.");
    }
    if (qOther.Sympathie[qPlayer.PlayerNum] < 0) {
        return AtText("Mit Ihnen mache ich keine krummen Sachen. Ich traue Ihnen nicht.", "I do no shady business with you. I do not trust you.");
    }
    gRival[qPlayer.PlayerNum].cartelUntil[rival] = Sim.Date + kAtCartelDays;
    ok = true;
    return bprintf(AtText("Einverstanden. %li Tage lang %li Prozent mehr auf alle Tickets - aber wenn das auffliegt, wird es teuer.", "Agreed. %li percent more on all tickets for %li days - but if this is found out, it will be expensive."),
                   static_cast<long>((gLanguage == LANGUAGE_D) ? kAtCartelDays : kAtCartelPercent),
                   static_cast<long>((gLanguage == LANGUAGE_D) ? kAtCartelPercent : kAtCartelDays));
}

CString AtRivalDealFuel(PLAYER &qPlayer, SLONG rival, bool &ok) {
    ok = false;
    PLAYER &qOther = Sim.Players.Players[rival];
    if (Sim.Date - gRival[qPlayer.PlayerNum].lastFuel < 3) {
        return AtText("Wir haben doch gerade erst zusammen eingekauft.", "We have only just bought together.");
    }
    if (qOther.Sympathie[qPlayer.PlayerNum] < -20) {
        return AtText("Mit Ihnen kaufe ich nicht ein.", "I am not buying together with you.");
    }
    const SLONG amount = qPlayer.Tank - qPlayer.TankInhalt;
    if (qPlayer.Tank <= 0 || amount <= 0) {
        return AtText("Sie haben keinen Platz in Ihren Tanks. Dann lohnt sich das nicht.", "You have no room in your tanks. Then it is not worth it.");
    }
    const auto transaction = GameMechanic::calcKerosinPrice(qPlayer, 1, amount);
    const __int64 cost = transaction.Kosten - transaction.Rabatt;
    if (cost <= 0 || qPlayer.Money < cost) {
        return AtText("Daf\374r reicht Ihr Geld im Moment nicht.", "You do not have the money for that at the moment.");
    }
    if (!GameMechanic::buyKerosin(qPlayer, 1, amount)) {
        return AtText("Daraus wird heute nichts.", "Nothing will come of that today.");
    }
    const __int64 rebate = cost * kAtFuelRebatePercent / 100;
    qPlayer.ChangeMoney(rebate, 3610, "");
    gRival[qPlayer.PlayerNum].lastFuel = Sim.Date;
    qOther.NetAddSympathie(qPlayer.PlayerNum, 3);
    ok = true;
    return bprintf(AtText("Gute Idee. Ihre Tanks sind voll, und der Mengenrabatt bringt Ihnen %s zur\374ck.", "Good idea. Your tanks are full, and the bulk discount gets you %s back."),
                   (LPCTSTR)Insert1000erDots64(rebate));
}

__int64 AtRivalOrderCommission(const CAuftrag &order) { return static_cast<__int64>(order.Praemie) * kAtOrderCommissionPercent / 100; }

// index: position of the order in the album of the player
CString AtRivalDealGiveOrder(PLAYER &qPlayer, SLONG rival, SLONG index, bool &ok) {
    ok = false;
    PLAYER &qOther = Sim.Players.Players[rival];
    if (Sim.Date == gRival[qPlayer.PlayerNum].lastOrderDeal) {
        return AtText("Ein Auftragsgesch\344ft am Tag reicht mir.", "One deal about orders a day is enough for me.");
    }
    if (qOther.Sympathie[qPlayer.PlayerNum] < -50) {
        return AtText("Ihre Auftr\344ge k\366nnen Sie behalten.", "You can keep your orders.");
    }
    if (index < 0 || index >= qPlayer.Auftraege.AnzEntries() || qPlayer.Auftraege.IsInAlbum(index) == 0 || qPlayer.Auftraege[index].InPlan != 0) {
        return AtText("Diesen Auftrag gibt es nicht mehr.", "This order no longer exists.");
    }
    const __int64 commission = AtRivalOrderCommission(qPlayer.Auftraege[index]);
    qPlayer.Auftraege -= qPlayer.Auftraege.GetIdFromIndex(index);
    qPlayer.Blocks.RepaintAll = TRUE;
    qPlayer.ChangeMoney(commission, 3611, "");
    gRival[qPlayer.PlayerNum].lastOrderDeal = Sim.Date;
    qOther.NetAddSympathie(qPlayer.PlayerNum, 2);
    ok = true;
    return bprintf(AtText("Den fliege ich f\374r Sie. Hier sind %s Provision.", "I will fly that one for you. Here is a commission of %s."),
                   (LPCTSTR)Insert1000erDots64(commission));
}

// The order a rival would pass on today
CAuftrag AtRivalOfferedOrder(SLONG rival) {
    CAuftrag order;
    TEAKRAND rnd(static_cast<ULONG>(Sim.StartTime) + static_cast<ULONG>(Sim.Date) * 977U + static_cast<ULONG>(rival) * 31U + 5U);
    rnd.Rand(100);
    order.RefillForReisebuero(0, &rnd);
    return order;
}

CString AtRivalDealTakeOrder(PLAYER &qPlayer, SLONG rival, bool &ok) {
    ok = false;
    PLAYER &qOther = Sim.Players.Players[rival];
    if (Sim.Date == gRival[qPlayer.PlayerNum].lastOrderDeal) {
        return AtText("Ein Auftragsgesch\344ft am Tag reicht mir.", "One deal about orders a day is enough for me.");
    }
    if (qOther.Sympathie[qPlayer.PlayerNum] < -50) {
        return AtText("Ihnen gebe ich nichts ab.", "I am not passing anything on to you.");
    }
    CAuftrag order = AtRivalOfferedOrder(rival);
    const __int64 fee = static_cast<__int64>(order.Praemie) * kAtOrderFeePercent / 100;
    if (order.Praemie <= 0 || qPlayer.Money < fee) {
        return AtText("Daf\374r reicht Ihr Geld im Moment nicht.", "You do not have the money for that at the moment.");
    }
    if (qPlayer.Auftraege.GetNumFree() < 3) {
        qPlayer.Auftraege.ReSize(qPlayer.Auftraege.AnzEntries() + 10);
    }
    qPlayer.Auftraege += order;
    qPlayer.Statistiken[STAT_AUFTRAEGE].AddAtPastDay(1);
    qPlayer.Blocks.RepaintAll = TRUE;
    qPlayer.ChangeMoney(-fee, 3611, "");
    qOther.ChangeMoney(fee, 3611, "");
    gRival[qPlayer.PlayerNum].lastOrderDeal = Sim.Date;
    ok = true;
    return AtText("Er geh\366rt Ihnen. Sie finden ihn bei Ihren Auftr\344gen.", "It is yours. You will find it among your orders.");
}

CString AtRivalDealSmear(PLAYER &qPlayer, SLONG rival, SLONG target, bool &ok) {
    ok = false;
    if (!ValidPlayer(target) || target == rival || target == qPlayer.PlayerNum || Sim.Players.Players[target].IsOut != 0) {
        return AtText("\334ber wen wollen Sie reden?", "Who do you want to talk about?");
    }
    PLAYER &qOther = Sim.Players.Players[rival];
    PLAYER &qTarget = Sim.Players.Players[target];
    if (Sim.Date == gRival[qPlayer.PlayerNum].lastSmear) {
        return AtText("Heute haben Sie schon genug getratscht.", "You have gossiped enough for today.");
    }
    if (qPlayer.Money < kAtSmearPrice) {
        return AtText("Ger\374chte kosten Geld. Das haben Sie gerade nicht.", "Rumours cost money. You do not have it right now.");
    }
    qPlayer.ChangeMoney(-kAtSmearPrice, 3612, "");
    gRival[qPlayer.PlayerNum].lastSmear = Sim.Date;
    qTarget.Image = std::max<SLONG>(-1000, qTarget.Image - 30);
    qOther.NetAddSympathie(target, -15);
    ok = true;

    TEAKRAND rnd(static_cast<ULONG>(Sim.StartTime) + static_cast<ULONG>(Sim.Date) * 331U + static_cast<ULONG>(target));
    rnd.Rand(100);
    if (rnd.Rand(100) < 30) {
        qTarget.NetAddSympathie(qPlayer.PlayerNum, -25);
        Tell(qPlayer.PlayerNum, bprintf(AtText("%s hat erfahren, wer die Ger\374chte gestreut hat, und ist nicht erfreut.", "%s has learned who spread the rumours and is not amused."),
                                        (LPCTSTR)qTarget.AirlineX));
    }
    return bprintf(AtText("Das ist ja interessant! Das \374ber %s spricht sich bestimmt schnell herum.", "Now that is interesting! The story about %s will get around quickly."),
                   (LPCTSTR)qTarget.AirlineX);
}

//--------------------------------------------------------------------------------------------
// Extra figures on the statistics screen
//--------------------------------------------------------------------------------------------
CString AtStatCalcLabel(SLONG code) {
    switch (code) {
    case 0:
        return AtText("Einnahmen/Flug", "Income per flight");
    case 1:
        return AtText("Einnahmen/Flugzeug", "Income per plane");
    case 2:
        return AtText("Passagiere/Flug", "Passengers/flight");
    case 3:
        return AtText("Zufriedene (%)", "Satisfied (%)");
    case 4:
        return AtText("Marktanteil Passag.", "Market share pass.");
    case 5:
        return AtText("Nebenauftr\344ge", "Side jobs done");
    default:
        return AtText("Festgeld - Kredit", "Deposit - term loan");
    }
}

CString AtStatCalc(SLONG player, SLONG viewer, SLONG code) {
    if (!ValidPlayer(player) || !ValidPlayer(viewer)) {
        return "";
    }
    PLAYER &qPlayer = Sim.Players.Players[player];
    if (player != viewer && (code >= 5 || Sim.Players.Players[viewer].HasBerater(BERATERTYP_INFO) < 1)) {
        return "???";
    }
    auto stat = [&qPlayer](SLONG id) { return qPlayer.Statistiken[id].GetAtPastDay(0); };
    const __int64 income = stat(STAT_E_ROUTEN) + stat(STAT_E_AUFTRAEGE) + stat(STAT_E_FRACHT);

    switch (code) {
    case 0:
        return Einheiten[EINH_DM].bString64(income / std::max<__int64>(1, stat(STAT_FLUEGE)));
    case 1:
        return Einheiten[EINH_DM].bString64(income / std::max<__int64>(1, stat(STAT_FLUGZEUGE)));
    case 2:
        return bprintf("%lli", static_cast<long long>(stat(STAT_PASSAGIERE) / std::max<__int64>(1, stat(STAT_FLUEGE))));
    case 3: {
        const __int64 all = stat(STAT_ZUFR_PASSAGIERE) + stat(STAT_UNZUFR_PASSAGIERE);
        return (all > 0) ? CString(bprintf("%lli%%", static_cast<long long>(stat(STAT_ZUFR_PASSAGIERE) * 100 / all))) : CString("-");
    }
    case 4: {
        __int64 all = 0;
        for (SLONG c = 0; c < Sim.Players.Players.AnzEntries() && c < 4; c++) {
            if (Sim.Players.Players[c].IsOut == 0) {
                all += Sim.Players.Players[c].Statistiken[STAT_PASSAGIERE].GetAtPastDay(0);
            }
        }
        return (all > 0) ? CString(bprintf("%lli%%", static_cast<long long>(stat(STAT_PASSAGIERE) * 100 / all))) : CString("-");
    }
    case 5:
        return bprintf("%li", static_cast<long>(gRival[player].questsDone));
    default:
        return Einheiten[EINH_DM].bString64(gFinance[player].deposit - gFinance[player].loan);
    }
}
