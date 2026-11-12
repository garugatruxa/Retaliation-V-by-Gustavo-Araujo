// Waves de retaliadores: grupos a pe, veiculos, barcos, Juggernaut no Insurgent e bloqueios de estrada.
#include "spawn.h"
#include <cmath>

// ---------- utilidades ----------
static unsigned s_rng = 0x9E3779B9u;
int Rand(int n) {
    if (n <= 0) return 0;
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 17; s_rng ^= s_rng << 5;
    return (int)(s_rng % (unsigned)n);
}
float RandF(float a, float b) { return a + (b - a) * (Rand(10001) / 10000.f); }

static Vector3 V3(float x, float y, float z) { Vector3 v{}; v.x = x; v.y = y; v.z = z; return v; }
static float D2(const Vector3 &a, const Vector3 &b) { return std::hypot(a.x - b.x, a.y - b.y); }
static const float kDeg = 3.14159265f / 180.f;
// direcao no GTA: heading 0 = norte (+y), cresce no sentido anti-horario
static void Dir(float headingDeg, float &dx, float &dy) { dx = -std::sin(headingDeg * kDeg); dy = std::cos(headingDeg * kDeg); }
static float HeadingTo(const Vector3 &from, const Vector3 &to) {
    return std::atan2(-(to.x - from.x), to.y - from.y) / kDeg;
}
static float AngDiff(float a, float b) {
    float d = std::fmod(std::fabs(a - b), 360.f);
    return d > 180.f ? 360.f - d : d;
}
static Hash H(const char *s) { return GAMEPLAY::GET_HASH_KEY((char *)s); }

static bool LoadModel(Hash m) {
    if (!m || !STREAMING::IS_MODEL_IN_CDIMAGE(m)) return false;
    STREAMING::REQUEST_MODEL(m);
    DWORD t0 = GetTickCount();
    while (!STREAMING::HAS_MODEL_LOADED(m)) {
        if (GetTickCount() - t0 > 2000) return false;
        HudTick();
        WAIT(0);
    }
    return true;
}
static bool LoadAnimSet(const char *s) {
    STREAMING::REQUEST_ANIM_SET((char *)s);
    DWORD t0 = GetTickCount();
    while (!STREAMING::HAS_ANIM_SET_LOADED((char *)s)) {
        if (GetTickCount() - t0 > 1500) return false;
        WAIT(0);
    }
    return true;
}

// ---------- estado ----------
struct Hunter {
    Ped ped;
    Blip blip = 0;
    int born = 0, leftAt = 0;
    bool reached = false, stayInVeh = false, leaving = false, juggernaut = false, roadblock = false, sniper = false;
    bool tactical = false; // IA estilo NOOSE (so Merryweather e Cacadores)
    bool rangeExempt = false; // pilotos e torretas: armas do veiculo, sem limite de alcance
    bool heliRappel = false;  // vai descer de corda
    int shootRate = -1;
    bool farOk = false; // jato e Cargobob voam longe: so sao removidos a 1500 m
    int nextArea = 0, nextGesture = 0;
    float areaRadius = 30.f;
};
struct SquadVeh { Vehicle veh; int born; bool roadblock; bool scored = false; };
enum PKind { P_FOOT, P_VEH, P_BOAT, P_JUGG, P_ROADBLOCK, P_HELI, P_TANK, P_JET };
struct Pending { int at; PKind k; int n; int role; bool bicycleOk; };

static std::vector<Hunter> s_h;
static std::vector<SquadVeh> s_v;
static std::vector<Pending> s_q;
static int s_nextWave = 0, s_lastUpdate = 0;
static bool s_wasDead = false, s_wasMission = false;
static Ped s_player = 0;
static Vector3 s_ppos{};
static Faction *s_fac = nullptr;
static int s_level = 0;
static const float kBlipRadius = 120.f; // padrao; o .ini pode mudar
static float BlipRadius() { return g_cfg.blipRadius > 0 ? g_cfg.blipRadius : kBlipRadius; }

int HuntersAlive() { return (int)s_h.size(); }
static int Queued() {
    int n = 0;
    for (auto &p : s_q) n += p.k == P_FOOT ? p.n : (p.k == P_ROADBLOCK || p.k == P_JUGG || p.k == P_HELI) ? 4 : 2;
    return n;
}
static int MaxAlive() { return s_level >= 1 && s_level <= 8 ? g_cfg.lv[s_level - 1].maxAlive : 10; }
static int Budget() { return MaxAlive() - HuntersAlive(); }
static bool JuggAlive() {
    for (auto &h : s_h) if (h.juggernaut) return true;
    for (auto &p : s_q) if (p.k == P_JUGG) return true;
    return false;
}
// v5: a partir de Escalation.Start (70) cada ponto soma PerPoint % aos especiais
static float EscBonus() {
    int pts = g_st.Pts();
    return pts > g_cfg.escStart ? (pts - g_cfg.escStart) * g_cfg.escPerPoint : 0.f;
}
// chance com teto: base + bonus, nunca acima de max
static int Pct(int base, float bonus, int max) {
    int v = base + (int)bonus;
    return v > max ? max : v;
}
static float TopBonus() {
    if (g_cfg.levelStart.empty() || LevelOf(g_st.Pts()) < (int)g_cfg.levelStart.size()) return 0.f;
    return (g_st.Pts() - g_cfg.levelStart.back()) * g_cfg.topBonusPerPoint;
}

// ---------- alcance por arma (v5) ----------
// Pistolas, submetralhadoras e escopetas so atiram de perto; fuzis e metralhadoras um pouco mais longe;
// so snipers e RPG atiram assim que veem o jogador.
enum RangeClass { RC_SHORT, RC_MEDIUM, RC_LONG };
static RangeClass RangeOf(Hash w) {
    static const Hash rpg = H("WEAPON_RPG"), homing = H("WEAPON_HOMINGLAUNCHER");
    if (w == rpg || w == homing) return RC_LONG;
    switch ((Hash)WEAPON::GET_WEAPONTYPE_GROUP(w)) {
    case 0xB7BBD827: return RC_LONG;   // snipers
    case 0x39D5C192:                   // fuzis/carabinas
    case 0x451B04BC:                   // metralhadoras
    case 0xA27A4F9F: return RC_MEDIUM; // pesadas (minigun, lanca-granadas)
    default: return RC_SHORT;          // pistolas, SMGs, escopetas, corpo a corpo
    }
}
static float RangeMeters(RangeClass c) {
    return c == RC_SHORT ? g_cfg.rangeShort : c == RC_MEDIUM ? g_cfg.rangeMedium : 0.f; // 0 = sem limite
}

// ---------- criacao de NPCs ----------
static bool IsPro() {
    return s_fac && (!strcmp(s_fac->id, "FIB") || !strcmp(s_fac->id, "CACADORES") || !strcmp(s_fac->id, "MERRYWEATHER"));
}
// IA tatica (estilo NOOSE) so para a elite: Merryweather e Cacadores de recompensa.
static bool IsTactical() {
    return s_fac && (!strcmp(s_fac->id, "CACADORES") || !strcmp(s_fac->id, "MERRYWEATHER"));
}
// Dos 40 pontos para cima, mais colete, capacete e armas melhores (v3).
static int GearPct(int base, float perPoint, int cap) {
    int pts = g_st.Pts();
    if (pts < g_cfg.gearStart) return 0;
    int v = base + (int)((pts - g_cfg.gearStart) * perPoint);
    return v > cap ? cap : v;
}
static int BetterPct() { return GearPct(5, 0.5f, 35); }

static void ApplyGear(Ped ped, int level) {
    int vest = GearPct(15, 1.4f, 90), helmet = GearPct(5, 1.3f, 80) + (IsPro() ? 20 : 0);
    if (Rand(100) < vest) {
        PED::SET_PED_ARMOUR(ped, 100);
        // colete visivel, se o modelo tiver (componente 9 = equipamento/colete)
        int n = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(ped, 9);
        if (n > 1) {
            int d = 1 + Rand(n - 1);
            if (PED::IS_PED_COMPONENT_VARIATION_VALID(ped, 9, d, 0)) PED::SET_PED_COMPONENT_VARIATION(ped, 9, d, 0, 0);
        }
    }
    if (Rand(100) < helmet) {
        int n = PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(ped, 0); // prop 0 = cabeca
        if (n > 0) PED::SET_PED_PROP_INDEX(ped, 0, Rand(n), 0, TRUE);
    }
}

