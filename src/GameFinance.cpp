//============================================================================================
// GameFinance.cpp : Optional extensions of the bank (see gGameplayExtras):
//  - fixed deposit: money is locked for a week and comes back with interest
//  - term loan: cheaper than the regular credit, but repaid in one sum on a fixed day
//  - short sales: profit if the share price of a competitor falls
//  - newspaper lines about share prices and about airlines that are being bought up
//
// The contracts are kept in a small file next to the savegame ("game<N>.fin"), so the format
// of the savegame itself does not change.
//============================================================================================
#include "global.h"
#include "Proto.h"

#include <cmath>
#include <cstdio>

ATFinance gFinance[4];

namespace {
CString SidecarName(SLONG slot) { return FullFilename((LPCTSTR)bprintf("game%li.fin", static_cast<long>(slot)), SavegamePath); }

bool ValidPlayer(SLONG p) { return p >= 0 && p < 4 && p < Sim.Players.Players.AnzEntries(); }

void Tell(SLONG player, const CString &text) {
    if (player == Sim.localPlayer && ValidPlayer(player)) {
        Sim.Players.Players[player].Messages.AddMessage(BERATERTYP_GIRL, text, MESSAGE_URGENT);
    }
}
} // namespace

void AtFinanceReset() {
    for (auto &f : gFinance) {
        f = ATFinance();
    }
}

void AtFinanceSave(SLONG slot) {
    FILE *file = fopen(SidecarName(slot).c_str(), "w");
    if (file == nullptr) {
        return;
    }
    fprintf(file, "ATFIN 1\n");
    for (const auto &f : gFinance) {
        fprintf(file, "%lld %d %lld %d\n", static_cast<long long>(f.deposit), static_cast<int>(f.depositDue), static_cast<long long>(f.loan),
                static_cast<int>(f.loanDue));
        for (SLONG c = 0; c < 4; c++) {
            fprintf(file, "%d %.4f %d\n", static_cast<int>(f.shortShares[c]), f.shortPrice[c], static_cast<int>(f.shortDue[c]));
        }
    }
    fclose(file);
}

void AtFinanceLoad(SLONG slot) {
    AtFinanceReset();

    FILE *file = fopen(SidecarName(slot).c_str(), "r");
    if (file == nullptr) {
        return; // saved without the extensions
    }
    int version = 0;
    bool ok = (fscanf(file, "ATFIN %d", &version) == 1 && version == 1);
    for (SLONG p = 0; ok && p < 4; p++) {
        long long deposit = 0;
        long long loan = 0;
        int depositDue = 0;
        int loanDue = 0;
        ok = (fscanf(file, "%lld %d %lld %d", &deposit, &depositDue, &loan, &loanDue) == 4);
        if (ok) {
            gFinance[p].deposit = deposit;
            gFinance[p].depositDue = depositDue;
            gFinance[p].loan = loan;
            gFinance[p].loanDue = loanDue;
        }
        for (SLONG c = 0; ok && c < 4; c++) {
            int shares = 0;
            int due = 0;
            double price = 0;
            ok = (fscanf(file, "%d %lf %d", &shares, &price, &due) == 3);
            if (ok) {
                gFinance[p].shortShares[c] = shares;
                gFinance[p].shortPrice[c] = price;
                gFinance[p].shortDue[c] = due;
            }
        }
    }
    fclose(file);
    if (!ok) {
        AtFinanceReset();
    }
}

//--------------------------------------------------------------------------------------------
// Contracts
//--------------------------------------------------------------------------------------------
bool AtFinanceOpenDeposit(PLAYER &qPlayer, __int64 amount) {
    if (!ValidPlayer(qPlayer.PlayerNum) || amount <= 0 || amount > qPlayer.Money) {
        return false;
    }
    ATFinance &f = gFinance[qPlayer.PlayerNum];
    if (f.deposit != 0) {
        return false;
    }
    f.deposit = amount;
    f.depositDue = Sim.Date + kAtDepositDays;
    qPlayer.ChangeMoney(-amount, 3601, "");
    return true;
}

bool AtFinanceOpenLoan(PLAYER &qPlayer, __int64 amount) {
    if (!ValidPlayer(qPlayer.PlayerNum) || amount <= 0 || amount > qPlayer.CalcCreditLimit()) {
        return false;
    }
    ATFinance &f = gFinance[qPlayer.PlayerNum];
    if (f.loan != 0) {
        return false;
    }
    f.loan = amount;
    f.loanDue = Sim.Date + kAtLoanDays;
    qPlayer.ChangeMoney(amount, 3603, "");
    return true;
}

