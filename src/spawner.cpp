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
    int nextArea = 0, nextGesture = 0;
    float areaRadius = 30.f;
};
struct SquadVeh { Vehicle veh; int born; bool roadblock; };
enum PKind { P_FOOT, P_VEH, P_BOAT, P_JUGG, P_ROADBLOCK };
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
    for (auto &p : s_q) n += p.k == P_FOOT ? p.n : p.k == P_ROADBLOCK ? 4 : p.k == P_JUGG ? 4 : 2;
    return n;
}
static int Budget() { return g_cfg.maxAlive - HuntersAlive(); }
static bool JuggAlive() {
    for (auto &h : s_h) if (h.juggernaut) return true;
    for (auto &p : s_q) if (p.k == P_JUGG) return true;
    return false;
}
static float TopBonus() {
    if (g_cfg.levelStart.empty() || LevelOf(g_st.Pts()) < (int)g_cfg.levelStart.size()) return 0.f;
    return (g_st.Pts() - g_cfg.levelStart.back()) * g_cfg.topBonusPerPoint;
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

static Vehicle NewVehicle(Hash model, const Vector3 &p, float heading) {
    if (!LoadModel(model)) return 0;
    Vehicle v = VEHICLE::CREATE_VEHICLE(model, p.x, p.y, p.z, heading, FALSE, TRUE);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    if (!v) return 0;
    VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(v);
    return v;
}

// ---------- posicoes ----------
// Ponto a pe a media distancia, de preferencia atras da camera e fora da tela.
static bool FindFootPos(float dmin, float dmax, Vector3 &out) {
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z;
    for (int i = 0; i < 14; ++i) {
        float ang = i < 9 ? camH + 180.f + RandF(-80.f, 80.f) : RandF(0.f, 360.f);
        float d = RandF(dmin, dmax), dx, dy;
        Dir(ang, dx, dy);
        float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d, gz = 0.f;
        if (!GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, s_ppos.z + 60.f, &gz, 0)) continue;
        Vector3 safe;
        if (PATHFIND::GET_SAFE_COORD_FOR_PED(x, y, gz + 1.f, TRUE, &safe, 16)) { x = safe.x; y = safe.y; gz = safe.z; }
        if (std::fabs(gz - s_ppos.z) > 30.f) continue;
        Vector3 c = V3(x, y, gz + 1.f);
        if (D2(c, s_ppos) < dmin * 0.7f) continue;
        if (CAM::IS_SPHERE_VISIBLE(c.x, c.y, c.z, 1.5f)) continue;
        out = c;
        return true;
    }
    return false;
}

// Ponto numa rua, virado para o jogador.
static bool FindRoadPos(float dmin, float dmax, Vector3 &out, float &heading) {
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z;
    for (int i = 0; i < 12; ++i) {
        float ang = i < 6 ? camH + 180.f + RandF(-90.f, 90.f) : RandF(0.f, 360.f);
        float d = RandF(dmin, dmax), dx, dy;
        Dir(ang, dx, dy);
        Vector3 node;
        float h = 0.f;
        if (!PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(s_ppos.x + dx * d, s_ppos.y + dy * d, s_ppos.z, &node, &h,
                                                             1, 3.0f, 0))
            continue;
        float nd = D2(node, s_ppos);
        if (nd < dmin * 0.75f || nd > dmax * 1.3f) continue;
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
    s_v.push_back({v, g_st.now, false});
    Log("  %s com %d (%s) a %.0fm", kind, seats, s_fac->name, D2(p, s_ppos));
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
        Hash jm = H("u_m_y_juggernaut_01");
        bool real = STREAMING::IS_MODEL_IN_CDIMAGE(jm);
        if (!real) { Log("  modelo u_m_y_juggernaut_01 nao existe: usando um da faccao."); jm = RandomPed(); }
        Ped j = NewPedIn(v, jm, jugSeat);
        if (j) {
            if (real) PED::SET_PED_DEFAULT_COMPONENT_VARIATION(j);
            SetupPed(j, s_level, H("WEAPON_MINIGUN"));
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
            Log("  JUGGERNAUT a caminho (%s) a %.0fm", s_fac->name, D2(p, s_ppos));
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
        if (!PATHFIND::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(s_ppos.x + fx * d, s_ppos.y + fy * d, s_ppos.z, &n, &h, 1,
                                                             3.0f, 0))
            continue;
        float nd = D2(n, s_ppos);
        if (nd < 180.f || nd > 380.f) continue;
        float dot = ((n.x - s_ppos.x) * fx + (n.y - s_ppos.y) * fy) / nd;
        if (dot < 0.8f) continue;
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
            GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, vp.z + 5.f, &gz, 0);
            Vector3 q = V3(x, y, gz + 0.5f);
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

// ---------- waves ----------
static void Queue(int delayMs, PKind k, int n = 0, int role = 0, bool bicycleOk = false) {
    s_q.push_back({g_st.now + delayMs, k, n, role, bicycleOk});
}

static void BuildWave() {
    const Config::Lv &L = g_cfg.lv[s_level - 1];
    int free = Budget() - Queued();
    if (free <= 0) { Log("Wave adiada: limite de %d retaliadores.", g_cfg.maxAlive); return; }
    bool inVeh = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE);
    bool air = PED::IS_PED_IN_FLYING_VEHICLE(s_player);
    bool boat = PED::IS_PED_IN_ANY_BOAT(s_player);
    Log("Wave nivel %d (%s, %s) - jogador %s, %d vivos.", s_level, s_fac->name,
        RegionAt(s_ppos.x, s_ppos.y) == RG_CITY ? "cidade" : "interior",
        air ? "voando" : boat ? "de barco" : inVeh ? "de veiculo" : "a pe", HuntersAlive());

    if (air) { Log("  jogador voando: helicopteros e caca entram na proxima versao."); return; }
    if (boat) {
        if (s_level >= 4 && !s_fac->boats.empty()) { Queue(1000, P_BOAT); if (L.vehicles >= 2) Queue(6000, P_BOAT); }
        else Log("  jogador de barco: esta faccao nao tem barcos neste nivel.");
        return;
    }

    float bonus = TopBonus();
    if (g_cfg.jugEnabled && s_level >= g_cfg.jugMinLevel && !JuggAlive() && free >= 4 &&
        Rand(100) < g_cfg.jugChance + bonus) {
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
    bool sniper = s_level >= 6 && SniperFor(s_level);
    bool heavy = HeavyFor(s_level) != 0; // nivel 7+
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
    s_h.clear(); s_v.clear(); s_q.clear();
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
        bool timeout = !h.reached && !h.roadblock && g_st.now - h.born > g_cfg.arriveTimeout * 1000;
        bool rbOld = h.roadblock && g_st.now - h.born > (g_cfg.arriveTimeout + 60) * 1000 && d > 150.f;
        if (d > g_cfg.despawnDist || timeout || rbOld) {
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
    FactionsInit();
    GesturesInit();
}

void SpawnerTick(Ped player) {
    s_player = player;
    s_ppos = ENTITY::GET_ENTITY_COORDS(player, TRUE);
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
