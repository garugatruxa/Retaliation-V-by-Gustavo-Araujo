// Deteccao de crimes, decaimento, morte/prisao e calor maximo.
#include "spawn.h"
#include <unordered_set>

static std::unordered_set<int> s_deadPeds;   // peds mortos ja avaliados
static std::unordered_set<int> s_deadVehs;   // veiculos destruidos ja avaliados
static std::unordered_set<int> s_stolen;     // veiculos ja contados como roubo
static std::unordered_set<int> s_ignoreVeh;  // veiculos dos retaliadores (nao dao pontos)

void PointsIgnorePed(int p) { s_deadPeds.insert(p); }
void PointsIgnoreVeh(int v) { s_ignoreVeh.insert(v); }
static int s_lastPrune = 0;
static bool s_wasDead = false;

static const int kMaxEnt = 1024;
static int s_buf[kMaxEnt];

static Hash H(const char *s) { return GAMEPLAY::GET_HASH_KEY((char *)s); }

static bool IsGangGroup(Hash g) {
    static const Hash gangs[] = {
        H("AMBIENT_GANG_LOST"),     H("AMBIENT_GANG_MEXICAN"), H("AMBIENT_GANG_FAMILY"),
        H("AMBIENT_GANG_BALLAS"),   H("AMBIENT_GANG_MARABUNTE"), H("AMBIENT_GANG_CULT"),
        H("AMBIENT_GANG_SALVA"),    H("AMBIENT_GANG_WEICHENG"), H("AMBIENT_GANG_HILLBILLY"),
    };
    for (Hash x : gangs)
        if (g == x) return true;
    return false;
}

enum Victim { V_NONE, V_CIV, V_COP, V_GANG };

static Victim Classify(Ped p) {
    int t = PED::GET_PED_TYPE(p);
    if (t == 28) return V_NONE;                       // animal
    if (t == 6 || t == 27 || t == 29) return V_COP;   // policia, SWAT, exercito
    Hash g = PED::GET_PED_RELATIONSHIP_GROUP_HASH(p);
    static const Hash cop = H("COP"), army = H("ARMY");
    if (g == cop || g == army) return V_COP;
    if ((t >= 7 && t <= 18) || IsGangGroup(g)) return V_GANG;
    return V_CIV;
}

// v7: fora do ciclo (sem Retaliation ativo) os crimes nao dao pontos; so a policia age.
static bool s_force = false;
void AddPointsForce(int amount, const char *why) { s_force = true; AddPoints(amount, why); s_force = false; }

void AddPoints(int amount, const char *why) {
    State &s = g_st;
    // so no modo ciclo (3) os crimes nao dao pontos fora do Retaliation
    if (amount > 0 && s.Pts() <= 0 && !s_force && !s.director && g_cfg.mode == 3) return;
    if (amount > 0 && s.now < s.cooldownUntil) {
        Log("%s ignorado (cooldown do calor maximo).", why);
        return;
    }
    int before = s.Pts();
    int lvBefore = LevelOf(before);

    // v9: escala decrescente - a partir de 50 pontos, cada 5 pontos acima tiram 0,2 de cada ganho (minimo 0,2).
    // As fracoes se acumulam por personagem ate completar um ponto. Nao vale para teclas de teste nem o ciclo.
    static float frac[CH_COUNT] = {};
    if (amount > 0 && !s_force && g_cfg.scaleStep > 0 && before >= g_cfg.scaleStart) {
        int steps = (before - g_cfg.scaleStart) / g_cfg.scaleStep;
        float gain = amount - steps * g_cfg.scalePerStep;
        if (gain < g_cfg.scaleMinGain) gain = g_cfg.scaleMinGain;
        frac[s.ch] += gain;
        int whole = (int)frac[s.ch];
        frac[s.ch] -= whole;
        if (whole <= 0) {
            s.lastCrime = s.now; s.lastDecay = s.now;
            Log("+%.1f %s (acumulando %.1f) -> %d pontos [%s]", gain, why, frac[s.ch], before, kCharName[s.ch]);
            return;
        }
        if (gain != (float)amount) Log("  ganho reduzido pela escala: +%d vira +%.1f", amount, gain);
        amount = whole;
    }
    if (before <= 0) frac[s.ch] = 0.f;

    int v = before + amount;
    if (v < 0) v = 0;
    if (v > g_cfg.maxPoints) v = g_cfg.maxPoints;
    s.Pts() = v;
    if (amount > 0) { s.lastCrime = s.now; s.lastDecay = s.now; }
    int lv = LevelOf(v);
    Log("%+d %s -> %d pontos (nivel %d) [%s]", amount, why, v, lv, kCharName[s.ch]);
    if (lv != lvBefore) Log("Nivel %d -> %d", lvBefore, lv); // v4: sem aviso na tela, so a barra
}