// Gangues e FIB: comportamento da v2. Elite (IsTactical): IA no estilo da NOOSE, a media distancia,
// com cobertura, flanqueando e sem correr de cara no jogador.
static void SetupPed(Ped ped, int level, Hash weapon) {
    const Config::Lv &L = g_cfg.lv[level - 1];
    PED::SET_PED_RELATIONSHIP_GROUP_HASH(ped, s_fac->group);
    PED::SET_PED_MAX_HEALTH(ped, L.health);
    ENTITY::SET_ENTITY_HEALTH(ped, L.health);
    PED::SET_PED_ARMOUR(ped, L.armor);
    PED::SET_PED_ACCURACY(ped, L.accuracy);
    if (weapon) WEAPON::GIVE_WEAPON_TO_PED(ped, weapon, 9999, FALSE, TRUE);
    PED::SET_PED_COMBAT_ABILITY(ped, level >= 6 ? 2 : level >= 3 ? 1 : 0);
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 46, TRUE); // sempre lutar
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 5, TRUE);  // lutar mesmo contra armado
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 0, TRUE);  // usar cobertura
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 2, TRUE);  // atirar de dentro do veiculo
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 1, TRUE);  // usar veiculos
    PED::SET_PED_COMBAT_ATTRIBUTES(ped, 3, TRUE);  // pode sair do veiculo
    if (IsTactical()) {
        PED::SET_PED_COMBAT_ABILITY(ped, 2);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 12, TRUE); // atirar as cegas da cobertura
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 42, TRUE); // flanquear
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 22, TRUE); // arrastar feridos para a cobertura
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 28, TRUE); // avancar quando nao consegue acertar
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 13, FALSE); // nao ser agressivo demais
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 50, FALSE); // nao fazer "carga" de frente
        PED::SET_PED_COMBAT_ATTRIBUTES(ped, 58, TRUE);  // nao fugir do combate
        PED::SET_PED_COMBAT_MOVEMENT(ped, Rand(100) < 70 ? 1 : 2); // maioria defensiva, alguns avancam
        PED::SET_PED_COMBAT_RANGE(ped, 1);
    }
    PED::SET_PED_FLEE_ATTRIBUTES(ped, 0, FALSE);
    PED::SET_PED_SEEING_RANGE(ped, 250.f);
    PED::SET_PED_KEEP_TASK(ped, TRUE);
    if (weapon) {
        RangeClass rc = RangeOf(weapon);
        PED::SET_PED_COMBAT_RANGE(ped, rc == RC_SHORT ? 0 : rc == RC_MEDIUM ? 1 : 2); // chega perto conforme a arma
    }
    ApplyGear(ped, level);
}
static Hunter &AddHunter(Ped ped) {
    Hunter h;
    h.ped = ped;
    h.born = g_st.now;
    h.tactical = IsTactical();
    h.areaRadius = RandF(18.f, 34.f);
    h.nextGesture = g_st.now + 6000 + Rand(8000);
    s_h.push_back(h);
    return s_h.back();
}

static Ped NewPedAt(Hash model, const Vector3 &p, float heading) {
    if (!LoadModel(model)) return 0;
    Ped ped = PED::CREATE_PED(4, model, p.x, p.y, p.z, heading, FALSE, TRUE);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    if (ped) ENTITY::SET_ENTITY_LOAD_COLLISION_FLAG(ped, TRUE); // nao cair pelo mapa longe do jogador
    return ped;
}
static Ped NewPedIn(Vehicle v, Hash model, int seat) {
    if (!LoadModel(model)) return 0;
    Ped ped = PED::CREATE_PED_INSIDE_VEHICLE(v, 4, model, seat, FALSE, TRUE);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    return ped;
}
static Hash RandomPed() { return s_fac->peds.empty() ? 0 : s_fac->peds[Rand((int)s_fac->peds.size())]; }
static Hash RandomOf(const std::vector<Hash> &v) { return v.empty() ? 0 : v[Rand((int)v.size())]; }

static Vehicle NewVehicle(Hash model, const Vector3 &p, float heading, bool ground = true) {
    if (!LoadModel(model)) return 0;
    Vehicle v = VEHICLE::CREATE_VEHICLE(model, p.x, p.y, p.z, heading, FALSE, TRUE);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    if (!v) return 0;
    ENTITY::SET_ENTITY_LOAD_COLLISION_FLAG(v, TRUE);
    if (ground) VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(v);
    return v;
}

// ---------- posicoes ----------
// v4: tudo nasce no chao e no mesmo "andar" do jogador (viaduto em cima, rua embaixo...).
// O chao e sempre procurado a partir da altura do jogador para baixo, entao um viaduto acima dele e ignorado.
static float s_pgz = 0.f; // altura do chao sob o jogador

static bool GroundBelow(float x, float y, float fromZ, float &gz) {
    return GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, fromZ, &gz, 0) != 0;
}

// Caminha do jogador ate o ponto, em passos de ~15 m, seguindo o chao. Recusa se houver um degrau maior que
// maxStep (queda de viaduto, subida para outra pista). strict=false aceita quando o chao ainda nao carregou longe.
static bool SameLevel(float x1, float y1, float &outZ, bool strict) {
    float x0 = s_ppos.x, y0 = s_ppos.y, z = s_pgz;
    float d = std::hypot(x1 - x0, y1 - y0);
    int steps = (int)(d / 15.f) + 1;
    const float maxStep = 4.5f;
    for (int k = 1; k <= steps; ++k) {
        float t = (float)k / steps, g;
        if (!GroundBelow(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z + 3.5f, g)) {
            if (strict) return false;
            break; // longe demais para ter colisao carregada: decide pela tolerancia de altura
        }
        if (std::fabs(g - z) > maxStep) return false;
        z = g;
    }
    outZ = z;
    return true;
}

static void UpdatePlayerGround() {
    float gz;
    s_pgz = GroundBelow(s_ppos.x, s_ppos.y, s_ppos.z + 1.0f, gz) ? gz : s_ppos.z - 1.0f;
}

// Ponto a pe a media distancia, no chao, no mesmo nivel, de preferencia atras da camera e fora da tela.
static bool FindFootPos(float dmin, float dmax, Vector3 &out) {
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z;
    for (int i = 0; i < 18; ++i) {
        float ang = i < 11 ? camH + 180.f + RandF(-80.f, 80.f) : RandF(0.f, 360.f);
        float d = RandF(dmin, dmax), dx, dy;
        Dir(ang, dx, dy);
        float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d, gz = 0.f;
        if (!SameLevel(x, y, gz, true)) continue;
        Vector3 safe;
        if (PATHFIND::GET_SAFE_COORD_FOR_PED(x, y, gz + 1.f, TRUE, &safe, 16) && std::fabs(safe.z - gz) < 1.5f &&
            std::hypot(safe.x - x, safe.y - y) < 12.f) {
            x = safe.x; y = safe.y; gz = safe.z;
        }
        Vector3 c = V3(x, y, gz + 0.1f);
        if (D2(c, s_ppos) < dmin * 0.7f) continue;
        if (CAM::IS_SPHERE_VISIBLE(c.x, c.y, c.z + 1.f, 1.5f)) continue;
        out = c;
        return true;
    }
    return false;
}

// Ponto numa rua do mesmo nivel do jogador, virado para ele.
static bool FindRoadPos(float dmin, float dmax, Vector3 &out, float &heading) {
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z;
    for (int i = 0; i < 16; ++i) {
        float ang = i < 8 ? camH + 180.f + RandF(-90.f, 90.f) : RandF(0.f, 360.f);
        float d = RandF(dmin, dmax), dx, dy;
        Dir(ang, dx, dy);
        Vector3 node;
        float h = 0.f;
        if (!PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(s_ppos.x + dx * d, s_ppos.y + dy * d, s_pgz, &node, &h,
                                                             1, 3.0f, 0))
            continue;
        float nd = D2(node, s_ppos);
        if (nd < dmin * 0.75f || nd > dmax * 1.3f) continue;
        // altura: tolerancia cresce com a distancia (ladeiras), mas pista em outro andar e recusada
        if (std::fabs(node.z - s_pgz) > 6.f + nd * 0.04f) continue;
        float lz;
        if (!SameLevel(node.x, node.y, lz, false)) continue;
        if (nd < 200.f && CAM::IS_SPHERE_VISIBLE(node.x, node.y, node.z + 1.f, 3.f)) continue;
        float to = HeadingTo(node, s_ppos);
        if (AngDiff(h, to) > 90.f) h += 180.f;
        node.z += 0.5f;
        out = node;
        heading = h;
        return true;
    }
    return false;
}
static bool FindWaterPos(Vector3 &out) {
    for (int i = 0; i < 16; ++i) {
        float d = RandF(110.f, 170.f), dx, dy, wh = 0.f;
        Dir(RandF(0.f, 360.f), dx, dy);
        float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d;
        if (WATER::GET_WATER_HEIGHT(x, y, s_ppos.z, &wh)) { out = V3(x, y, wh + 0.5f); return true; }
    }
    return false;
}

// ---------- unidades ----------
static void SpawnFoot(int n, int role) {
    float dmin = g_cfg.footMin, dmax = g_cfg.footMax;
    if (role == 1) { dmin = dmax; dmax += 40.f; } // sniper fica mais longe
    Vector3 p;
    if (!FindFootPos(dmin, dmax, p)) { Log("  a pe: nenhum ponto seguro encontrado."); return; }
    for (int i = 0; i < n && Budget() > 0; ++i) {
        Vector3 q = V3(p.x + RandF(-2.f, 2.f), p.y + RandF(-2.f, 2.f), p.z);
        Ped ped = NewPedAt(RandomPed(), q, HeadingTo(q, s_ppos));
        if (!ped) continue;
        Hash w = WeaponFor(s_level, BetterPct());
        if (i == 0 && role == 1) { Hash sw = SniperFor(s_level); if (sw) w = sw; }
        if (i == 0 && role == 2) { Hash hw = HeavyFor(s_level); if (hw) w = hw; }
        SetupPed(ped, s_level, w);
        if (i == 0 && role == 2) PED::SET_PED_ACCURACY(ped, g_cfg.explosiveAcc); // RPG/lanca-granadas: mira cinematografica
        bool sn = i == 0 && role == 1;
        if (sn) { PED::SET_PED_COMBAT_MOVEMENT(ped, 0); PED::SET_PED_COMBAT_RANGE(ped, 2); }
        AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
        AddHunter(ped).sniper = sn;
    }
    Log("  a pe: %d (%s)%s a %.0fm", n, s_fac->name, role == 1 ? " com sniper" : role == 2 ? " com explosivo" : "",
        D2(p, s_ppos));
}