bool AtFinanceOpenShort(PLAYER &qPlayer, SLONG target, SLONG shares) {
    if (!ValidPlayer(qPlayer.PlayerNum) || !ValidPlayer(target) || target == qPlayer.PlayerNum || shares <= 0) {
        return false;
    }
    const PLAYER &qTarget = Sim.Players.Players[target];
    ATFinance &f = gFinance[qPlayer.PlayerNum];
    if (qTarget.IsOut != 0 || qTarget.Kurse[0] < 1.0 || f.shortShares[target] != 0) {
        return false;
    }
    // the value of the shares has to be on the account as a security
    if (static_cast<double>(qPlayer.Money) < qTarget.Kurse[0] * shares) {
        return false;
    }
    f.shortShares[target] = shares;
    f.shortPrice[target] = qTarget.Kurse[0];
    f.shortDue[target] = Sim.Date + kAtShortDays;
    return true;
}

__int64 AtFinanceCloseShort(PLAYER &qPlayer, SLONG target) {
    if (!ValidPlayer(qPlayer.PlayerNum) || !ValidPlayer(target)) {
        return 0;
    }
    ATFinance &f = gFinance[qPlayer.PlayerNum];
    if (f.shortShares[target] == 0) {
        return 0;
    }
    const double now = Sim.Players.Players[target].Kurse[0];
    const auto profit = static_cast<__int64>((f.shortPrice[target] - now) * f.shortShares[target]);
    f.shortShares[target] = 0;
    f.shortPrice[target] = 0;
    f.shortDue[target] = 0;
    if (profit > 0) {
        qPlayer.ChangeMoney(profit, 3605, "");
    } else if (profit < 0) {
        qPlayer.ChangeMoney(profit, 3606, "");
    }
    return profit;
}

