// Deteccao de crimes, decaimento, morte/prisao e calor maximo.
#include "common.h"
#include <unordered_set>

static std::unordered_set<int> s_deadPeds;   // peds mortos ja avaliados
static std::unordered_set<int> s_deadVehs;   // veiculos destruidos ja avaliados
static std::unordered_set<int> s_stolen;     // veiculos ja contados como roubo
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

void AddPoints(int amount, const char *why) {
    State &s = g_st;
    if (amount > 0 && s.now < s.cooldownUntil) {
        Log("%s ignorado (cooldown do calor maximo).", why);
        return;
    }
    int before = s.Pts();
    int lvBefore = LevelOf(before);
    int v = before + amount;
    if (v < 0) v = 0;
    if (v > g_cfg.maxPoints) v = g_cfg.maxPoints;
    s.Pts() = v;
    if (amount > 0) { s.lastCrime = s.now; s.lastDecay = s.now; }
    int lv = LevelOf(v);
    Log("%+d %s -> %d pontos (nivel %d) [%s]", amount, why, v, lv, kCharName[s.ch]);
    if (lv != lvBefore) {
        Log("Nivel %d -> %d", lvBefore, lv);
        if (lv > lvBefore) {
            char msg[96];
            snprintf(msg, sizeof msg, "~r~Retaliação~s~: nível %d", lv);
            Notify(msg);
        }
    }
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
        if (ENTITY::IS_ENTITY_A_MISSION_ENTITY(v)) continue;
        bool byPlayer = ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, player, TRUE) ||
                        (pveh && pveh != v && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, pveh, TRUE)) ||
                        (plast && plast != v && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(v, plast, TRUE));
        if (byPlayer) AddPoints(g_cfg.blowVehicle, "veiculo explodido");
    }
    // roubo: jogador ao volante de um veiculo marcado como roubado
    if (pveh && VEHICLE::GET_PED_IN_VEHICLE_SEAT(pveh, -1) == player && !s_stolen.count(pveh) &&
        VEHICLE::IS_VEHICLE_STOLEN(pveh) && !ENTITY::IS_ENTITY_A_MISSION_ENTITY(pveh)) {
        s_stolen.insert(pveh);
        AddPoints(g_cfg.stealVehicle, "veiculo roubado");
    }
}

void PointsTick(Ped player, Player pid) {
    State &s = g_st;

    // morte ou prisao zera os pontos do personagem atual
    bool dead = PLAYER::IS_PLAYER_DEAD(pid) || PLAYER::IS_PLAYER_BEING_ARRESTED(pid, TRUE);
    if (dead && !s_wasDead) {
        if (s.Pts() > 0) Log("Jogador morreu ou foi preso: pontos zerados [%s].", kCharName[s.ch]);
        s.Pts() = 0;
        s.maxHeatSince = -1;
    }
    s_wasDead = dead;
    if (dead) return;

    Vehicle pveh = PED::IS_PED_IN_ANY_VEHICLE(player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(player, FALSE) : 0;
    Vehicle plast = PED::GET_VEHICLE_PED_IS_IN(player, TRUE);
    ScanKills(player, pveh, plast);
    ScanVehicles(player, pveh, plast);

    if (s.now - s_lastPrune > 5000) {
        s_lastPrune = s.now;
        Prune(s_deadPeds, true);
        Prune(s_deadVehs, true);
        Prune(s_stolen, false);
    }

    // decaimento
    if (s.Pts() > 0 && s.now - s.lastCrime >= g_cfg.decayInterval * 1000 &&
        s.now - s.lastDecay >= g_cfg.decayInterval * 1000) {
        s.lastDecay = s.now;
        AddPoints(-g_cfg.decayAmount, "tempo sem crimes");
    }

    // calor maximo: tempo seguido acima do limite vira estrelas da policia
    if (g_cfg.maxHeatEnabled) {
        if (s.Pts() >= g_cfg.maxHeatPoints) {
            if (s.maxHeatSince < 0) {
                s.maxHeatSince = s.now;
                Log("Calor maximo carregando (%ds).", g_cfg.maxHeatSeconds);
            } else if (s.now - s.maxHeatSince >= g_cfg.maxHeatSeconds * 1000) {
                PLAYER::SET_PLAYER_WANTED_LEVEL(pid, g_cfg.maxHeatWanted, FALSE);
                PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(pid, FALSE);
                s.Pts() = 0;
                s.maxHeatSince = -1;
                s.cooldownUntil = s.now + g_cfg.maxHeatCooldown * 1000;
                Log("CALOR MAXIMO: %d estrelas, retaliacao zerada, cooldown %ds.", g_cfg.maxHeatWanted,
                    g_cfg.maxHeatCooldown);
                Notify("~r~Calor máximo!~s~ A polícia assumiu a caçada.");
            }
        } else if (s.maxHeatSince >= 0) {
            s.maxHeatSince = -1;
            Log("Calor maximo interrompido.");
        }
    }
}