static void FillVehicle(Vehicle v, int seats) {
    for (int s = -1; s < seats - 1 && Budget() > 0; ++s) {
        Ped ped = NewPedIn(v, RandomPed(), s);
        if (!ped) continue;
        SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
        if (s == -1) {
            PED::SET_DRIVER_ABILITY(ped, 1.0f);
            PED::SET_DRIVER_AGGRESSIVENESS(ped, 0.75f);
            AI::TASK_VEHICLE_CHASE(ped, s_player);
            AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(ped, 0.f);
        } else {
            AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
        }
        AddHunter(ped);
    }
}

// v6: veiculos com torreta (Technical, Caracara, Menacer, Halftrack, Insurgent...) ganham um atirador nela.
static int FillTurrets(Vehicle v) {
    int n = 0, maxP = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v);
    for (int s = 0; s < maxP && Budget() > 0 && n < 2; ++s) {
        if (!invoke<BOOL>(0xE33FFA906CE74880, v, s) || !VEHICLE::IS_VEHICLE_SEAT_FREE(v, s)) continue; // IS_TURRET_SEAT
        Ped g = NewPedIn(v, RandomPed(), s);
        if (!g) continue;
        SetupPed(g, s_level, WeaponFor(s_level, BetterPct()));
        PED::SET_PED_COMBAT_ATTRIBUTES(g, 3, FALSE);
        AI::TASK_COMBAT_PED(g, s_player, 0, 16);
        Hunter &h = AddHunter(g);
        h.stayInVeh = true;
        h.rangeExempt = true;
        ++n;
    }
    return n;
}

static void SpawnVehicle(bool bicycleOk) {
    Hash model = 0;
    int seats = 2;
    float dmin = g_cfg.vehMin, dmax = g_cfg.vehMax;
    const char *kind = "carro";
    if (bicycleOk && s_level <= 2 && !s_fac->bicycles.empty() && Rand(100) < 40) {
        model = RandomOf(s_fac->bicycles); seats = 1; dmin = 80.f; dmax = 120.f; kind = "bicicleta";
    } else if (!s_fac->bikes.empty() && (s_fac->cars.empty() || Rand(100) < s_fac->bikePct)) {
        model = RandomOf(s_fac->bikes); seats = 2; kind = "moto";
    } else {
        model = RandomOf(s_fac->cars); seats = 2 + Rand(3); // 2 a 4
    }
    if (!model) return;
    Vector3 p; float h;
    if (!FindRoadPos(dmin, dmax, p, h)) { Log("  %s: nenhuma rua encontrada.", kind); return; }
    Vehicle v = NewVehicle(model, p, h);
    if (!v) { Log("  %s: falha ao criar o veiculo.", kind); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);
    int maxSeats = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v) + 1;
    if (seats > maxSeats) seats = maxSeats;
    FillVehicle(v, seats);
    int gunners = FillTurrets(v);
    s_v.push_back({v, g_st.now, false});
    Log("  %s com %d%s (%s) a %.0fm", kind, seats, gunners ? " + atirador na torreta" : "", s_fac->name, D2(p, s_ppos));
}

static void SpawnBoat() {
    Hash model = RandomOf(s_fac->boats);
    Vector3 p;
    if (!model || !FindWaterPos(p)) { Log("  barco: sem agua por perto."); return; }
    Vehicle v = NewVehicle(model, p, HeadingTo(p, s_ppos));
    if (!v) return;
    VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);
    int maxSeats = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v) + 1;
    FillVehicle(v, maxSeats < 3 ? maxSeats : 3);
    s_v.push_back({v, g_st.now, false});
    Log("  barco (%s) a %.0fm", s_fac->name, D2(p, s_ppos));
}

// Arma do Juggernaut: Combat MG ou escopeta automatica de tambor (comuns), minigun (rara, sobe com os pontos).
static Hash JuggWeapon() {
    if (Rand(100) < Pct(g_cfg.jugMinigunPct, EscBonus() * 0.3f, g_cfg.jugMinigunMax)) return H("WEAPON_MINIGUN");
    const char *w = Rand(2) ? "WEAPON_COMBATMG" : "WEAPON_ASSAULTSHOTGUN";
    if (!strcmp(w, "WEAPON_COMBATMG") && WEAPON::IS_WEAPON_VALID(H("WEAPON_COMBATMG_MK2")) && Rand(100) < 30)
        w = "WEAPON_COMBATMG_MK2";
    return H(w);
}

// Juggernaut sentado na cacamba do Insurgent, com motorista e atirador na torreta .50.
static void SpawnJuggernaut() {
    Hash model = 0;
    for (const char *n : {"insurgent", "insurgent2"})
        if (STREAMING::IS_MODEL_IN_CDIMAGE(H(n))) { model = H(n); break; }
    if (!model) model = RandomOf(s_fac->cars);
    Vector3 p; float h;
    if (!model || !FindRoadPos(g_cfg.vehMin, g_cfg.vehMax, p, h)) { Log("  Juggernaut: nenhuma rua encontrada."); return; }
    Vehicle v = NewVehicle(model, p, h);
    if (!v) { Log("  Juggernaut: falha ao criar o Insurgent."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);

    int maxP = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v);
    int turret = -2, jugSeat = -2;
    std::string layout;
    for (int s = 0; s < maxP; ++s) {
        bool t = invoke<BOOL>(0xE33FFA906CE74880, v, s) != 0; // IS_TURRET_SEAT
        if (t && turret == -2) turret = s;
        layout += t ? "T" : "-";
    }
    for (int s = maxP - 1; s >= 0; --s)
        if (s != turret) { jugSeat = s; break; } // ultimo assento comum = cacamba
    Log("  Insurgent: %d assentos de passageiro [%s], torreta=%d, Juggernaut no assento %d", maxP, layout.c_str(),
        turret, jugSeat);

    // motorista e atirador ficam no veiculo
    for (int s : {-1, turret}) {
        if (s == -2 || Budget() <= 0) continue;
        Ped ped = NewPedIn(v, RandomPed(), s);
        if (!ped) continue;
        SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
        if (s == -1) {
            PED::SET_DRIVER_ABILITY(ped, 1.0f);
            PED::SET_DRIVER_AGGRESSIVENESS(ped, 0.8f);
            AI::TASK_VEHICLE_CHASE(ped, s_player);
            AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(ped, 25.f);
        } else {
            AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
        }
        AddHunter(ped).stayInVeh = true;
    }

    // o Juggernaut
    if (jugSeat != -2) {
        // v8: humano com a armadura pesada do assalto de Paleto Bay (roupa copiada do jogo);
        // sem ela, um mercenario da Merryweather com capacete e colete. Nunca o robo do Online.
        const Config::Outfit &of = g_cfg.jugOutfit;
        bool suit = of.valid && STREAMING::IS_MODEL_IN_CDIMAGE(of.model);
        Hash jm = suit ? (Hash)of.model : H("s_m_y_blackops_01");
        if (!STREAMING::IS_MODEL_IN_CDIMAGE(jm)) jm = RandomPed();
        Ped j = NewPedIn(v, jm, jugSeat);
        if (j) {
            if (suit) {
                OutfitApply(j, of);
            } else {
                Log("  Juggernaut sem a armadura de Paleto copiada: usando mercenario com capacete e colete.");
                int n = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(j, 9);
                if (n > 1) PED::SET_PED_COMPONENT_VARIATION(j, 9, n - 1, 0, 0);
                if (PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(j, 0) > 0) PED::SET_PED_PROP_INDEX(j, 0, 0, 0, TRUE);
            }
            Hash jw = JuggWeapon();
            SetupPed(j, s_level, jw);
            PED::SET_PED_MAX_HEALTH(j, g_cfg.jugHealth);
            ENTITY::SET_ENTITY_HEALTH(j, g_cfg.jugHealth);
            PED::SET_PED_ARMOUR(j, 100);
            PED::SET_PED_SUFFERS_CRITICAL_HITS(j, FALSE);
            PED::SET_PED_CAN_RAGDOLL(j, FALSE);
            PED::SET_PED_RAGDOLL_ON_COLLISION(j, FALSE);
            PED::SET_PED_FIRING_PATTERN(j, 0xC6EE6B4C); // rajada continua
            PED::SET_PED_COMBAT_MOVEMENT(j, 2);           // avanca
            PED::SET_PED_COMBAT_ATTRIBUTES(j, 0, FALSE);  // nao usa cobertura
            PED::SET_PED_COMBAT_RANGE(j, 1);
            AI::TASK_COMBAT_PED(j, s_player, 0, 16);
            Hunter &hj = AddHunter(j);
            hj.juggernaut = true;
            Log("  JUGGERNAUT a caminho (%s, %s) a %.0fm", s_fac->name,
                jw == H("WEAPON_MINIGUN") ? "minigun" : jw == H("WEAPON_ASSAULTSHOTGUN") ? "escopeta automatica" : "Combat MG",
                D2(p, s_ppos));
        }
    }

    // o resto dos assentos com a tropa
    for (int s = 0; s < maxP && Budget() > 0; ++s) {
        if (s == turret || s == jugSeat || !VEHICLE::IS_VEHICLE_SEAT_FREE(v, s)) continue;
        if (s >= 3) break; // ate 3 passageiros alem do Juggernaut e da torreta
        Ped ped = NewPedIn(v, RandomPed(), s);
        if (!ped) continue;
        SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
        AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
        AddHunter(ped);
    }
    s_v.push_back({v, g_st.now, false});
}