//--------------------------------------------------------------------------------------------
// Start of a day: pays out and collects what is due. Contracts are honoured even if the
// extensions were switched off in the meantime.
//--------------------------------------------------------------------------------------------
void AtFinanceNewDay() {
    if (Sim.bNetwork != 0) {
        return;
    }

    for (SLONG p = 0; p < 4 && p < Sim.Players.Players.AnzEntries(); p++) {
        PLAYER &qPlayer = Sim.Players.Players[p];
        ATFinance &f = gFinance[p];
        if (qPlayer.IsOut != 0) {
            continue;
        }

        if (f.deposit != 0 && Sim.Date >= f.depositDue) {
            const __int64 back = f.deposit + f.deposit * kAtDepositPercent / 100;
            f.deposit = 0;
            qPlayer.ChangeMoney(back, 3602, "");
            Tell(p, CString(bprintf(AtText("Ihr Festgeld ist f\344llig: %s sind wieder auf dem Konto.", "Your fixed deposit is due: %s are back in your account."),
                                    (LPCTSTR)Insert1000erDots64(back))));
        }

        if (f.loan != 0 && Sim.Date >= f.loanDue) {
            const __int64 back = f.loan + f.loan * kAtLoanPercent / 100;
            f.loan = 0;
            qPlayer.ChangeMoney(-back, 3604, "");
            Tell(p, CString(bprintf(AtText("Der Laufzeitkredit wurde getilgt: %s wurden abgebucht.", "The term loan was repaid: %s were debited."),
                                    (LPCTSTR)Insert1000erDots64(back))));
        }

        for (SLONG c = 0; c < 4; c++) {
            if (f.shortShares[c] != 0 && (Sim.Date >= f.shortDue[c] || Sim.Players.Players[c].IsOut != 0)) {
                const __int64 profit = AtFinanceCloseShort(qPlayer, c);
                Tell(p, CString(bprintf(profit >= 0 ? AtText("Leerverkauf von %s abgerechnet: %s Gewinn.", "Short sale of %s settled: %s profit.")
                                                    : AtText("Leerverkauf von %s abgerechnet: %s Verlust.", "Short sale of %s settled: %s loss."),
                                        (LPCTSTR)Sim.Players.Players[c].AirlineX, (LPCTSTR)Insert1000erDots64(profit >= 0 ? profit : -profit))));
            }
        }
    }

    // Warning: somebody is buying up the airline of the player
    if (AtExtrasActive() && ValidPlayer(Sim.localPlayer)) {
        const PLAYER &qLocal = Sim.Players.Players[Sim.localPlayer];
        for (SLONG d = 0; d < 4 && d < Sim.Players.Players.AnzEntries(); d++) {
            if (d == Sim.localPlayer || Sim.Players.Players[d].IsOut != 0 || qLocal.AnzAktien <= 0) {
                continue;
            }
            const __int64 percent = static_cast<__int64>(Sim.Players.Players[d].OwnsAktien[Sim.localPlayer]) * 100 / qLocal.AnzAktien;
            if (percent >= 30) {
                Tell(Sim.localPlayer, CString(bprintf(AtText("Achtung: %s h\344lt schon %li Prozent Ihrer Aktien. Ab der H\344lfte droht die \334bernahme - kaufen Sie in der Bank zur\374ck!", "Warning: %s already holds %li percent of your shares. At one half a takeover looms - buy back at the bank!"),
                                                      (LPCTSTR)Sim.Players.Players[d].AirlineX, static_cast<long>(percent))));
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
// Texts
//--------------------------------------------------------------------------------------------
CString AtFinanceSummary(SLONG player) {
    if (!ValidPlayer(player)) {
        return CString();
    }
    const ATFinance &f = gFinance[player];
    CString text;

    if (f.deposit != 0) {
        text += bprintf(AtText("Festgeld: %s, f\344llig in %li Tagen. ", "Fixed deposit: %s, due in %li days. "), (LPCTSTR)Insert1000erDots64(f.deposit),
                        static_cast<long>(max(0, f.depositDue - Sim.Date)));
    }
    if (f.loan != 0) {
        text += bprintf(AtText("Laufzeitkredit: %s, f\344llig in %li Tagen. ", "Term loan: %s, due in %li days. "), (LPCTSTR)Insert1000erDots64(f.loan),
                        static_cast<long>(max(0, f.loanDue - Sim.Date)));
    }
    for (SLONG c = 0; c < 4 && c < Sim.Players.Players.AnzEntries(); c++) {
        if (f.shortShares[c] != 0) {
            text += bprintf(AtText("Leerverkauf %s: %li Aktien zu %li, heute %li. ", "Short sale %s: %li shares at %li, today %li. "),
                            (LPCTSTR)Sim.Players.Players[c].AirlineX, static_cast<long>(f.shortShares[c]), static_cast<long>(f.shortPrice[c]),
                            static_cast<long>(Sim.Players.Players[c].Kurse[0]));
        }
    }
    if (text.IsEmpty()) {
        text = AtText("Sie haben im Moment weder Festgeld noch einen Laufzeitkredit noch Leerverk\344ufe.", "At the moment you have no fixed deposit, no term loan and no short sales.");
    }
    return text;
}

// Newspaper: the share that moved most since yesterday
CString AtStockHeadline() {
    SLONG best = -1;
    double bestMove = 0.05; // at least 5 percent
    for (SLONG c = 0; c < 4 && c < Sim.Players.Players.AnzEntries(); c++) {
        const PLAYER &qPlayer = Sim.Players.Players[c];
        if (qPlayer.IsOut != 0 || qPlayer.Kurse[1] < 1.0) {
            continue;
        }
        const double move = qPlayer.Kurse[0] / qPlayer.Kurse[1] - 1.0;
        if (fabs(move) > fabs(bestMove)) {
            bestMove = move;
            best = c;
        }
    }
    if (best == -1) {
        return CString();
    }
    const long percent = static_cast<long>(fabs(bestMove) * 100.0);
    return CString(bprintf(bestMove > 0 ? AtText("B\366rse: Aktie von %s steigt um %li Prozent", "Stock market: shares of %s rise by %li percent")
                                        : AtText("B\366rse: Aktie von %s f\344llt um %li Prozent", "Stock market: shares of %s fall by %li percent"),
                           (LPCTSTR)Sim.Players.Players[best].AirlineX, percent));
}

// Newspaper: the largest stake one airline holds in another, from one quarter on
CString AtTakeoverHeadline() {
    SLONG buyer = -1;
    SLONG target = -1;
    __int64 bestPercent = 24;
    for (SLONG t = 0; t < 4 && t < Sim.Players.Players.AnzEntries(); t++) {
        const PLAYER &qTarget = Sim.Players.Players[t];
        if (qTarget.IsOut != 0 || qTarget.AnzAktien <= 0) {
            continue;
        }
        for (SLONG b = 0; b < 4 && b < Sim.Players.Players.AnzEntries(); b++) {
            if (b == t || Sim.Players.Players[b].IsOut != 0) {
                continue;
            }
            const __int64 percent = static_cast<__int64>(Sim.Players.Players[b].OwnsAktien[t]) * 100 / qTarget.AnzAktien;
            if (percent > bestPercent) {
                bestPercent = percent;
                buyer = b;
                target = t;
            }
        }
    }
    if (buyer == -1) {
        return CString();
    }
    return CString(bprintf(AtText("\334bernahme-Ger\374chte: %s h\344lt bereits %li Prozent an %s", "Takeover rumours: %s already holds %li percent of %s"),
                           (LPCTSTR)Sim.Players.Players[buyer].AirlineX, static_cast<long>(bestPercent), (LPCTSTR)Sim.Players.Players[target].AirlineX));
}