// Marca tudo que ja esta morto como avaliado, sem dar pontos (ao iniciar ou carregar).
void PointsSeed() {
    s_deadPeds.clear(); s_deadVehs.clear(); s_stolen.clear();
    int n = worldGetAllPeds(s_buf, kMaxEnt);
    for (int i = 0; i < n; ++i)
        if (ENTITY::IS_ENTITY_DEAD(s_buf[i])) s_deadPeds.insert(s_buf[i]);
    n = worldGetAllVehicles(s_buf, kMaxEnt);
    for (int i = 0; i < n; ++i)
        if (ENTITY::IS_ENTITY_DEAD(s_buf[i])) s_deadVehs.insert(s_buf[i]);
}

void PointsReset() {
    for (int &p : g_st.points) p = 0;
    g_st.maxHeatSince = -1;
    g_st.cooldownUntil = 0;
    PointsSeed();
}

// Remove handles que nao existem mais ou foram reaproveitados por entidades vivas.
static void Prune(std::unordered_set<int> &set, bool keepOnlyDead) {
    for (auto it = set.begin(); it != set.end();) {
        bool gone = !ENTITY::DOES_ENTITY_EXIST(*it) || (keepOnlyDead && !ENTITY::IS_ENTITY_DEAD(*it));
        it = gone ? set.erase(it) : std::next(it);
    }
}

static void ScanKills(Ped player, Vehicle pveh, Vehicle plast) {
    int n = worldGetAllPeds(s_buf, kMaxEnt);
    for (int i = 0; i < n; ++i) {
        Ped p = s_buf[i];
        if (p == player || !ENTITY::IS_ENTITY_DEAD(p)) continue;
        if (!s_deadPeds.insert(p).second) continue;   // ja avaliado
        if (PED::IS_PED_A_PLAYER(p) || ENTITY::IS_ENTITY_A_MISSION_ENTITY(p)) continue; // missao / retaliadores
        Entity k = PED::_GET_PED_KILLER(p);
        if (!k || !(k == player || (pveh && k == pveh) || (plast && k == plast))) continue;
        switch (Classify(p)) {
        case V_CIV: AddPoints(g_cfg.killCivilian, "civil morto"); break;
        case V_COP: AddPoints(g_cfg.killCop, "policial morto"); break;
        case V_GANG: AddPoints(g_cfg.killGang, "membro de gangue morto"); break;
        default: break;
        }
    }
}

static void ScanVehicles(Ped player, Vehicle pveh, Vehicle plast) {
    int n = worldGetAllVehicles(s_buf, kMaxEnt);
    for (int i = 0; i < n; ++i) {
        Vehicle v = s_buf[i];
        if (!ENTITY::IS_ENTITY_DEAD(v)) continue;
        if (!s_deadVehs.insert(v).second) continue;
        if (ENTITY::IS_ENTITY_A_MISSION_ENTITY(v) || s_ignoreVeh.count(v)) continue;
        bool byPlayer = ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, player, TRUE) ||
                        (pveh && pveh != v && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, pveh, TRUE)) ||
                        (plast && plast != v && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, plast, TRUE));
        if (byPlayer) {
            Hash m = ENTITY::GET_ENTITY_MODEL(v);
            if (VEHICLE::IS_THIS_MODEL_A_HELI(m) || VEHICLE::IS_THIS_MODEL_A_PLANE(m))
                AddPoints(g_cfg.blowAircraft, "aeronave destruida");
            else
                AddPoints(g_cfg.blowVehicle, "veiculo explodido");
        }
    }
    // roubo: jogador ao volante de um veiculo marcado como roubado
    if (pveh && VEHICLE::GET_PED_IN_VEHICLE_SEAT(pveh, -1) == player && !s_stolen.count(pveh) &&
        VEHICLE::IS_VEHICLE_STOLEN(pveh) && !ENTITY::IS_ENTITY_A_MISSION_ENTITY(pveh)) {
        s_stolen.insert(pveh);
        AddPoints(g_cfg.stealVehicle, "veiculo roubado");
    }
}