static void ApplyJuggernautWalk(Ped j) {
    if (LoadAnimSet("ANIM_GROUP_MOVE_BALLISTIC")) PED::SET_PED_MOVEMENT_CLIPSET(j, (char *)"ANIM_GROUP_MOVE_BALLISTIC", 1.f);
    if (LoadAnimSet("MOVE_STRAFE@BALLISTIC")) PED::SET_PED_STRAFE_CLIPSET(j, (char *)"MOVE_STRAFE@BALLISTIC");
}

// Bloqueio de estrada a frente do jogador: carros atravessados e atiradores atras deles.
static bool SpawnRoadblock() {
    Ped pl = s_player;
    Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(pl, FALSE);
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(pv ? pv : pl);
    float sp = std::hypot(vel.x, vel.y), fx, fy;
    if (sp > 4.f) { fx = vel.x / sp; fy = vel.y / sp; }
    else Dir(ENTITY::GET_ENTITY_HEADING(pv ? pv : pl), fx, fy);

    Vector3 node; float nh = 0.f; bool ok = false;
    for (float d : {300.f, 260.f, 220.f, 340.f}) {
        Vector3 n; float h;
        if (!PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(s_ppos.x + fx * d, s_ppos.y + fy * d, s_pgz, &n, &h, 1,
                                                             3.0f, 0))
            continue;
        float nd = D2(n, s_ppos);
        if (nd < 180.f || nd > 380.f) continue;
        float dot = ((n.x - s_ppos.x) * fx + (n.y - s_ppos.y) * fy) / nd;
        if (dot < 0.8f) continue;
        if (std::fabs(n.z - s_pgz) > 6.f + nd * 0.04f) continue; // mesma pista/andar do jogador
        float lz;
        if (!SameLevel(n.x, n.y, lz, false)) continue;
        node = n; nh = h; ok = true;
        break;
    }
    if (!ok || s_fac->cars.empty()) return false;

    float rx, ry; // direita da pista
    Dir(nh - 90.f, rx, ry);
    float ax = (node.x - s_ppos.x), ay = (node.y - s_ppos.y), al = std::hypot(ax, ay);
    ax /= al; ay /= al; // "atras" do bloqueio, visto do jogador
    int nveh = s_level >= 6 ? 3 : 2, perVeh = s_level >= 6 ? 3 : 2;
    const float offs[3] = {-3.4f, 3.4f, 0.f};
    int men = 0;
    for (int i = 0; i < nveh; ++i) {
        float o = offs[i];
        Vector3 vp = V3(node.x + rx * o + (i == 2 ? ax * 5.f : 0.f), node.y + ry * o + (i == 2 ? ay * 5.f : 0.f),
                        node.z + 0.5f);
        Vehicle v = NewVehicle(RandomOf(s_fac->cars), vp, nh + 90.f + RandF(-12.f, 12.f));
        if (!v) continue;
        VEHICLE::SET_VEHICLE_ENGINE_ON(v, FALSE, TRUE, FALSE);
        s_v.push_back({v, g_st.now, true});
        for (int k = 0; k < perVeh && Budget() > 0; ++k) {
            float side = (k - (perVeh - 1) * 0.5f) * 1.6f;
            float x = vp.x + ax * 3.f + rx * side, y = vp.y + ay * 3.f + ry * side, gz = vp.z;
            if (!GroundBelow(x, y, node.z + 3.f, gz) || std::fabs(gz - node.z) > 3.f) gz = node.z;
            Vector3 q = V3(x, y, gz + 0.1f);
            Ped ped = NewPedAt(RandomPed(), q, HeadingTo(q, s_ppos));
            if (!ped) continue;
            SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
            PED::SET_PED_COMBAT_MOVEMENT(ped, 1); // defensivo
            PED::SET_PED_SPHERE_DEFENSIVE_AREA(ped, node.x, node.y, node.z, 18.f, FALSE, FALSE);
            AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
            AddHunter(ped).roadblock = true;
            ++men;
        }
    }
    Log("  BLOQUEIO de estrada (%s): %d carros, %d atiradores a %.0fm a frente", s_fac->name, nveh, men,
        D2(node, s_ppos));
    return true;
}

// ---------- helicopteros (v5) ----------
// Tipos: atiradores (Frogger/Maverick com tropa atirando das portas), rapel (Maverick/Annihilator para perto
// do jogador e a tropa desce de corda) e ataque (Buzzard, mais raro). Matar o piloto derruba o helicoptero.
enum HeliType { HT_SHOOTERS, HT_RAPPEL, HT_ATTACK };
// state: 0 chegando, 1 rapel, 2 rondando, 10 pousando, 11 desembarcando
struct HeliSt { Vehicle veh; Ped pilot; HeliType type; int state; int at; Vector3 drop; bool land; };
static std::vector<HeliSt> s_helis;
static std::vector<Hash> s_genericHelis;

static int HelisQueued() {
    int n = 0;
    for (auto &p : s_q) if (p.k == P_HELI) ++n;
    return n;
}
static int HeliLimit(bool air) {
    int lim = air ? g_cfg.heliMaxAir : s_level >= 7 ? g_cfg.heliMaxTop : s_level == 6 ? 2 : g_cfg.heliMax;
    return lim > 3 ? 3 : lim; // v6: nunca mais de 3 ao mesmo tempo
}
static bool CanRappel(Hash m) {
    static const Hash a = H("maverick"), b = H("annihilator"), c = H("polmav");
    return m == a || m == b || m == c;
}
static void PilotCircle(Ped pilot, Vehicle v, float radius) {
    AI::TASK_HELI_MISSION(pilot, v, 0, s_player, 0.f, 0.f, 0.f, 9, 35.f, radius, -1.f, 40, 25, -1.f, 0);
}
static void PilotLand(Ped pilot, Vehicle v, const Vector3 &p) {
    // missao "ir ate" com a flag 32 (pousar ao chegar)
    AI::TASK_HELI_MISSION(pilot, v, 0, 0, p.x, p.y, p.z, 4, 30.f, 3.f, -1.f, 0, 0, -1.f, 32);
}
static void PilotGoTo(Ped pilot, Vehicle v, const Vector3 &p) {
    AI::TASK_HELI_MISSION(pilot, v, 0, 0, p.x, p.y, p.z + 22.f, 4, 40.f, 5.f, -1.f, 30, 18, -1.f, 0);
}

// Ponto no chao perto do jogador, no mesmo nivel, para a tropa descer.
static Vector3 DropPoint() {
    for (int i = 0; i < 10; ++i) {
        float dx, dy, d = RandF(18.f, 32.f), gz;
        Dir(RandF(0.f, 360.f), dx, dy);
        float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d;
        if (SameLevel(x, y, gz, true)) return V3(x, y, gz);
    }
    return V3(s_ppos.x, s_ppos.y, s_pgz);
}

static int HelisNow() { return (int)s_helis.size() + HelisQueued(); }
// chance de colocar mais um helicoptero, dado quantos ja estao no ar
static int NextHeliPct(int n) {
    if (n == 0) return (int)(g_cfg.heliChance[s_level - 1] + TopBonus() + EscBonus() * 0.5f);
    if (n == 1) return s_level >= 6 ? Pct(g_cfg.heliSecondPct, EscBonus() * 0.33f + (g_st.Pts() >= 90 ? 5 : 0), g_cfg.heliSecondMax) : 0;
    if (n == 2) return s_level >= 8 ? Pct(g_cfg.heliThirdPct, TopBonus() * 0.3f, g_cfg.heliThirdMax) : 0;
    return 0;
}

