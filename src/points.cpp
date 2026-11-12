// Deteccao de crimes, decaimento, morte/prisao e calor maximo.
#include "spawn.h"
#include <unordered_set>
#include <unordered_map>

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
static void DisorderReset();
static void DisorderOnDeath();
static bool DisorderHold();

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
    // v10: trava de nivel - so as teclas de teste (AddPointsForce) mudam os pontos
    if (g_cfg.levelLock && !s_force) return;
    // so no modo ciclo (3) os crimes nao dao pontos fora do Retaliation
    if (amount > 0 && s.Pts() <= 0 && !s_force && g_cfg.mode == 3) return; // v10: vale tambem no Modo Diretor
    if (amount > 0 && s.now < s.cooldownUntil) {
        Log("%s ignorado (cooldown do calor maximo).", why);
        return;
    }
    int before = s.Pts();
    int lvBefore = LevelOf(before);

    // v9: escala decrescente. v10: a partir de 250 pontos, cada 25 acima tiram 0,25 de cada ganho (minimo 0,5).
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
    DisorderReset(); // v11
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
        if (!k || !(k == player || (pveh && k == pveh) || (plast && k == plast))) {
            // v11: na Desordem, qualquer morte perto do jogador sobe a pressao, mesmo sem ser ele
            if (DisorderOn() && Classify(p) != V_NONE) {
                Vector3 a = ENTITY::GET_ENTITY_COORDS(p, FALSE), b = ENTITY::GET_ENTITY_COORDS(player, TRUE);
                float dx = a.x - b.x, dy = a.y - b.y;
                if (dx * dx + dy * dy < g_cfg.disDeathRadius * g_cfg.disDeathRadius)
                    AddPoints(g_cfg.disDeathPts, "morte na desordem");
            }
            continue;
        }
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
        } else if (DisorderOn()) { // v11: destruicao perto do jogador, de quem for
            Vector3 a = ENTITY::GET_ENTITY_COORDS(v, FALSE), b = ENTITY::GET_ENTITY_COORDS(player, TRUE);
            float dx = a.x - b.x, dy = a.y - b.y;
            if (dx * dx + dy * dy < g_cfg.disDeathRadius * g_cfg.disDeathRadius)
                AddPoints(g_cfg.disVehPts, "veiculo destruido na desordem");
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
// v10: no Modo Diretor o mod manda na policia: reaplica tudo todo frame, por cima da configuracao de
// procurado do Modo Diretor (que senao desfaz o que o mod pediu)
static bool s_dmForce = false;

static void ApplyMaxWanted(int v) {
    if (s_dmForce) PLAYER::SET_DISPATCH_COPS_FOR_PLAYER(PLAYER::PLAYER_ID(), v > 0);
    if (v == s_maxWantedApplied && !s_dmForce) return;
    PLAYER::SET_MAX_WANTED_LEVEL(v);
    s_maxWantedApplied = v;
}
static void PoliceIgnore(Player pid, bool on) {
    if (on == s_ignoring && !s_dmForce) return;
    PLAYER::SET_POLICE_IGNORE_PLAYER(pid, on);
    s_ignoring = on;
}
static void ClearWanted(Player pid) {
    PLAYER::SET_PLAYER_WANTED_LEVEL(pid, 0, FALSE);
    PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(pid, FALSE);
}

void PoliceRestore(Player pid) {
    DisorderReset(); // v11
    RelationsDisorder(false, false);
    s_dmForce = false;
    PLAYER::SET_DISPATCH_COPS_FOR_PLAYER(pid, TRUE);
    PoliceIgnore(pid, false);
    ApplyMaxWanted(5);
    s_jointUntil = 0;
    s_wasActive = false;
    g_st.wantedMaxSince = -1;
}

// ---------- v11: modo 4, Desordem ----------
// Comeca no nivel escolhido no menu (ou no StartLevel) com N estrelas (no maximo 5), e nada baixa no primeiro
// minuto. Policia e retaliadores se enfrentam; o jogador so e atacado por quem chegar muito perto ou pelo lado
// que ele atacar primeiro. Qualquer morte ou destruicao perto do jogador sobe a pressao, e as estrelas seguem o
// nivel. Os retaliadores tambem atacam civis, e parte dos civis anda com arma curta.
static bool s_disOn = false;
static int s_disHold = 0, s_disRestartAt = 0, s_disCentre = 0, s_disScan = 0, s_disPrune = 0;
static int s_polAggro = 0, s_hunAggro = 0;
static std::unordered_set<int> s_civChecked;           // civis ja sorteados (arma ou nao)
static std::unordered_map<int, int> s_copNear;          // policial perto do jogador -> quando foi mandado atacar
static std::unordered_map<int, int> s_copHp;            // vida do policial no ultimo exame

static bool MissionPaused() { return g_st.paused && strcmp(g_st.pauseReason, "interior") != 0; }
bool DisorderOn() { return g_cfg.mode == 4 && s_disOn && !MissionPaused(); }
bool DisorderCalm() { return DisorderOn() && g_st.now >= s_hunAggro; }
static bool DisorderHold() { return DisorderOn() && g_st.now < s_disHold; }
void DisorderProvoke(bool police) {
    if (!DisorderOn()) return;
    int &until = police ? s_polAggro : s_hunAggro;
    if (g_st.now >= until)
        Log("Desordem: o jogador atacou %s; eles ficam contra ele por %ds.", police ? "a policia" : "os retaliadores",
            g_cfg.disAggroSeconds);
    until = g_st.now + g_cfg.disAggroSeconds * 1000;
}
void DisorderMenuLevel() {
    if (g_cfg.mode != 4) return;
    s_disOn = true;
    s_disRestartAt = 0;
    s_disHold = g_st.now + g_cfg.disHoldSeconds * 1000;
    Log("Desordem: nivel %d escolhido no menu; nada baixa por %ds.", LevelOf(g_st.Pts()), g_cfg.disHoldSeconds);
}
static void DisorderReset() {
    s_disOn = false;
    s_disHold = s_disRestartAt = s_polAggro = s_hunAggro = 0;
    s_civChecked.clear(); s_copNear.clear(); s_copHp.clear();
}
// morte ou prisao: recomeca no nivel inicial depois de RestartDelay segundos (contados depois de voltar)
static void DisorderOnDeath() {
    if (g_cfg.mode != 4 || !s_disOn) return;
    s_polAggro = s_hunAggro = 0;
    if (!g_cfg.disRestartOnDeath) return; // segue ligada, a partir do zero
    s_disOn = false;
    s_disRestartAt = 1; // marca: comeca a contar quando o jogador voltar
}

// a cada 0,5 s: policiais atacados pelo jogador ou muito perto dele; civis armados
static void DisorderScan(Ped player) {
    if (g_st.now < s_disScan) return;
    s_disScan = g_st.now + 500;
    static const Hash kCivGuns[] = {H("WEAPON_PISTOL"), H("WEAPON_MICROSMG"), H("WEAPON_MINISMG"),
                                    H("WEAPON_MACHINEPISTOL")};
    Vehicle pveh = PED::IS_PED_IN_ANY_VEHICLE(player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(player, FALSE) : 0;
    Vector3 pp = ENTITY::GET_ENTITY_COORDS(player, TRUE);
    float nearD = g_cfg.disNearDist;
    int armed = 0;
    int n = worldGetAllPeds(s_buf, kMaxEnt);
    for (int i = 0; i < n; ++i) {
        Ped p = s_buf[i];
        if (p == player || ENTITY::IS_ENTITY_DEAD(p) || PED::IS_PED_A_PLAYER(p) || ENTITY::IS_ENTITY_A_MISSION_ENTITY(p))
            continue;
        Vector3 q = ENTITY::GET_ENTITY_COORDS(p, TRUE);
        float dx = q.x - pp.x, dy = q.y - pp.y, dz = q.z - pp.z, d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > 160.f * 160.f) continue;
        Victim v = Classify(p);
        if (v == V_COP) {
            Vehicle cv = PED::IS_PED_IN_ANY_VEHICLE(p, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(p, FALSE) : 0;
            int hp = ENTITY::GET_ENTITY_HEALTH(p);
            auto it = s_copHp.find(p);
            bool hurt = it != s_copHp.end() && hp < it->second;
            s_copHp[p] = hp;
            bool byPlayer = ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(p, player, TRUE) ||
                            (pveh && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(p, pveh, TRUE));
            if (hurt && byPlayer) DisorderProvoke(true);
            if (cv && cv != pveh && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(cv, player, TRUE)) {
                DisorderProvoke(true);
                ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(cv);
            }
            if (byPlayer) ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(p);
            // muito perto do jogador: esse policial vai para cima dele
            if (!cv && g_st.now >= s_polAggro && d2 < nearD * nearD) {
                auto c = s_copNear.find(p);
                if (c == s_copNear.end() || g_st.now - c->second > 6000) {
                    AI::TASK_COMBAT_PED(p, player, 0, 16);
                    s_copNear[p] = g_st.now;
                }
            }
        } else if (v == V_CIV && g_cfg.disArmedCivPct > 0 && s_civChecked.insert(p).second) {
            if (Rand(100) < g_cfg.disArmedCivPct) {
                WEAPON::GIVE_WEAPON_TO_PED(p, kCivGuns[Rand(4)], 200, FALSE, FALSE);
                ++armed;
            }
        }
    }
    if (armed) Log("Desordem: %d civil(is) andando com arma curta.", armed);
    if (g_st.now >= s_disPrune) {
        s_disPrune = g_st.now + 10000;
        for (auto it = s_civChecked.begin(); it != s_civChecked.end();)
            it = ENTITY::DOES_ENTITY_EXIST(*it) ? std::next(it) : s_civChecked.erase(it);
        for (auto it = s_copNear.begin(); it != s_copNear.end();)
            it = ENTITY::DOES_ENTITY_EXIST(it->first) ? std::next(it) : s_copNear.erase(it);
        for (auto it = s_copHp.begin(); it != s_copHp.end();)
            it = ENTITY::DOES_ENTITY_EXIST(it->first) ? std::next(it) : s_copHp.erase(it);
    }
}

static void DisorderTick(Player pid, bool missionPause) {
    State &s = g_st;
    s_jointUntil = 0;
    s.wantedMaxSince = -1;
    if (missionPause) { // missoes da historia: tudo normal
        RelationsDisorder(false, false);
        PoliceIgnore(pid, false);
        ApplyMaxWanted(5);
        return;
    }
    if (!s_disOn) {
        if (s_disRestartAt == 1) s_disRestartAt = s.now + g_cfg.disRestartDelay * 1000;
        if (s.now >= s_disRestartAt) {
            s_disOn = true;
            s_disRestartAt = 0;
            s_disHold = s.now + g_cfg.disHoldSeconds * 1000;
            int lv = g_cfg.disStartLevel < 1 ? 1 : g_cfg.disStartLevel > (int)g_cfg.levelStart.size()
                                                     ? (int)g_cfg.levelStart.size() : g_cfg.disStartLevel;
            int start = g_cfg.levelStart[lv - 1];
            if (s.Pts() < start) AddPointsForce(start - s.Pts(), "a Desordem comeca");
            int l = LevelOf(s.Pts());
            Log("Desordem: comeca no nivel %d com %d estrela(s); nada baixa por %ds.", l, l > 5 ? 5 : l,
                g_cfg.disHoldSeconds);
        }
    }
    bool on = s_disOn;
    int lv = LevelOf(s.Pts());
    int stars = lv > 5 ? 5 : lv;
    ApplyMaxWanted(5);
    // as estrelas seguem o nivel (nivel N = N estrelas). Sem nivel, a policia age como no jogo normal
    if (on && stars > 0 && PLAYER::GET_PLAYER_WANTED_LEVEL(pid) != stars) {
        PLAYER::SET_PLAYER_WANTED_LEVEL(pid, stars, FALSE);
        PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(pid, FALSE);
    }
    // policia na area, mas ignorando o jogador (enfrenta os retaliadores), a nao ser que ele ataque a policia
    bool polCalm = on && stars > 0 && s.now >= s_polAggro;
    PoliceIgnore(pid, polCalm);
    if (polCalm && s.now >= s_disCentre) {
        s_disCentre = s.now + 3000;
        Vector3 p = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
        invoke<Void>(0x520E541A97A13354, pid, &p, FALSE, FALSE); // SET_PLAYER_WANTED_CENTRE_POSITION
    }
    RelationsDisorder(on, s.now >= s_hunAggro);
    bool active = s.Pts() > 0;
    if (s_wasActive && !active) DespawnAll(false, "jogador livre");
    s_wasActive = active;
    if (on) DisorderScan(PLAYER::PLAYER_PED_ID());
}

void CycleTick(Player pid) {
    State &s = g_st;
    bool missionPause0 = s.paused && strcmp(s.pauseReason, "interior") != 0;
    // v10: os 3 modos tambem valem no Modo Diretor, acima da configuracao de policia dele
    if (s_dmForce && !s.director) { // saiu do Modo Diretor: devolve o despacho da policia e reaplica tudo
        PLAYER::SET_DISPATCH_COPS_FOR_PLAYER(pid, TRUE);
        s_maxWantedApplied = -1;
        s_ignoring = !s_ignoring;
    }
    s_dmForce = s.director;
    // v11, modo 4: Desordem
    if (g_cfg.mode == 4) { DisorderTick(pid, missionPause0); return; }
    RelationsDisorder(false, false);
    // v8, modo 1: junto com a policia (original) - o mod nao mexe na policia (no Modo Diretor, forca a policia normal)
    // v8, modo 2: substitui a policia - o jogador nunca ganha estrelas; os retaliadores vem no lugar
    if (g_cfg.mode != 3) {
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
            s_nextCentre = 0;
            if (g_cfg.cycleJointSeconds > 0) {
                s_jointUntil = s.now + g_cfg.cycleJointSeconds * 1000;
                PoliceIgnore(pid, true);
                Log("Fase conjunta por %ds: policia contra os retaliadores.", g_cfg.cycleJointSeconds);
            } else {
                // v10: alterna direto - a policia vai embora e so volta quando a retaliacao acabar
                s_jointUntil = 0;
                ApplyMaxWanted(0);
                ClearWanted(pid);
                Log("Ciclo: a policia saiu, o Retaliation assume ate o jogador ficar livre.");
            }
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
        if (g_cfg.levelLock) Log("Jogador morreu ou foi preso: nivel mantido (trava de nivel).");
        else if (s.Pts() > 0) Log("Jogador morreu ou foi preso: pontos zerados [%s].", kCharName[s.ch]);
        if (!g_cfg.levelLock) s.Pts() = 0;
        DisorderOnDeath(); // v11
        s.maxHeatSince = -1;
        s.hotSince = -1;
        s.wantedMaxSince = -1;
    }
    s_wasDead = dead;
    if (dead) return;

    // v7: acima de HotTimer.Points (350 na escala de 500) por 5 minutos sem morrer nem ser preso -> cai de volta para o nivel 2
    if (g_cfg.hotEnabled && !g_cfg.levelLock) { // v10: com a trava, o timer nao derruba o nivel
        if (s.Pts() >= g_cfg.hotPoints) {
            if (s.hotSince < 0) {
                s.hotSince = s.now;
                Log("Timer de %ds acima de %d pontos iniciado.", g_cfg.hotSeconds, g_cfg.hotPoints);
            } else if (s.now - s.hotSince >= g_cfg.hotSeconds * 1000) {
                s.hotSince = -1;
                // v7: volta para o nivel 2; o jogador escolhe entre voltar ao caos ou se esconder
                int lv = g_cfg.cycleStartLevel;
                int to = lv >= 1 && lv <= (int)g_cfg.levelStart.size() ? g_cfg.levelStart[lv - 1] : 21;
                if (g_cfg.mode == 3) // v10: no ciclo, sobreviver ao timer encerra a retaliacao
                    AddPoints(-s.Pts(), "sobreviveu ao timer acima do limite: fim da retaliacao");
                else if (s.Pts() > to) AddPoints(to - s.Pts(), "sobreviveu 5 minutos acima do limite: volta ao nivel 2");
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
    if (s.Pts() > 0 && !DisorderHold() && s.now - s.lastCrime >= g_cfg.decayInterval * 1000 &&
        s.now - s.lastDecay >= g_cfg.decayInterval * 1000) {
        s.lastDecay = s.now;
        AddPoints(-g_cfg.decayAmount, "tempo sem crimes");
    }

    // v4: o calor maximo foi removido; quem manda agora e o timer de 5 minutos (HotTimer) acima.
}