// ---------- ciclo policia -> Retaliation -> livre (v7) ----------
// 5 estrelas por 3 minutos sem morrer nem ser preso: o Retaliation comeca no nivel 2. Durante a "fase conjunta"
// (5 minutos) as estrelas continuam e a policia segue vindo, mas IGNORA o jogador e so enfrenta os retaliadores.
// Depois disso as estrelas somem e a policia fica bloqueada ate o jogador ficar livre (pontos zerados).
static int s_maxWantedApplied = -1;
static bool s_wasActive = false;
static bool s_ignoring = true; // comeca "ligado" para o 1o frame sempre desligar (ex.: mod recarregado no meio da fase)
static int s_jointUntil = 0, s_nextCentre = 0;

static void ApplyMaxWanted(int v) {
    if (v == s_maxWantedApplied) return;
    PLAYER::SET_MAX_WANTED_LEVEL(v);
    s_maxWantedApplied = v;
}
static void PoliceIgnore(Player pid, bool on) {
    if (on == s_ignoring) return;
    PLAYER::SET_POLICE_IGNORE_PLAYER(pid, on);
    s_ignoring = on;
}
static void ClearWanted(Player pid) {
    PLAYER::SET_PLAYER_WANTED_LEVEL(pid, 0, FALSE);
    PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(pid, FALSE);
}

void PoliceRestore(Player pid) {
    PoliceIgnore(pid, false);
    ApplyMaxWanted(5);
    s_jointUntil = 0;
    s_wasActive = false;
    g_st.wantedMaxSince = -1;
}