static void SpawnHeli(bool air) {
    const std::vector<Hash> &transport = !s_fac->helis.empty() ? s_fac->helis : s_genericHelis;
    HeliType type = HT_SHOOTERS;
    Hash model = 0;
    if (!s_fac->attackHelis.empty() &&
        Rand(100) < Pct(g_cfg.heliAttackPct, EscBonus() * 0.25f, g_cfg.heliAttackMax) + (air ? 10 : 0)) {
        type = HT_ATTACK;
        model = RandomOf(s_fac->attackHelis);
    } else if (!air && Rand(100) < g_cfg.heliRappelPct) {
        std::vector<Hash> rp;
        for (Hash m : transport) if (CanRappel(m)) rp.push_back(m);
        if (!rp.empty()) { type = HT_RAPPEL; model = RandomOf(rp); }
    }
    if (!model) model = RandomOf(transport);
    if (!model) return;

    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z, dx, dy, d = RandF(330.f, 430.f);
    Dir(camH + 180.f + RandF(-100.f, 100.f), dx, dy);
    float baseZ = air ? s_ppos.z : s_pgz;
    Vector3 p = V3(s_ppos.x + dx * d, s_ppos.y + dy * d, baseZ + (air ? RandF(-10.f, 25.f) : 70.f));
    Vehicle v = NewVehicle(model, p, HeadingTo(p, s_ppos), false);
    if (!v) { Log("  helicoptero: falha ao criar."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);
    VEHICLE::SET_HELI_BLADES_FULL_SPEED(v);
    VEHICLE::SET_VEHICLE_FORWARD_SPEED(v, 25.f);

    Ped pilot = NewPedIn(v, RandomPed(), -1);
    if (!pilot) { VEHICLE::DELETE_VEHICLE(&v); return; }
    SetupPed(pilot, s_level, WeaponFor(s_level, BetterPct()));
    PED::SET_PED_COMBAT_ATTRIBUTES(pilot, 3, FALSE); // piloto nao sai do helicoptero
    Hunter &hp = AddHunter(pilot);
    hp.stayInVeh = true;
    hp.rangeExempt = true;

    int maxP = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v), crew = 0;
    {
        int first = maxP > 1 ? 1 : 0; // portas de tras; o copiloto fica vazio
        int last = type == HT_RAPPEL ? (maxP - 1 < 4 ? maxP - 1 : 4) : (maxP - 1 < 3 ? maxP - 1 : 3);
        if (type == HT_ATTACK && last > 2) last = 2; // Buzzard: ate 2 na traseira, para poder desembarcar
        for (int s = first; s <= last && Budget() > 0; ++s) {
            Ped ped = NewPedIn(v, RandomPed(), s);
            if (!ped) continue;
            SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
            PED::SET_PED_COMBAT_ATTRIBUTES(ped, 3, FALSE);
            if (type != HT_RAPPEL) AI::TASK_COMBAT_PED(ped, s_player, 0, 16);
            Hunter &h = AddHunter(ped);
            h.stayInVeh = true;
            h.heliRappel = type == HT_RAPPEL;
            ++crew;
        }
    }

    // v5: ataque e rapel podem decidir pousar perto do jogador (so se ele estiver a pe)
    bool land = !air && type != HT_SHOOTERS && crew > 0 && Rand(100) < g_cfg.heliLandPct;
    HeliSt st{v, pilot, type, 0, g_st.now, {}, land};
    if (type == HT_ATTACK) {
        PED::SET_PED_ACCURACY(pilot, g_cfg.explosiveAcc); // misseis passam perto em vez de acertar em cheio
        AI::TASK_COMBAT_PED(pilot, s_player, 0, 16); // usa as armas do Buzzard
    } else if (type == HT_RAPPEL) {
        st.drop = DropPoint();
        PilotGoTo(pilot, v, st.drop);
    } else {
        PilotCircle(pilot, v, air ? 60.f : 38.f);
    }
    s_helis.push_back(st);
    s_v.push_back({v, g_st.now, false});
    static const char *kName[] = {"atiradores", "rapel", "ataque"};
    Log("  HELICOPTERO (%s, %s) com %d tripulantes a %.0fm", kName[type], s_fac->name, crew, D2(p, s_ppos));
}

static void UpdateHelis() {
    for (size_t i = 0; i < s_helis.size();) {
        HeliSt &st = s_helis[i];
        bool alive = ENTITY::DOES_ENTITY_EXIST(st.veh) && !ENTITY::IS_ENTITY_DEAD(st.veh) &&
                     ENTITY::DOES_ENTITY_EXIST(st.pilot) && !PED::IS_PED_DEAD_OR_DYING(st.pilot, TRUE);
        if (!alive) { s_helis.erase(s_helis.begin() + i); continue; } // sem piloto ele cai sozinho
        Vector3 hp = ENTITY::GET_ENTITY_COORDS(st.veh, TRUE);

        bool onFoot = !PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE);

        // v5: pouso perto do jogador a pe; a tropa desce e vai atras dele
        if (st.land && st.state == 0 && onFoot && D2(hp, s_ppos) < 160.f) {
            Vector3 lp = DropPoint();
            PilotLand(st.pilot, st.veh, lp);
            st.drop = lp;
            st.state = 10;
            st.at = g_st.now;
            Log("  helicoptero decidiu pousar perto do jogador.");
        } else if (st.state == 10) {
            if (!onFoot) { // jogador entrou num veiculo: desiste de pousar
                st.land = false;
                st.state = 0;
                st.at = g_st.now;
                if (st.type == HT_ATTACK) AI::TASK_COMBAT_PED(st.pilot, s_player, 0, 16);
                else PilotGoTo(st.pilot, st.veh, st.drop = DropPoint());
            } else {
                Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(st.veh);
                bool down = ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(st.veh) < 3.f && std::hypot(vel.x, vel.y) < 3.f;
                if (down || g_st.now - st.at > 45000) {
                    for (Hunter &h : s_h)
                        if (h.ped != st.pilot && PED::IS_PED_IN_VEHICLE(h.ped, st.veh, FALSE)) {
                            h.stayInVeh = false;
                            h.heliRappel = false;
                            h.leaving = true;
                            h.leftAt = g_st.now;
                            AI::TASK_LEAVE_VEHICLE(h.ped, st.veh, 0);
                        }
                    st.state = 11;
                    st.at = g_st.now;
                    Log("  helicoptero pousou: tropa desembarcando.");
                }
            }
        } else if (st.state == 11 && g_st.now - st.at > 6000) {
            st.state = 2;
            st.land = false;
            if (st.type == HT_ATTACK) AI::TASK_COMBAT_PED(st.pilot, s_player, 0, 16); // decola e volta a atacar
            else PilotCircle(st.pilot, st.veh, 55.f);
        }

        if (st.type == HT_RAPPEL && st.state == 0 && !st.land) {
            // segue o jogador se ele se afastar do ponto de descida
            if (D2(st.drop, s_ppos) > 45.f && g_st.now - st.at > 5000) {
                st.drop = DropPoint();
                st.at = g_st.now;
                PilotGoTo(st.pilot, st.veh, st.drop);
            }
            bool overDrop = D2(hp, st.drop) < 20.f && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(st.veh) < 38.f;
            if (overDrop || g_st.now - st.at > 60000) {
                for (Hunter &h : s_h)
                    if (h.heliRappel && PED::IS_PED_IN_VEHICLE(h.ped, st.veh, FALSE))
                        invoke<Void>(0x09693B0312F91649, h.ped, 10.0f); // TASK_RAPPEL_FROM_HELI
                st.state = 1;
                st.at = g_st.now;
                Log("  rapel: tropa descendo de corda.");
            }
        } else if (st.type == HT_RAPPEL && st.state == 1) {
            bool pending = false;
            for (Hunter &h : s_h) {
                if (!h.heliRappel) continue;
                bool out = !PED::IS_PED_IN_ANY_VEHICLE(h.ped, FALSE);
                if ((out && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(h.ped) < 2.5f) || g_st.now - st.at > 25000) {
                    h.heliRappel = false;
                    h.stayInVeh = false;
                    AI::TASK_COMBAT_PED(h.ped, s_player, 0, 16);
                } else {
                    pending = true;
                }
            }
            if (!pending) { st.state = 2; PilotCircle(st.pilot, st.veh, 55.f); } // depois fica rondando
        }
        ++i;
    }
}

// ---------- tanque entregue pelo Cargobob (v6) ----------
// O Cargobob chega com o Rhino pendurado, solta num ponto aberto perto do jogador e vai embora.
// Abater o Cargobob antes derruba o tanque junto. Um tanque por vez.
struct TankSt { Vehicle cargo = 0, rhino = 0; Ped pilot = 0, driver = 0; int state = -1, at = 0; Vector3 drop{}; bool planB = false; };
static TankSt s_tank;
static bool TankActive() {
    if (s_tank.state >= 0) return true;
    for (auto &p : s_q) if (p.k == P_TANK) return true; // ja na fila
    return false;
}

static Vector3 PointAround(float dmin, float dmax) {
    for (int i = 0; i < 14; ++i) {
        float dx, dy, d = RandF(dmin, dmax), gz;
        Dir(RandF(0.f, 360.f), dx, dy);
        float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d;
        if (SameLevel(x, y, gz, i < 9)) return V3(x, y, gz);
    }
    return V3(s_ppos.x, s_ppos.y, s_pgz);
}

static Hunter &AddCrew(Ped p) {
    Hunter &h = AddHunter(p);
    h.stayInVeh = true;
    h.rangeExempt = true;
    h.reached = true;
    h.farOk = true;
    return h;
}

static void SpawnTank() {
    Hash cb = H("cargobob"), rh = H("rhino");
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(cb) || !STREAMING::IS_MODEL_IN_CDIMAGE(rh)) { Log("  tanque: modelos nao encontrados."); return; }
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z, dx, dy, d = RandF(380.f, 450.f);
    Dir(camH + 180.f + RandF(-90.f, 90.f), dx, dy);
    Vector3 p = V3(s_ppos.x + dx * d, s_ppos.y + dy * d, s_pgz + 85.f);
    TankSt t;
    t.cargo = NewVehicle(cb, p, HeadingTo(p, s_ppos), false);
    if (!t.cargo) { Log("  tanque: falha ao criar o Cargobob."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(t.cargo, TRUE, TRUE, FALSE);
    VEHICLE::SET_HELI_BLADES_FULL_SPEED(t.cargo);
    VEHICLE::_ENABLE_CARGOBOB_HOOK(t.cargo, 0); // gancho
    t.rhino = NewVehicle(rh, V3(p.x, p.y, p.z - 10.f), HeadingTo(p, s_ppos), false);
    if (!t.rhino) { VEHICLE::DELETE_VEHICLE(&t.cargo); Log("  tanque: falha ao criar o Rhino."); return; }
    ENTITY::SET_ENTITY_INVINCIBLE(t.rhino, TRUE); // imune ate tocar o chao
    VEHICLE::ATTACH_VEHICLE_TO_CARGOBOB(t.rhino, t.cargo, -1, 0.f, 0.f, 0.f);

    t.pilot = NewPedIn(t.cargo, RandomPed(), -1);
    t.driver = NewPedIn(t.rhino, RandomPed(), -1);
    if (!t.pilot || !t.driver) Log("  tanque: falha ao criar a tripulacao.");
    if (t.pilot) { SetupPed(t.pilot, s_level, WeaponFor(s_level)); PED::SET_PED_COMBAT_ATTRIBUTES(t.pilot, 3, FALSE); AddCrew(t.pilot); }
    if (t.driver) {
        SetupPed(t.driver, s_level, WeaponFor(s_level));
        PED::SET_PED_COMBAT_ATTRIBUTES(t.driver, 3, FALSE);
        PED::SET_PED_ACCURACY(t.driver, g_cfg.explosiveAcc); // canhao do tanque impreciso
        // pendurado no Cargobob o tanque nao atira; o canhao so libera depois de tocar o chao
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, TRUE);
        PED::SET_PED_SHOOT_RATE(t.driver, 0);
        VEHICLE::DISABLE_VEHICLE_WEAPON(TRUE, H("VEHICLE_WEAPON_TANK"), t.rhino, t.driver);
        AddCrew(t.driver);
    }
    VEHICLE::SET_VEHICLE_FORWARD_SPEED(t.cargo, 20.f);
    t.drop = PointAround(110.f, 140.f);
    if (t.pilot) PilotGoTo(t.pilot, t.cargo, t.drop);
    t.state = 0;
    t.at = g_st.now;
    s_tank = t;
    s_v.push_back({t.cargo, g_st.now, false});
    s_v.push_back({t.rhino, g_st.now, false});
    Log("  TANQUE a caminho no Cargobob (%s) a %.0fm; entrega a %.0fm do jogador", s_fac->name, D2(p, s_ppos),
        D2(t.drop, s_ppos));
}

static void ReleaseTankFromHook(TankSt &t) {
    if (t.planB) ENTITY::DETACH_ENTITY(t.rhino, TRUE, TRUE);
    else VEHICLE::DETACH_VEHICLE_FROM_CARGOBOB(t.rhino, t.cargo);
}

static void UpdateTank() {
    TankSt &t = s_tank;
    if (t.state < 0) return;
    bool rhinoOk = ENTITY::DOES_ENTITY_EXIST(t.rhino) && !ENTITY::IS_ENTITY_DEAD(t.rhino);
    if (!rhinoOk) { t.state = -1; Log("Tanque destruido."); return; }
    bool cargoOk = ENTITY::DOES_ENTITY_EXIST(t.cargo) && !ENTITY::IS_ENTITY_DEAD(t.cargo) && t.pilot &&
                   !PED::IS_PED_DEAD_OR_DYING(t.pilot, TRUE);
    if (t.state == 0) {
        // plano B: se o gancho nao pegou, prende o tanque direto embaixo do Cargobob
        if (cargoOk && !t.planB && g_st.now - t.at > 1500 && !VEHICLE::IS_VEHICLE_ATTACHED_TO_CARGOBOB(t.rhino, t.cargo)) {
            ENTITY::ATTACH_ENTITY_TO_ENTITY(t.rhino, t.cargo, 0, 0.f, 0.f, -7.f, 0.f, 0.f, 0.f, FALSE, FALSE, FALSE, FALSE, 2, TRUE);
            t.planB = true;
            Log("  tanque: gancho nao pegou, preso direto no Cargobob (plano B).");
        }
        bool there = cargoOk && D2(ENTITY::GET_ENTITY_COORDS(t.cargo, TRUE), t.drop) < 18.f &&
                     ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(t.cargo) < 42.f;
        if (!cargoOk || there || g_st.now - t.at > 70000) {
            ReleaseTankFromHook(t);
            if (cargoOk) { // vai embora
                float ax = s_ppos.x - t.drop.x, ay = s_ppos.y - t.drop.y, al = std::hypot(ax, ay) + 0.01f;
                Vector3 away = V3(t.drop.x - ax / al * 900.f, t.drop.y - ay / al * 900.f, t.drop.z + 120.f);
                AI::TASK_HELI_MISSION(t.pilot, t.cargo, 0, 0, away.x, away.y, away.z, 4, 50.f, 10.f, -1.f, 150, 80, -1.f, 0);
            }
            t.state = 1;
            t.at = g_st.now;
            Log(cargoOk ? "  tanque solto perto do jogador!" : "  Cargobob abatido: o tanque caiu.");
        }
    } else if (t.state == 1) {
        if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(t.rhino) < 1.5f || g_st.now - t.at > 8000) {
            ENTITY::SET_ENTITY_INVINCIBLE(t.rhino, FALSE);
            if (t.driver && !PED::IS_PED_DEAD_OR_DYING(t.driver, TRUE)) {
                // no chao: libera o canhao e comeca a cacar
                VEHICLE::DISABLE_VEHICLE_WEAPON(FALSE, H("VEHICLE_WEAPON_TANK"), t.rhino, t.driver);
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, FALSE);
                PED::SET_PED_SHOOT_RATE(t.driver, 100);
                AI::TASK_COMBAT_PED(t.driver, s_player, 0, 16);
            }
            t.state = 2;
            Log("  tanque no chao, cacando o jogador.");
        }
    } else if (t.state == 2 && (!t.driver || PED::IS_PED_DEAD_OR_DYING(t.driver, TRUE))) {
        t.state = -1;
        Log("Motorista do tanque abatido.");
    }
}

// ---------- jato (v6) ----------
// Lazer (padrao) ou Hydra (mais raro). Ataca alternando canhao e misseis e vai embora depois de um tempo.
struct JetSt { Vehicle v = 0; Ped pilot = 0; int born = 0, nextSwap = 0, wi = 0; bool leaving = false; };
static JetSt s_jet;
static bool JetActive() {
    if (s_jet.v) return true;
    for (auto &p : s_q) if (p.k == P_JET) return true;
    return false;
}