void CycleTick(Player pid) {
    State &s = g_st;
    bool missionPause0 = s.paused && strcmp(s.pauseReason, "interior") != 0;
    // v8, modo 1: junto com a policia (original) - o mod nao mexe na policia
    // v8, modo 2: substitui a policia - o jogador nunca ganha estrelas; os retaliadores vem no lugar
    if (!s.director && g_cfg.mode != 3) {
        PoliceIgnore(pid, false);
        s_jointUntil = 0;
        s.wantedMaxSince = -1;
        bool block = g_cfg.mode == 2 && !missionPause0;
        ApplyMaxWanted(block ? 0 : 5);
        if (block && PLAYER::GET_PLAYER_WANTED_LEVEL(pid) > 0) ClearWanted(pid);
        bool active = s.Pts() > 0;
        if (s_wasActive && !active) DespawnAll(false, "jogador livre");
        s_wasActive = active;
        return;
    }
    if (s.director) { // Modo Diretor: sem ciclo e sem mexer na policia (ele tem a propria configuracao)
        PoliceIgnore(pid, false);
        ApplyMaxWanted(5);
        s_jointUntil = 0;
        s_wasActive = s.Pts() > 0;
        s.wantedMaxSince = -1;
        return;
    }
    bool active = s.Pts() > 0;
    bool missionPause = s.paused && strcmp(s.pauseReason, "interior") != 0;
    bool joint = active && s_jointUntil && s.now < s_jointUntil;

    if (missionPause) { // missoes da historia: policia normal
        PoliceIgnore(pid, false);
        ApplyMaxWanted(5);
    } else if (joint) {
        // fase conjunta: mantem as estrelas e a policia procurando na area do jogador, mas ignorando ele
        ApplyMaxWanted(5);
        PoliceIgnore(pid, true);
        if (PLAYER::GET_PLAYER_WANTED_LEVEL(pid) < g_cfg.cycleWanted) {
            PLAYER::SET_PLAYER_WANTED_LEVEL(pid, g_cfg.cycleWanted, FALSE);
            PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(pid, FALSE);
        }
        if (s.now >= s_nextCentre) {
            s_nextCentre = s.now + 3000;
            Vector3 p = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
            invoke<Void>(0x520E541A97A13354, pid, &p, FALSE, FALSE); // SET_PLAYER_WANTED_CENTRE_POSITION
        }
    } else if (active) {
        if (s_jointUntil) { // fim da fase conjunta
            s_jointUntil = 0;
            PoliceIgnore(pid, false);
            ClearWanted(pid);
            Log("Fase conjunta encerrada: a policia foi embora, o Retaliation segue sozinho.");
        }
        PoliceIgnore(pid, false);
        ApplyMaxWanted(0); // policia bloqueada ate o jogador ficar livre
        if (PLAYER::GET_PLAYER_WANTED_LEVEL(pid) > 0) ClearWanted(pid);
    } else {
        PoliceIgnore(pid, false);
        ApplyMaxWanted(5);
    }

    if (s_wasActive && !active) {
        s_jointUntil = 0;
        Log("Retaliation zerado: jogador livre, policia de volta ao normal.");
        DespawnAll(false, "jogador livre");
    }
    s_wasActive = active;

    if (active || missionPause) { s.wantedMaxSince = -1; return; }
    int wl = PLAYER::GET_PLAYER_WANTED_LEVEL(pid);
    if (wl >= g_cfg.cycleWanted) {
        if (s.wantedMaxSince < 0) {
            s.wantedMaxSince = s.now;
            Log("%d estrelas: em %ds o Retaliation entra junto com a policia.", wl, g_cfg.cycleSeconds);
        } else if (s.now - s.wantedMaxSince >= g_cfg.cycleSeconds * 1000) {
            s.wantedMaxSince = -1;
            int lv = g_cfg.cycleStartLevel;
            int start = lv >= 1 && lv <= (int)g_cfg.levelStart.size() ? g_cfg.levelStart[lv - 1] : 21;
            AddPointsForce(start, "o Retaliation assume; a policia passa a ignorar o jogador");
            s_jointUntil = s.now + g_cfg.cycleJointSeconds * 1000;
            s_nextCentre = 0;
            PoliceIgnore(pid, true);
            Log("Fase conjunta por %ds: policia contra os retaliadores.", g_cfg.cycleJointSeconds);
        }
    } else if (s.wantedMaxSince >= 0) {
        s.wantedMaxSince = -1;
        Log("Estrelas perdidas antes do tempo: timer interrompido.");
    }
}
void PointsTick(Ped player, Player pid) {    State &s = g_st;

    // morte ou prisao zera os pontos do personagem atual
    bool dead = PLAYER::IS_PLAYER_DEAD(pid) || PLAYER::IS_PLAYER_BEING_ARRESTED(pid, TRUE);
    if (dead && !s_wasDead) {
        if (s.Pts() > 0) Log("Jogador morreu ou foi preso: pontos zerados [%s].", kCharName[s.ch]);
        s.Pts() = 0;
        s.maxHeatSince = -1;
        s.hotSince = -1;
        s.wantedMaxSince = -1;
    }
    s_wasDead = dead;
    if (dead) return;

    // v7: acima de 70 pontos por 5 minutos sem morrer nem ser preso -> cai de volta para o nivel 2
    if (g_cfg.hotEnabled) {
        if (s.Pts() >= g_cfg.hotPoints) {
            if (s.hotSince < 0) {
                s.hotSince = s.now;
                Log("Timer de %ds acima de %d pontos iniciado.", g_cfg.hotSeconds, g_cfg.hotPoints);
            } else if (s.now - s.hotSince >= g_cfg.hotSeconds * 1000) {
                s.hotSince = -1;
                // v7: volta para o nivel 2; o jogador escolhe entre voltar ao caos ou se esconder
                int lv = g_cfg.cycleStartLevel;
                int to = lv >= 1 && lv <= (int)g_cfg.levelStart.size() ? g_cfg.levelStart[lv - 1] : 21;
                if (s.Pts() > to) AddPoints(to - s.Pts(), "sobreviveu 5 minutos acima do limite: volta ao nivel 2");
            }
        } else if (s.hotSince >= 0) {
            s.hotSince = -1;
            Log("Timer acima de %d pontos interrompido.", g_cfg.hotPoints);
        }
    }

    CycleTick(pid);

    Vehicle pveh = PED::IS_PED_IN_ANY_VEHICLE(player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(player, FALSE) : 0;
    Vehicle plast = PED::GET_VEHICLE_PED_IS_IN(player, TRUE);
    ScanKills(player, pveh, plast);
    ScanVehicles(player, pveh, plast);

    if (s.now - s_lastPrune > 5000) {
        s_lastPrune = s.now;
        Prune(s_deadPeds, true);
        Prune(s_deadVehs, true);
        Prune(s_stolen, false);
        Prune(s_ignoreVeh, false);
    }

    // decaimento
    if (s.Pts() > 0 && s.now - s.lastCrime >= g_cfg.decayInterval * 1000 &&
        s.now - s.lastDecay >= g_cfg.decayInterval * 1000) {
        s.lastDecay = s.now;
        AddPoints(-g_cfg.decayAmount, "tempo sem crimes");
    }

    // v4: o calor maximo foi removido; quem manda agora e o timer de 5 minutos (HotTimer) acima.
}