static void SpawnJet() {
    Hash m = H("lazer");
    if (Rand(100) < g_cfg.jetHydraPct && STREAMING::IS_MODEL_IN_CDIMAGE(H("hydra"))) m = H("hydra");
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(m)) { Log("  jato: modelo nao encontrado."); return; }
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z, dx, dy, d = RandF(850.f, 1000.f);
    Dir(camH + 180.f + RandF(-60.f, 60.f), dx, dy);
    float base = s_ppos.z > s_pgz ? s_ppos.z : s_pgz;
    Vector3 p = V3(s_ppos.x + dx * d, s_ppos.y + dy * d, base + 180.f);
    JetSt j;
    j.v = NewVehicle(m, p, HeadingTo(p, s_ppos), false);
    if (!j.v) { Log("  jato: falha ao criar."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(j.v, TRUE, TRUE, FALSE);
    VEHICLE::_SET_VEHICLE_LANDING_GEAR(j.v, 3); // trem de pouso recolhido
    VEHICLE::SET_VEHICLE_FORWARD_SPEED(j.v, 70.f);
    j.pilot = NewPedIn(j.v, RandomPed(), -1);
    if (!j.pilot) { VEHICLE::DELETE_VEHICLE(&j.v); return; }
    SetupPed(j.pilot, s_level, WeaponFor(s_level));
    PED::SET_PED_ACCURACY(j.pilot, g_cfg.explosiveAcc); // misseis e canhao do jato imprecisos
    PED::SET_PED_COMBAT_ATTRIBUTES(j.pilot, 3, FALSE);
    AddCrew(j.pilot);
    AI::TASK_PLANE_MISSION(j.pilot, j.v, 0, s_player, 0.f, 0.f, 0.f, 6, 80.f, 0.f, -1.f, 2600.f, 40.f); // atacar
    j.born = g_st.now;
    j.nextSwap = g_st.now + 5000;
    s_jet = j;
    s_v.push_back({j.v, g_st.now, false});
    Log("  JATO (%s) chegando a %.0fm", m == H("hydra") ? "Hydra" : "Lazer", D2(p, s_ppos));
}

static void UpdateJet() {
    JetSt &j = s_jet;
    if (!j.v) return;
    if (!ENTITY::DOES_ENTITY_EXIST(j.v) || ENTITY::IS_ENTITY_DEAD(j.v) || !ENTITY::DOES_ENTITY_EXIST(j.pilot) ||
        PED::IS_PED_DEAD_OR_DYING(j.pilot, TRUE)) {
        Log("Jato abatido ou fora de combate.");
        j = JetSt{};
        return;
    }
    if (!j.leaving && g_st.now >= j.nextSwap) { // usa todas as armas: alterna canhao e misseis
        static const char *kWeaps[] = {"VEHICLE_WEAPON_PLAYER_LAZER", "VEHICLE_WEAPON_PLANE_ROCKET",
                                       "VEHICLE_WEAPON_SPACE_ROCKET"};
        j.wi = (j.wi + 1) % 3;
        WEAPON::SET_CURRENT_PED_VEHICLE_WEAPON(j.pilot, H(kWeaps[j.wi]));
        j.nextSwap = g_st.now + 5000;
    }
    if (!j.leaving && g_st.now - j.born > g_cfg.jetDuration * 1000) {
        Vector3 jp = ENTITY::GET_ENTITY_COORDS(j.v, TRUE);
        float ax = jp.x - s_ppos.x, ay = jp.y - s_ppos.y, al = std::hypot(ax, ay) + 0.01f;
        AI::TASK_PLANE_MISSION(j.pilot, j.v, 0, 0, jp.x + ax / al * 3000.f, jp.y + ay / al * 3000.f, jp.z + 300.f, 4,
                               80.f, 0.f, -1.f, 2600.f, 200.f);
        j.leaving = true;
        Log("  jato indo embora.");
    }
}
// ---------- waves ----------
static void Queue(int delayMs, PKind k, int n = 0, int role = 0, bool bicycleOk = false) {
    s_q.push_back({g_st.now + delayMs, k, n, role, bicycleOk});
}

static void BuildWave() {
    const Config::Lv &L = g_cfg.lv[s_level - 1];
    int free = Budget() - Queued();
    if (free <= 0) { Log("Wave adiada: limite de %d retaliadores.", MaxAlive()); return; }
    bool inVeh = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE);
    bool air = PED::IS_PED_IN_FLYING_VEHICLE(s_player);
    bool boat = PED::IS_PED_IN_ANY_BOAT(s_player);
    Log("Wave nivel %d (%s, %s) - jogador %s, %d vivos.", s_level, s_fac->name,
        RegionAt(s_ppos.x, s_ppos.y) == RG_CITY ? "cidade" : "interior",
        air ? "voando" : boat ? "de barco" : inVeh ? "de veiculo" : "a pe", HuntersAlive());

    float bonus = TopBonus();
    int heliCap = HeliLimit(air) - (int)s_helis.size() - HelisQueued();

    // v6: jato (so no ultimo nivel, um por vez; mais chance se o jogador estiver voando)
    if (g_cfg.jetEnabled && s_level >= g_cfg.jetMinLevel && !JetActive() && free >= 1 &&
        Rand(100) < Pct(g_cfg.jetChance, (bonus + EscBonus() * 0.5f) * 0.3f, g_cfg.jetMax) + (air ? 5 : 0)) {
        Queue(2000, P_JET);
        free -= 1;
    }

    // v5: jogador voando -> foco em helicopteros, e so alguns veiculos seguindo por terra
    if (air) {
        if (s_level >= 3 && heliCap > 0 && free >= 4) {
            int n = HelisNow();
            // voando: o 1o vem sempre, o 2o e comum, o 3o continua raro
            int pct = n == 0 ? 100 : n == 1 ? 60 : Pct(g_cfg.heliThirdPct + 5, 0.f, g_cfg.heliThirdMax + 5);
            if (Rand(100) < pct) { Queue(1000, P_HELI, 0, 1); free -= 4; }
        } else if (s_level < 3) {
            Log("  jogador voando: helicopteros a partir do nivel 3.");
        }
        if (free >= 2) Queue(3000, P_VEH);
        return;
    }

    // helicoptero na wave (no chao ou de barco)
    bool hasHeli = !s_fac->helis.empty() || !s_fac->attackHelis.empty();
    // cada helicoptero a mais no ar tem a sua chance: 1o normal, 2o baixa, 3o bem rara (so no nivel 8)
    if (hasHeli && heliCap > 0 && free >= 4) {
        int n = HelisNow();
        if (Rand(100) < NextHeliPct(n)) {
            Queue(4000, P_HELI);
            free -= 4;
            if (n == 0 && heliCap > 1 && free >= 4 && Rand(100) < NextHeliPct(1)) { Queue(12000, P_HELI); free -= 4; }
        }
    }

    // v6: tanque no Cargobob (so no ultimo nivel, um por vez, com o jogador no chao)
    if (!boat && g_cfg.tankEnabled && s_level >= g_cfg.tankMinLevel && !TankActive() && free >= 2 &&
        Rand(100) < Pct(g_cfg.tankChance, (bonus + EscBonus() * 0.5f) * 0.4f, g_cfg.tankMax)) {
        Queue(3000, P_TANK);
        free -= 2;
    }

    if (boat) {
        if (s_level >= 4 && !s_fac->boats.empty()) { Queue(1000, P_BOAT); if (L.vehicles >= 2) Queue(6000, P_BOAT); }
        else Log("  jogador de barco: esta faccao nao tem barcos neste nivel.");
        return;
    }

    if (g_cfg.jugEnabled && (s_level >= g_cfg.jugMinLevel || g_st.Pts() >= g_cfg.escStart) && !JuggAlive() &&
        free >= 4 && Rand(100) < Pct(g_cfg.jugChance, (bonus + EscBonus()) * 0.4f, g_cfg.jugMax)) {
        Queue(2500, P_JUGG);
        free -= 4;
    }

    if (inVeh) {
        if (g_cfg.rbEnabled && s_level >= g_cfg.rbMinLevel && free >= 4 && Rand(100) < g_cfg.rbChance + bonus) {
            Queue(0, P_ROADBLOCK);
            free -= 4;
        }
        for (int i = 0; i < L.vehicles + 1 && free > 0; ++i, free -= 2) Queue(1500 + i * 4000, P_VEH);
        return;
    }

    // a pe: duplas chegando aos poucos, depois os veiculos de longe
    int foot = L.foot < free ? L.foot : free;
    // snipers e explosivos (RPG, lanca-granadas) sao raros: uma chance por wave
    bool sniper = s_level >= 6 && SniperFor(s_level) && Rand(100) < g_cfg.sniperPct;
    bool heavy = HeavyFor(s_level) != 0 && Rand(100) < Pct(g_cfg.heavyPct, EscBonus() * 0.2f, g_cfg.heavyMax); // nivel 7+
    for (int i = 0, k = 0; i < foot; i += 2, ++k) {
        int n = foot - i >= 2 ? 2 : 1;
        int role = k == 0 && sniper ? 1 : k == 1 && heavy ? 2 : 0;
        Queue(k * 2500, P_FOOT, n, role);
    }
    free -= foot;
    for (int i = 0; i < L.vehicles && free > 0; ++i, free -= 2) Queue(6000 + i * 4000, P_VEH, 0, 0, true);
}

static void RunQueue() {
    for (size_t i = 0; i < s_q.size(); ++i) {
        if (s_q[i].at > g_st.now) continue;
        Pending p = s_q[i];
        s_q.erase(s_q.begin() + i);
        if (Budget() <= 0) return;
        switch (p.k) {
        case P_FOOT: SpawnFoot(p.n, p.role); break;
        case P_VEH: SpawnVehicle(p.bicycleOk); break;
        case P_BOAT: SpawnBoat(); break;
        case P_JUGG: SpawnJuggernaut(); break;
        case P_HELI: SpawnHeli(p.role == 1); break;
        case P_TANK: if (s_tank.state < 0) SpawnTank(); break;
        case P_JET: if (!s_jet.v) SpawnJet(); break;
        case P_ROADBLOCK:
            if (!SpawnRoadblock()) { Log("  bloqueio: nenhuma rua a frente; vem um carro no lugar."); SpawnVehicle(false); }
            break;
        }
        return; // uma unidade por frame
    }
}

// ---------- sinais de mao ----------
// Gestos de combate do proprio jogo (os mesmos da NOOSE e das gangues), tocados so na parte de cima do corpo,
// sem interromper o combate. So os dicionarios que existirem no jogo sao usados.
static std::vector<const char *> s_gestRifle, s_gestGang;

static void GesturesInit() {
    static const char *rifle[] = {"combat@gestures@rifle@beckon", "combat@gestures@rifle@halt",
                                  "combat@gestures@rifle@over_there", "combat@gestures@rifle@pile_up",
                                  "combat@gestures@rifle@come_here"};
    static const char *gang[] = {"combat@gestures@gang@pistol_1h@beckon", "combat@gestures@gang@pistol_1h@halt",
                                 "combat@gestures@gang@pistol_1h@over_there", "combat@gestures@gang@pistol_1h@come_here",
                                 "combat@gestures@gang@pistol_1h@hands_up"};
    s_gestRifle.clear(); s_gestGang.clear();
    for (const char *d : rifle) if (STREAMING::DOES_ANIM_DICT_EXIST((char *)d)) s_gestRifle.push_back(d);
    for (const char *d : gang) if (STREAMING::DOES_ANIM_DICT_EXIST((char *)d)) s_gestGang.push_back(d);
    Log("Sinais de mao: %d de rifle, %d de gangue.", (int)s_gestRifle.size(), (int)s_gestGang.size());
}

static void TryGesture(Ped ped, const Vector3 &hp) {
    // so faz sentido com um companheiro por perto para "receber" o sinal
    bool mate = false;
    for (Hunter &o : s_h)
        if (o.ped != ped && D2(ENTITY::GET_ENTITY_COORDS(o.ped, TRUE), hp) < 30.f) { mate = true; break; }
    if (!mate) return;
    const auto &pool = !s_gestRifle.empty() ? s_gestRifle : s_gestGang;
    if (pool.empty()) return;
    const char *dict = pool[Rand((int)pool.size())];
    STREAMING::REQUEST_ANIM_DICT((char *)dict);
    if (!STREAMING::HAS_ANIM_DICT_LOADED((char *)dict)) return; // carrega e tenta no proximo ciclo
    AI::TASK_PLAY_ANIM(ped, (char *)dict, (char *)"0", 4.f, -4.f, 1800, 48, 0.f, FALSE, FALSE, FALSE);
}

// ---------- atualizacao e limpeza ----------
static void RemoveBlip(Hunter &h) {
    if (h.blip) { UI::REMOVE_BLIP(&h.blip); h.blip = 0; }
}

static void ReleasePed(Hunter &h, bool del) {
    RemoveBlip(h);
    PointsIgnorePed(h.ped);
    if (del) PED::DELETE_PED(&h.ped);
    else ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&h.ped);
}

static void ReleaseVeh(SquadVeh &s, bool del) {
    PointsIgnoreVeh(s.veh);
    if (del) VEHICLE::DELETE_VEHICLE(&s.veh);
    else ENTITY::SET_VEHICLE_AS_NO_LONGER_NEEDED(&s.veh);
}

void DespawnAll(bool hard, const char *why) {
    int n = (int)s_h.size();
    for (Hunter &h : s_h) {
        bool del = hard || (ENTITY::DOES_ENTITY_EXIST(h.ped) && !ENTITY::IS_ENTITY_ON_SCREEN(h.ped));
        if (ENTITY::DOES_ENTITY_EXIST(h.ped)) ReleasePed(h, del);
    }
    for (SquadVeh &s : s_v) {
        if (!ENTITY::DOES_ENTITY_EXIST(s.veh)) continue;
        ReleaseVeh(s, hard || !ENTITY::IS_ENTITY_ON_SCREEN(s.veh));
    }
    s_h.clear(); s_v.clear(); s_q.clear(); s_helis.clear();
    s_tank = TankSt{};
    s_jet = JetSt{};
    s_nextWave = 0;
    if (n) Log("Retaliadores removidos (%s): %d.", why, n);
}

static void UpdateHunters() {
    bool onFoot = !PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE);
    float br = BlipRadius();
    for (size_t i = 0; i < s_h.size();) {
        Hunter &h = s_h[i];
        if (!ENTITY::DOES_ENTITY_EXIST(h.ped)) { RemoveBlip(h); s_h.erase(s_h.begin() + i); continue; }
        if (PED::IS_PED_DEAD_OR_DYING(h.ped, TRUE)) {
            if (h.juggernaut) Log("Juggernaut abatido!");
            // v7: retaliador morto pelo jogador da pontos
            Entity k = PED::_GET_PED_KILLER(h.ped);
            Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            if (k && (k == s_player || (pv && k == pv))) AddPoints(g_cfg.killHunter, "retaliador morto");
            ReleasePed(h, false);
            s_h.erase(s_h.begin() + i);
            continue;
        }
        Vector3 hp = ENTITY::GET_ENTITY_COORDS(h.ped, TRUE);
        float d = D2(hp, s_ppos);

        // blip vermelho so perto do jogador
        if (!h.blip && d < br) {
            h.blip = UI::ADD_BLIP_FOR_ENTITY(h.ped);
            UI::SET_BLIP_COLOUR(h.blip, 1);
            UI::SET_BLIP_SCALE(h.blip, h.juggernaut ? 1.1f : 0.7f);
        } else if (h.blip && d > br + 15.f) {
            RemoveBlip(h);
        }

        if (d < 70.f) h.reached = true;

        // v5: so atira dentro do alcance da arma
        if (!h.rangeExempt) {
            float r = RangeMeters(RangeOf(WEAPON::GET_SELECTED_PED_WEAPON(h.ped)));
            int want = (r <= 0.f || d <= r) ? 100 : 0;
            if (want != h.shootRate) { PED::SET_PED_SHOOT_RATE(h.ped, want); h.shootRate = want; }
        }
        bool timeout = !h.reached && !h.roadblock && g_st.now - h.born > g_cfg.arriveTimeout * 1000;
        bool rbOld = h.roadblock && g_st.now - h.born > (g_cfg.arriveTimeout + 60) * 1000 && d > 150.f;
        if (d > (h.farOk ? 1500.f : g_cfg.despawnDist) || timeout || rbOld) {
            ReleasePed(h, !ENTITY::IS_ENTITY_ON_SCREEN(h.ped) || d > 200.f);
            s_h.erase(s_h.begin() + i);
            continue;
        }

        // descer do veiculo perto do jogador a pe
        bool inVeh = PED::IS_PED_IN_ANY_VEHICLE(h.ped, FALSE);
        if (inVeh && !h.stayInVeh && !h.leaving && onFoot && d < 45.f) {
            AI::TASK_LEAVE_VEHICLE(h.ped, PED::GET_VEHICLE_PED_IS_IN(h.ped, FALSE), 0);
            h.leaving = true;
            h.leftAt = g_st.now;
        } else if (h.leaving && (!inVeh || g_st.now - h.leftAt > 4000)) {
            h.leaving = false;
            if (h.juggernaut) ApplyJuggernautWalk(h.ped);
            AI::TASK_COMBAT_PED(h.ped, s_player, 0, 16);
        }

        // posicionamento tatico: area defensiva em volta do jogador, a media distancia, com cobertura
        if (h.tactical && !inVeh && !h.roadblock && !h.sniper && !h.juggernaut && g_st.now >= h.nextArea) {
            h.nextArea = g_st.now + 4000;
            PED::SET_PED_SPHERE_DEFENSIVE_AREA(h.ped, s_ppos.x, s_ppos.y, s_ppos.z, h.areaRadius, FALSE, FALSE);
        }
        if (h.tactical && !inVeh && !h.juggernaut && g_st.now >= h.nextGesture) {
            h.nextGesture = g_st.now + 9000 + Rand(10000);
            if (d > 12.f && d < 80.f && !PED::IS_PED_SHOOTING(h.ped)) TryGesture(h.ped, hp);
        }
        ++i;
    }

    for (size_t i = 0; i < s_v.size();) {
        SquadVeh &s = s_v[i];
        if (!ENTITY::DOES_ENTITY_EXIST(s.veh)) { s_v.erase(s_v.begin() + i); continue; }
        // v7: veiculo/aeronave dos retaliadores destruido pelo jogador tambem da pontos
        if (!s.scored && ENTITY::IS_ENTITY_DEAD(s.veh)) {
            s.scored = true;
            Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            if (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, s_player, TRUE) ||
                (pv && pv != s.veh && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, pv, TRUE))) {
                Hash m = ENTITY::GET_ENTITY_MODEL(s.veh);
                bool air = VEHICLE::IS_THIS_MODEL_A_HELI(m) || VEHICLE::IS_THIS_MODEL_A_PLANE(m);
                AddPoints(air ? g_cfg.blowAircraft : g_cfg.blowVehicle, air ? "aeronave dos retaliadores destruida"
                                                                             : "veiculo dos retaliadores destruido");
            }
        }
        float d = D2(ENTITY::GET_ENTITY_COORDS(s.veh, TRUE), s_ppos);
        bool used = false;
        if (!s.roadblock)
            for (Hunter &h : s_h)
                if (PED::IS_PED_IN_VEHICLE(h.ped, s.veh, FALSE)) { used = true; break; }
        bool drop = s.roadblock ? (d > g_cfg.despawnDist || g_st.now - s.born > (g_cfg.arriveTimeout + 60) * 1000)
                                : (!used && (d > 60.f || ENTITY::IS_ENTITY_DEAD(s.veh)));
        if (s.roadblock && ENTITY::IS_ENTITY_DEAD(s.veh)) drop = true;
        if (drop) {
            ReleaseVeh(s, d > 120.f && !ENTITY::IS_ENTITY_ON_SCREEN(s.veh));
            s_v.erase(s_v.begin() + i);
            continue;
        }
        ++i;
    }
}

void SpawnerInit() {
    s_rng ^= GetTickCount() * 2654435761u;
    s_h.clear(); s_v.clear(); s_q.clear();
    s_nextWave = 0;
    s_helis.clear();
    FactionsInit();
    GesturesInit();
    s_genericHelis.clear();
    for (const char *m : {"maverick", "frogger"})
        if (STREAMING::IS_MODEL_IN_CDIMAGE(H(m))) s_genericHelis.push_back(H(m));
}

void SpawnerTick(Ped player) {
    s_player = player;
    s_ppos = ENTITY::GET_ENTITY_COORDS(player, TRUE);
    UpdatePlayerGround();
    Player pid = PLAYER::PLAYER_ID();

    bool dead = PLAYER::IS_PLAYER_DEAD(pid) || PLAYER::IS_PLAYER_BEING_ARRESTED(pid, TRUE);
    if (dead && !s_wasDead) DespawnAll(false, "morte/prisao");
    s_wasDead = dead;
    bool mission = g_st.paused && strcmp(g_st.pauseReason, "interior") != 0;
    if (mission && !s_wasMission) DespawnAll(false, g_st.pauseReason);
    s_wasMission = mission;

    if (g_st.now - s_lastUpdate >= 200) {
        s_lastUpdate = g_st.now;
        UpdateHunters();
        UpdateHelis();
        UpdateTank();
        UpdateJet();
    }
    if (dead) return;

    s_level = LevelOf(g_st.Pts());
    bool active = s_level >= 1 && !g_st.paused && g_st.now >= g_st.cooldownUntil;
    if (!active) {
        s_q.clear();
        if (s_level < 1) s_nextWave = 0;
        return;
    }
    s_fac = FactionFor(s_level, RegionAt(s_ppos.x, s_ppos.y));
    if (!s_fac || s_fac->peds.empty()) return;

    if (!s_nextWave) s_nextWave = g_st.now + g_cfg.firstWaveDelay * 1000;
    if (g_st.now >= s_nextWave) {
        s_nextWave = g_st.now + g_cfg.lv[s_level - 1].interval * 1000;
        BuildWave();
    }
    RunQueue();
}
