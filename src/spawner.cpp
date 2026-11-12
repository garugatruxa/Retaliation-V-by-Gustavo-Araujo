// Waves de retaliadores: grupos a pe, veiculos, barcos, Juggernaut no Insurgent e bloqueios de estrada.
#include "spawn.h"
#include <cmath>
#include <algorithm>

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
    bool walking = false; // v9: veio a pe da estrada mais proxima, andando ate o jogador
    bool reached = false, stayInVeh = false, leaving = false, juggernaut = false, roadblock = false, sniper = false;
    bool tactical = false; // IA estilo NOOSE (so Merryweather e Cacadores)
    bool rangeExempt = false; // pilotos e torretas: armas do veiculo, sem limite de alcance
    bool heliRappel = false;  // vai descer de corda
    int shootRate = -1;
    bool farOk = false; // jato e Cargobob voam longe: so sao removidos a 1500 m
    bool tankGunner = false; // v10: o canhao do tanque e controlado em UpdateTank
    bool ragOk = false;      // v10.1: Juggernaut ja pode cair (ragdoll ligado perto da morte)
    bool lightJug = false;   // v10.1: Juggernaut leve da faccao
    int nextArea = 0, nextGesture = 0;
    bool para = false; // v10: descendo de paraquedas (evento do Titan)
    int nextLos = 0, losLostAt = 0; bool seeking = false; // v10: sem visao do jogador: vai atras dele
    int nextApproach = 0; bool approaching = false; // v10: fora do alcance, corre ate o jogador
    int nextPush = 0; bool pushing = false, wild = false; // v10: fora da estrada: avanca ate o jogador
    float areaRadius = 30.f;
    int lastHp = -1;          // v11: vida no ultimo exame (para saber se o jogador atacou)
    int onPlayer = -1;        // v11: na Desordem, 1 = atacando o jogador, 0 = outros alvos
    int nextChase = 0;        // v11: motorista na Desordem renova o destino
};
struct SquadVeh { Vehicle veh; int born; bool roadblock; bool scored = false; Blip blip = 0; int lastHp = -1; };
enum PKind { P_FOOT, P_VEH, P_BOAT, P_JUGG, P_ROADBLOCK, P_HELI, P_TANK, P_JET, P_KHAN, P_AIR, P_PARA };
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
static const float kBlipRadius = 180.f; // padrao; o .ini pode mudar
static float BlipRadius() { return g_cfg.blipRadius > 0 ? g_cfg.blipRadius : kBlipRadius; }
static bool s_wild = false; // v10: jogador a pe fora das estradas (ver UpdateWild)
// ---------- v11: alvo na Desordem ----------
// Fora da Desordem (ou com os retaliadores contra o jogador) todos atacam o jogador, como sempre. Na Desordem
// calma, quem esta longe do jogador luta contra os inimigos em volta (policia e civis); de perto, ataca ele.
static bool Calm() { return DisorderCalm(); }
static bool NearPlayer(Ped p) { return D2(ENTITY::GET_ENTITY_COORDS(p, TRUE), s_ppos) < g_cfg.disNearDist; }
static void Engage(Ped p) {
    if (Calm() && !NearPlayer(p)) AI::TASK_COMBAT_HATED_TARGETS_AROUND_PED(p, 300.f, 0);
    else AI::TASK_COMBAT_PED(p, s_player, 0, 16);
}
// motorista: persegue o jogador; na Desordem calma, so dirige ate a area dele (e desce para lutar)
static void Chase(Ped p) {
    Vehicle v = PED::GET_VEHICLE_PED_IS_IN(p, FALSE);
    if (Calm() && v && !NearPlayer(p))
        AI::TASK_VEHICLE_DRIVE_TO_COORD_LONGRANGE(p, v, s_ppos.x, s_ppos.y, s_ppos.z, 30.f, 786469, 35.f);
    else
        AI::TASK_VEHICLE_CHASE(p, s_player);
}
// v10: jogador a pe e no chao (nem em veiculo, nem nadando, caindo ou de paraquedas)
static bool PlayerOnFootGround() {
    return !PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) && !PED::IS_PED_SWIMMING(s_player) &&
           !PED::IS_PED_FALLING(s_player) && !PED::IS_PED_IN_PARACHUTE_FREE_FALL(s_player) &&
           PED::GET_PED_PARACHUTE_STATE(s_player) <= 0 && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(s_player) < 3.f;
}
// v10: icone do minimapa por tipo de veiculo (os inimigos a pe ficam com a bolinha vermelha padrao)
static int VehBlipSprite(Vehicle v) {
    Hash m = ENTITY::GET_ENTITY_MODEL(v);
    static const Hash rhino = GAMEPLAY::GET_HASH_KEY((char *)"rhino"), khan = GAMEPLAY::GET_HASH_KEY((char *)"khanjali");
    if (m == rhino || m == khan) return 421;             // tanque
    if (VEHICLE::IS_THIS_MODEL_A_HELI(m)) return 353;    // helicoptero com a helice girando
    if (VEHICLE::IS_THIS_MODEL_A_PLANE(m)) return 424;   // jato
    if (VEHICLE::IS_THIS_MODEL_A_BOAT(m)) return 427;    // barco
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(m) || VEHICLE::IS_THIS_MODEL_A_BICYCLE(m)) return 348; // moto/bicicleta
    return 225;                                          // carro
}

int HuntersAlive() { return (int)s_h.size(); }
static int Queued() {
    int n = 0;
    for (auto &p : s_q) n += p.k == P_FOOT ? p.n : (p.k == P_ROADBLOCK || p.k == P_JUGG || p.k == P_HELI) ? 4 : 2;
    return n;
}
// ---------- fluxo (v9) ----------
// Os inimigos chegam aos poucos: uma fila com espacamento entre unidades, um limite que cresce com o tempo
// desde que o Retaliation comecou e um teto de quantos podem estar perto do jogador ao mesmo tempo.
static int s_flowNext = 0;    // quando a proxima unidade da fila pode sair
static int s_activeSince = 0; // quando o Retaliation comecou (para a pressao crescente)

static int MaxAlive() {
    int base = s_level >= 1 && s_level <= 8 ? g_cfg.lv[s_level - 1].maxAlive : 10;
    if (base > kHardCap) base = kHardCap; // v10: teto absoluto
    // v10.1: com a policia tambem atras do jogador, menos retaliadores (sobra espaco para policia e civis)
    if (PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID()) > 0 && g_cfg.popWithPolicePct < 100) {
        int b = base * g_cfg.popWithPolicePct / 100;
        base = b < 4 ? 4 : b;
    }
    if (g_cfg.flowRampSeconds <= 0 || !s_activeSince) return base;
    float k = (g_st.now - s_activeSince) / (g_cfg.flowRampSeconds * 1000.f);
    if (k >= 1.f) return base;
    int start = g_cfg.flowRampStart < base ? g_cfg.flowRampStart : base;
    return start + (int)((base - start) * k);
}
static int FlowGapMs() {
    float lv = (float)(s_level < 1 ? 1 : s_level > 8 ? 8 : s_level);
    float gmin = 7.f - (lv - 1.f) * 0.5f, gmax = 12.f - (lv - 1.f) * 0.714f; // 7-12 s no nivel 1, 3.5-7 s no 8
    return (int)(RandF(gmin, gmax) * 1000.f * g_cfg.flowGapScale);
}
static int NearMax() { return g_cfg.flowNearBase + s_level; }
static int NearCount() {
    int n = 0;
    for (const auto &h : s_h)
        if (ENTITY::DOES_ENTITY_EXIST(h.ped) && D2(ENTITY::GET_ENTITY_COORDS(h.ped, TRUE), s_ppos) < 60.f) ++n;
    return n;
}
static int Budget() { return MaxAlive() - HuntersAlive(); }
static bool JuggAlive() {
    for (auto &h : s_h) if (h.juggernaut) return true;
    for (auto &p : s_q) if (p.k == P_JUGG) return true;
    return false;
}
// v5: a partir de Escalation.Start (350 na escala de 500) cada ponto soma PerPoint % aos especiais
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
enum RangeClass { RC_SHORT, RC_MEDIUM, RC_LONG, RC_RPG };
static RangeClass RangeOf(Hash w) {
    static const Hash rpg = H("WEAPON_RPG"), homing = H("WEAPON_HOMINGLAUNCHER"), gl = H("WEAPON_GRENADELAUNCHER"),
                      cgl = H("WEAPON_COMPACTLAUNCHER");
    if (w == rpg || w == homing || w == gl || w == cgl) return RC_RPG; // v10: explosivos tem alcance proprio
    switch ((Hash)WEAPON::GET_WEAPONTYPE_GROUP(w)) {
    case 0xB7BBD827: return RC_LONG;   // snipers
    case 0x39D5C192:                   // fuzis/carabinas
    case 0x451B04BC:                   // metralhadoras
    case 0xA27A4F9F: return RC_MEDIUM; // pesadas (minigun, lanca-granadas)
    default: return RC_SHORT;          // pistolas, SMGs, escopetas, corpo a corpo
    }
}
static float RangeMeters(RangeClass c) {
    return c == RC_SHORT ? g_cfg.rangeShort : c == RC_MEDIUM ? g_cfg.rangeMedium : c == RC_RPG ? g_cfg.rangeRpg
                                                                                               : g_cfg.rangeSniper;
}

// ---------- criacao de NPCs ----------
static bool IsPro() {
    return s_fac && (!strcmp(s_fac->id, "CAYO") || !strcmp(s_fac->id, "CACADORES") || !strcmp(s_fac->id, "BOGDAN") ||
                     !strcmp(s_fac->id, "MERRYWEATHER"));
}
// IA tatica (estilo NOOSE) so para a elite: Cacadores, mercenarios do Bogdan e Merryweather.
static bool IsTactical() {
    return s_fac && (!strcmp(s_fac->id, "CACADORES") || !strcmp(s_fac->id, "BOGDAN") || !strcmp(s_fac->id, "MERRYWEATHER"));
}
// Dos 40 pontos para cima, mais colete, capacete e armas melhores (v3).
static int GearPct(int base, float perPoint, int cap) {
    int pts = g_st.Pts();
    if (pts < g_cfg.gearStart) return 0;
    int v = base + (int)((pts - g_cfg.gearStart) * perPoint / PtsScale()); // v10: taxa na escala de 500
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
    if (weapon) { WEAPON::GIVE_WEAPON_TO_PED(ped, weapon, 9999, FALSE, TRUE); ApplyWeaponMods(ped, weapon, level); }
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
    // v10: arrastar companheiro ferido para a cobertura (a elite sempre; os outros com DragChance %)
    if (!IsTactical() && Rand(100) < g_cfg.dragPct) PED::SET_PED_COMBAT_ATTRIBUTES(ped, 22, TRUE);
    PED::SET_PED_FLEE_ATTRIBUTES(ped, 0, FALSE);
    PED::SET_PED_SEEING_RANGE(ped, 250.f);
    PED::SET_PED_KEEP_TASK(ped, TRUE);
    if (weapon) {
        RangeClass rc = RangeOf(weapon);
        PED::SET_PED_COMBAT_RANGE(ped, rc == RC_SHORT ? 0 : rc == RC_LONG ? 2 : 1); // chega perto conforme a arma
    }
    ApplyGear(ped, level);
}
// v10: armas que matam rapido (explosivos, canhoes, misseis, minigun) erram muito: mira baixa e IA fraca
static void PoorAim(Ped ped, int acc) {
    PED::SET_PED_ACCURACY(ped, acc);
    PED::SET_PED_COMBAT_ABILITY(ped, 0);
}
// v10: quem desce do helicoptero volta a mira normal do nivel
static void NormalAim(Ped ped) {
    int lv = s_level >= 1 && s_level <= 8 ? s_level : 1;
    PED::SET_PED_ACCURACY(ped, g_cfg.lv[lv - 1].accuracy);
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
    // no mato e nas montanhas o chao e irregular: degrau maior; na cidade, pequeno (viadutos)
    const float maxStep = RegionAt(s_ppos.x, s_ppos.y) == RG_COUNTRY ? 9.f : 4.5f;
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
    for (int i = 0; i < 30; ++i) {
        float ang = i < 15 ? camH + 180.f + RandF(-80.f, 80.f) : RandF(0.f, 360.f);
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
// v10: velocidade (m/s) e direcao de viagem do jogador; heading = para onde o carro aponta
static float PlayerTravel(float &fx, float &fy, float &heading) {
    Vehicle pv = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE) : 0;
    Entity e = pv ? pv : s_player;
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(e);
    float sp = std::hypot(vel.x, vel.y);
    heading = ENTITY::GET_ENTITY_HEADING(e);
    if (sp > 4.f) { fx = vel.x / sp; fy = vel.y / sp; }
    else Dir(heading, fx, fy);
    return sp;
}
static const float kFastSpeed = 18.f; // ~65 km/h: perseguicao de estrada
static bool PlayerFastInCar() {
    float fx, fy, h;
    return PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) && !PED::IS_PED_IN_FLYING_VEHICLE(s_player) &&
           !PED::IS_PED_IN_ANY_BOAT(s_player) && PlayerTravel(fx, fy, h) > kFastSpeed;
}
// v10: no de estrada perto de (x, y) cuja mao segue o sentido de viagem (fx, fy). Evita a pista contraria
// das rodovias de pista dupla: pede ao jogo o no que "aponta" para um ponto mais a frente e confere a direcao.
static bool RoadNodeInTravelDir(float x, float y, float fx, float fy, Vector3 &out, float &h) {
    float travelH = std::atan2(-fx, fy) * 57.29578f; // mesmo padrao de heading do jogo
    if (travelH < 0) travelH += 360.f;
    for (int nth = 0; nth < 4; ++nth) {
        Vector3 n; float nh = 0.f;
        BOOL ok = invoke<BOOL>(0x45905BE8654AE067, x, y, s_pgz, x + fx * 60.f, y + fy * 60.f, s_pgz, nth, &n, &nh, 1,
                               3.0f, 0.f); // GET_NTH_CLOSEST_VEHICLE_NODE_FAVOUR_DIRECTION
        if (!ok) continue;
        if (AngDiff(nh, travelH) > 55.f) continue; // mao contraria
        out = n; h = nh;
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
    else if (Rand(100) < g_cfg.flowFarFootPct) { dmin = dmax; dmax += 70.f; } // v9: alguns grupos vem de mais longe
    Vector3 p;
    bool fromRoad = false;
    if (s_wild && role != 1) {
        // v10: no mato/montanha ninguem nasce em volta do jogador: o grupo nasce numa estrada, o mais longe
        // possivel, e vem correndo. Sem estrada por perto, nasce no chao na distancia maxima, fora da vista.
        float rh;
        fromRoad = FindRoadPos(420.f, 650.f, p, rh) || FindRoadPos(280.f, 450.f, p, rh) || FindRoadPos(180.f, 300.f, p, rh);
        if (!fromRoad && !FindFootPos(g_cfg.footMax + 60.f, g_cfg.footMax + 140.f, p)) {
            Log("  a pe: nenhuma estrada nem ponto seguro longe do jogador.");
            return;
        }
    } else if (!FindFootPos(dmin, dmax, p)) {
        // v9: longe de tudo (mato, montanha): o grupo nasce na rua mais proxima e vem correndo ate o jogador
        float rh;
        if (!FindRoadPos(120.f, 380.f, p, rh)) { Log("  a pe: nenhum ponto seguro nem rua por perto."); return; }
        fromRoad = true;
    }
    for (int i = 0; i < n && Budget() > 0; ++i) {
        Vector3 q = V3(p.x + RandF(-2.f, 2.f), p.y + RandF(-2.f, 2.f), p.z);
        Ped ped = NewPedAt(RandomPed(), q, HeadingTo(q, s_ppos));
        if (!ped) continue;
        Hash w = WeaponFor(s_level, BetterPct());
        if (i == 0 && role == 1) { Hash sw = SniperFor(s_level); if (sw) w = sw; }
        if (i == 0 && role == 2) { Hash hw = HeavyFor(s_level); if (hw) w = hw; }
        SetupPed(ped, s_level, w);
        if (i == 0 && role == 2) PoorAim(ped, g_cfg.explosiveAcc); // RPG/lanca-granadas: mira cinematografica
        bool sn = i == 0 && role == 1;
        if (sn) { PED::SET_PED_COMBAT_MOVEMENT(ped, 0); PED::SET_PED_COMBAT_RANGE(ped, 2); }
        Hunter &hh = AddHunter(ped);
        hh.sniper = sn;
        if (fromRoad) {
            AI::TASK_GO_TO_ENTITY(ped, s_player, -1, 25.f, 2.0f, 0, 0); // corre da rua ate o jogador
            hh.walking = true;
            if (D2(p, s_ppos) > 300.f) hh.farOk = true; // v10: vem de longe (ate 1500 m antes de ser removido)
        } else {
            Engage(ped);
        }
    }
    if (fromRoad) Log("  a pe: %d saindo da estrada mais proxima, a %.0fm", n, D2(p, s_ppos));
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
            Chase(ped);
            AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(ped, 0.f);
        } else {
            Engage(ped);
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
        Engage(g);
        Hunter &h = AddHunter(g);
        h.stayInVeh = true;
        h.rangeExempt = true;
        ++n;
    }
    return n;
}

// v10: jogador correndo de carro: o perseguidor nasce atras dele, na mesma mao, e ja sai na mesma velocidade
static bool FindChasePos(Vector3 &out, float &heading, float &speed) {
    float fx, fy, ph;
    speed = PlayerTravel(fx, fy, ph);
    for (float d : {170.f, 140.f, 210.f, 240.f, 120.f}) {
        Vector3 n; float h;
        if (!RoadNodeInTravelDir(s_ppos.x - fx * d, s_ppos.y - fy * d, fx, fy, n, h)) continue;
        float nd = D2(n, s_ppos);
        if (nd < 90.f || nd > 300.f) continue;
        float dot = ((n.x - s_ppos.x) * fx + (n.y - s_ppos.y) * fy) / nd;
        if (dot > -0.6f) continue; // tem que estar atras
        if (std::fabs(n.z - s_pgz) > 6.f + nd * 0.04f) continue;
        float lz;
        if (!SameLevel(n.x, n.y, lz, false)) continue;
        n.z += 0.5f;
        out = n; heading = h;
        return true;
    }
    return false;
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
    Vector3 p; float h, chaseSpeed = 0.f;
    bool chase = PlayerFastInCar() && strcmp(kind, "bicicleta") != 0 && FindChasePos(p, h, chaseSpeed);
    if (!chase && !FindRoadPos(dmin, dmax, p, h)) { Log("  %s: nenhuma rua encontrada.", kind); return; }
    Vehicle v = NewVehicle(model, p, h);
    if (!v) { Log("  %s: falha ao criar o veiculo.", kind); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);
    if (chase) {
        VEHICLE::SET_VEHICLE_FORWARD_SPEED(v, chaseSpeed * 0.95f);
        kind = strcmp(kind, "moto") ? "perseguidor" : "moto perseguidora";
    }
    int maxSeats = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v) + 1;
    if (seats > maxSeats) seats = maxSeats;
    FillVehicle(v, seats);
    int gunners = FillTurrets(v);
    s_v.push_back({v, g_st.now, false});
    Log("  %s com %d%s (%s) a %.0fm", kind, seats, gunners ? " + atirador na torreta" : "", s_fac->name, D2(p, s_ppos));
}

// v9: jogador longe de qualquer estrada?
static bool FarFromRoad() {
    Vector3 n;
    if (!PATHFIND::GET_CLOSEST_VEHICLE_NODE(s_ppos.x, s_ppos.y, s_pgz, &n, 1, 3.0f, 0.f)) return true;
    return D2(n, s_ppos) > g_cfg.remoteRoadDist;
}
// v9: tem agua (mar, lago, rio) a ate ~220 m?
static bool WaterNearby() {
    for (int i = 0; i < 12; ++i) {
        float dx, dy, wh;
        Dir(i * 30.f, dx, dy);
        for (float d : {90.f, 160.f, 220.f})
            if (WATER::GET_WATER_HEIGHT(s_ppos.x + dx * d, s_ppos.y + dy * d, s_ppos.z, &wh)) return true;
    }
    return false;
}

// v10.1: jogador de barco ou nadando em mar aberto (agua em volta, sem terra perto)
static bool PlayerAtSea() {
    if (!PED::IS_PED_IN_ANY_BOAT(s_player) && !PED::IS_PED_SWIMMING(s_player)) return false;
    for (int i = 0; i < 8; ++i) {
        float dx, dy, wh;
        Dir(i * 45.f, dx, dy);
        for (float d : {50.f, 110.f}) {
            float x = s_ppos.x + dx * d, y = s_ppos.y + dy * d, gz = 0.f;
            if (!WATER::GET_WATER_HEIGHT(x, y, s_ppos.z, &wh)) return false;
            // chao acima da agua = terra (ilha, praia, pier)
            if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(x, y, wh + 60.f, &gz, FALSE) && gz > wh + 0.5f) return false;
        }
    }
    return true;
}

static void SpawnBoat() {
    static std::vector<Hash> generic;
    if (generic.empty())
        for (const char *m : {"dinghy", "seashark", "tropic"})
            if (STREAMING::IS_MODEL_IN_CDIMAGE(H(m))) generic.push_back(H(m));
    Hash model = RandomOf(!s_fac->boats.empty() ? s_fac->boats : generic);
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
    return H(Rand(2) ? "WEAPON_COMBATMG" : "WEAPON_ASSAULTSHOTGUN"); // v10: sem MK2
}

// v10: guarda pesado de Cayo Perico (atirador de minigun do complexo de El Rubio): o guarda do cartel
// com o colete mais pesado do modelo (componente 9), sem chapeu nem bone.
// v10: traje pesado do Online (CLO_LTSM_O_*), numeros tirados do proprio jogo (lista de roupas do Online).
// Camuflagem sorteada (textura do mesmo traje): 0 Noturno (preto, mais comum), 1 Urbano, 2 Aco, 3 Tundra, 4 Floresta.
// Mascara sorteada: mascara de gas (38), balistica carbono preta (125/10) ou forcas especiais carbono (126/1).
// Capacete balistico: o do proprio traje pesado (prop 0/39, mesma camuflagem) ou o Balistico Merryweather (0/123).
static bool HeavyArmorLook(Ped j) {
    struct C { int comp, draw, tex; };
    static const int kCamoPick[] = {0, 0, 0, 1, 2, 3, 4}; // Noturno sai mais
    int camo = kCamoPick[Rand(7)];
    static const C kMasks[] = {{1, 38, 0}, {1, 125, 10}, {1, 126, 1}};
    C mask = kMasks[Rand(3)];
    const C suit[] = {mask, {3, 17, camo}, {4, 31, camo}, {6, 24, 0}, {8, 15, 0}, {11, 49, camo},
                      {2, 0, 0}, {5, 0, 0}, {7, 0, 0}, {9, 0, 0}, {10, 0, 0}};
    PED::SET_PED_HEAD_BLEND_DATA(j, 0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, FALSE);
    PED::SET_PED_DEFAULT_COMPONENT_VARIATION(j);
    bool ok = true;
    for (const C &c : suit) {
        if (PED::IS_PED_COMPONENT_VARIATION_VALID(j, c.comp, c.draw, c.tex)) PED::SET_PED_COMPONENT_VARIATION(j, c.comp, c.draw, c.tex, 0);
        else if (c.comp == 1 || c.comp == 11) ok = false;
    }
    PED::CLEAR_ALL_PED_PROPS(j);
    bool own = Rand(100) < 60;
    int hd = own ? 39 : 123, ht = own ? camo : 0;
    if (PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(j, 0) > hd) PED::SET_PED_PROP_INDEX(j, 0, hd, ht, TRUE);
    return ok;
}
static bool IsFreemode(Ped p) { return ENTITY::GET_ENTITY_MODEL(p) == GAMEPLAY::GET_HASH_KEY((char *)"mp_m_freemode_01"); }

static void HeavyGuardLook(Ped j) {
    if (IsFreemode(j)) { if (!HeavyArmorLook(j)) Log("  Juggernaut: alguma peca do traje pesado nao existe neste jogo."); return; }
    PED::SET_PED_DEFAULT_COMPONENT_VARIATION(j);
    int n = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(j, 9);
    for (int d = n - 1; d > 0; --d)
        if (PED::IS_PED_COMPONENT_VARIATION_VALID(j, 9, d, 0)) { PED::SET_PED_COMPONENT_VARIATION(j, 9, d, 0, 0); break; }
    PED::CLEAR_ALL_PED_PROPS(j);
}

// ---------- v10.1: Heavy (Juggernaut leve), um por faccao ----------
// Um membro "chefe" da propria faccao, com o colete mais pesado do modelo, bandana/mascara, chapeu e oculos
// (o que o modelo tiver), 1000 de vida e armas de categoria menor com carregador estendido.
struct LightJugDef { const char *fac; const char *models[2]; const char *weaps[4]; };
static const LightJugDef kLightJug[] = {
    {"MARABUNTA", {"g_m_y_salvaboss_01", "g_m_y_salvagoon_01"}, {"WEAPON_MICROSMG", "WEAPON_SAWNOFFSHOTGUN", "WEAPON_COMPACTRIFLE", nullptr}},
    {"CAIPIRAS", {"a_m_m_hillbilly_01", "a_m_m_rurmeth_01"}, {"WEAPON_PUMPSHOTGUN", "WEAPON_COMPACTRIFLE", "WEAPON_MICROSMG", nullptr}},
    {"VAGOS", {"g_m_y_mexgang_01", "g_m_y_mexgoon_03"}, {"WEAPON_MICROSMG", "WEAPON_MINISMG", "WEAPON_BULLPUPSHOTGUN", nullptr}},
    {"ONEIL", {"a_m_m_hillbilly_02", "a_m_m_farmer_01"}, {"WEAPON_PUMPSHOTGUN", "WEAPON_ASSAULTRIFLE", "WEAPON_DBSHOTGUN", nullptr}},
    {"BALLAS", {"g_m_y_ballaorig_01", "g_m_y_ballaeast_01"}, {"WEAPON_MICROSMG", "WEAPON_MINISMG", "WEAPON_BULLPUPSHOTGUN", "WEAPON_ASSAULTRIFLE"}},
    {"ALTRUISTAS", {"a_m_y_acult_02", "a_m_m_acult_01"}, {"WEAPON_BULLPUPSHOTGUN", "WEAPON_ASSAULTRIFLE", "WEAPON_COMPACTRIFLE", nullptr}},
    {"KKANGPAE", {"g_m_m_korboss_01", "g_m_y_korlieut_01"}, {"WEAPON_ASSAULTSMG", "WEAPON_CARBINERIFLE", "WEAPON_BULLPUPSHOTGUN", nullptr}},
    {"LOST", {"g_m_y_lost_01", "g_m_y_lost_03"}, {"WEAPON_GUSENBERG", "WEAPON_BULLPUPSHOTGUN", "WEAPON_ASSAULTRIFLE", "WEAPON_MINISMG"}},
    {"TRIADS", {"g_m_m_chiboss_01", "g_m_m_chicold_01"}, {"WEAPON_ASSAULTSMG", "WEAPON_MINISMG", "WEAPON_BULLPUPSHOTGUN", nullptr}},
    {"MADRAZO", {"g_m_m_mexboss_01", "g_m_m_mexboss_02"}, {"WEAPON_ASSAULTRIFLE", "WEAPON_COMPACTRIFLE", "WEAPON_BULLPUPSHOTGUN", "WEAPON_MICROSMG"}},
    {"CAYO", {"g_m_m_cartelguards_02", "g_m_m_cartelguards_01"}, {"WEAPON_CARBINERIFLE", "WEAPON_ASSAULTSMG", "WEAPON_BULLPUPSHOTGUN", nullptr}},
    {"CACADORES", {"s_m_y_blackops_03", "ig_hunter"}, {"WEAPON_CARBINERIFLE", "WEAPON_BULLPUPSHOTGUN", "WEAPON_SMG", nullptr}},
    {"BOGDAN", {"mp_m_bogdangoon", nullptr}, {"WEAPON_ASSAULTRIFLE", "WEAPON_ASSAULTSMG", "WEAPON_BULLPUPSHOTGUN", nullptr}},
    {"MERRYWEATHER", {nullptr, nullptr}, {"WEAPON_CARBINERIFLE", "WEAPON_ASSAULTSMG", "WEAPON_BULLPUPSHOTGUN", nullptr}},
};
static const LightJugDef *LightJugFor() {
    for (const LightJugDef &d : kLightJug) if (!strcmp(d.fac, s_fac->id)) return &d;
    return nullptr;
}
static Hash LightJugModel(const LightJugDef *d) {
    if (d) {
        int first = d->models[1] ? Rand(2) : 0;
        for (int k = 0; k < 2; ++k) {
            const char *n = d->models[(first + k) % 2];
            if (n && STREAMING::IS_MODEL_IN_CDIMAGE(H(n))) return H(n);
        }
    }
    return RandomPed();
}
static Hash LightJugWeapon(const LightJugDef *d, const char **name) {
    static const char *kDef[] = {"WEAPON_ASSAULTSMG", "WEAPON_BULLPUPSHOTGUN", "WEAPON_CARBINERIFLE", nullptr};
    const char *const *w = d ? d->weaps : kDef;
    int n = 0;
    while (n < 4 && w[n]) ++n;
    for (int t = 0; t < 6; ++t) {
        const char *c = w[Rand(n)];
        if (WEAPON::IS_WEAPON_VALID(H(c))) { *name = c; return H(c); }
    }
    *name = "WEAPON_ASSAULTSMG";
    return H("WEAPON_ASSAULTSMG");
}
// carregador estendido, se a arma tiver (COMPONENT_<arma>_CLIP_02)
static void ExtendedClip(Ped p, Hash w, const char *wname) {
    char comp[64];
    snprintf(comp, sizeof comp, "COMPONENT_%s_CLIP_02", wname + 7); // pula o "WEAPON_"
    Hash c = H(comp);
    if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(w, c)) WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(p, w, c);
}
static void LightJugLook(Ped j) {
    // colete: o desenho mais alto do componente 9 costuma ser o mais pesado
    int vest = 0, nv = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(j, 9);
    for (int d = nv - 1; d > 0; --d)
        if (PED::IS_PED_COMPONENT_VARIATION_VALID(j, 9, d, 0)) { PED::SET_PED_COMPONENT_VARIATION(j, 9, d, 0, 0); vest = d; break; }
    // bandana/mascara (componente 1), se o modelo tiver
    int mask = 0, nm = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(j, 1);
    if (nm > 1) {
        int d = 1 + Rand(nm - 1);
        if (PED::IS_PED_COMPONENT_VARIATION_VALID(j, 1, d, 0)) { PED::SET_PED_COMPONENT_VARIATION(j, 1, d, 0, 0); mask = d; }
    }
    // chapeu/bandana na cabeca (prop 0) e oculos escuros (prop 1)
    PED::CLEAR_ALL_PED_PROPS(j);
    int hat = -1, eyes = -1;
    int nh = PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(j, 0), ne = PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(j, 1);
    if (nh > 0) { hat = Rand(nh); PED::SET_PED_PROP_INDEX(j, 0, hat, 0, TRUE); }
    if (ne > 0) { eyes = Rand(ne); PED::SET_PED_PROP_INDEX(j, 1, eyes, 0, TRUE); }
    Log("  Heavy: colete %d/%d, mascara %d/%d, cabeca %d/%d, oculos %d/%d", vest, nv, mask, nm, hat, nh, eyes, ne);
}
static void SetupLightJug(Ped j) {
    const LightJugDef *d = LightJugFor();
    const char *wn = "";
    Hash jw = LightJugWeapon(d, &wn);
    SetupPed(j, s_level, jw);
    LightJugLook(j); // depois do SetupPed, que pode trocar o colete
    ExtendedClip(j, jw, wn);
    PED::SET_PED_MAX_HEALTH(j, g_cfg.jugLightHealth);
    ENTITY::SET_ENTITY_HEALTH(j, g_cfg.jugLightHealth);
    PED::SET_PED_ARMOUR(j, 100);
    PED::SET_PED_SUFFERS_CRITICAL_HITS(j, FALSE);
    PED::SET_PED_RAGDOLL_ON_COLLISION(j, FALSE);
    PED::SET_PED_FIRING_PATTERN(j, 0xC6EE6B4C); // rajada continua
    PED::SET_PED_COMBAT_MOVEMENT(j, 2);          // avanca
    PED::SET_PED_COMBAT_RANGE(j, 1);
    Engage(j);
    Hunter &hj = AddHunter(j);
    hj.juggernaut = true;
    hj.lightJug = true;
    Log("  HEAVY a caminho (%s, %s)", s_fac->name, wn + 7);
}
// v10.1: o leve so nos niveis dele; onde o pesado tambem pode vir, sorteia um dos dois
static bool HeavyJugOk() { return s_level >= g_cfg.jugMinLevel || g_st.Pts() >= g_cfg.escStart; }
static bool LightJugOk() {
    return g_cfg.jugLightEnabled && s_level >= g_cfg.jugLightMinLevel && s_level <= g_cfg.jugLightMaxLevel;
}

// Juggernaut no Insurgent blindado, com motorista e tropa.
// v10: so o Insurgent sem torreta (na torreta o jogador era expulso no modo historia).
static void SpawnJuggernaut() {
    bool light = LightJugOk() && (!HeavyJugOk() || Rand(2)); // v10.1
    Hash model = 0;
    if (light) { // carro da faccao com pelo menos um banco de passageiro
        for (int t = 0; t < 6 && !model; ++t) {
            Hash c = RandomOf(s_fac->cars);
            if (c && invoke<int>(0x2AD93716F184EDA4, c) >= 2) model = c;
        }
    } else {
        for (const char *n : {"insurgent2"})
            if (STREAMING::IS_MODEL_IN_CDIMAGE(H(n))) { model = H(n); break; }
    }
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
            Chase(ped);
            AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(ped, 25.f);
        } else {
            Engage(ped);
        }
        AddHunter(ped).stayInVeh = true;
    }

    // o Juggernaut
    if (jugSeat != -2 && light) {
        Ped j = NewPedIn(v, LightJugModel(LightJugFor()), jugSeat);
        if (j) SetupLightJug(j);
    } else if (jugSeat != -2) {
        // v10: o NPC Juggernaut blindado do jogo (todo preto), o mesmo para todos.
        // O modelo vem do .ini ([Juggernaut] Model); se nao existir, tenta os outros Juggernauts do jogo.
        Hash jm = 0;
        bool suit = false;
        for (const char *n : {g_cfg.jugModel.c_str(), "mp_m_freemode_01", "s_m_y_blackops_01"})
            if (*n && STREAMING::IS_MODEL_IN_CDIMAGE(H(n))) { jm = H(n); suit = true; break; }
        if (!jm) { jm = H("s_m_y_blackops_01"); Log("  Juggernaut: modelo %s nao encontrado no jogo.", g_cfg.jugModel.c_str()); }
        if (!STREAMING::IS_MODEL_IN_CDIMAGE(jm)) jm = RandomPed();
        Ped j = NewPedIn(v, jm, jugSeat);
        if (j) {
            if (suit) {
                HeavyGuardLook(j);
            } else {
                int n = PED::GET_NUMBER_OF_PED_DRAWABLE_VARIATIONS(j, 9);
                if (n > 1) PED::SET_PED_COMPONENT_VARIATION(j, 9, n - 1, 0, 0);
                if (PED::GET_NUMBER_OF_PED_PROP_DRAWABLE_VARIATIONS(j, 0) > 0) PED::SET_PED_PROP_INDEX(j, 0, 0, 0, TRUE);
            }
            Hash jw = JuggWeapon();
            SetupPed(j, s_level, jw);
            if (suit) HeavyGuardLook(j); // SetupPed pode ter trocado o colete
            PED::SET_PED_MAX_HEALTH(j, g_cfg.jugHealth);
            ENTITY::SET_ENTITY_HEALTH(j, g_cfg.jugHealth);
            PED::SET_PED_ARMOUR(j, 100);
            PED::SET_PED_SUFFERS_CRITICAL_HITS(j, FALSE);
            // v10: imune a corpo a corpo (bloqueia a coronhada que mata na hora); tiros e explosoes funcionam
            ENTITY::SET_ENTITY_PROOFS(j, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
            PED::SET_PED_CAN_RAGDOLL(j, FALSE);
            PED::SET_PED_RAGDOLL_ON_COLLISION(j, FALSE);
            PED::SET_PED_FIRING_PATTERN(j, 0xC6EE6B4C); // rajada continua
            if (jw == H("WEAPON_MINIGUN")) PED::SET_PED_ACCURACY(j, g_cfg.jugMinigunAcc); // v10: minigun bem imprecisa
            PED::SET_PED_COMBAT_MOVEMENT(j, 2);           // avanca
            PED::SET_PED_COMBAT_ATTRIBUTES(j, 0, FALSE);  // nao usa cobertura
            PED::SET_PED_COMBAT_RANGE(j, 1);
            Engage(j);
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
        Engage(ped);
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
    // v10: rapido, o bloqueio nasce mais longe (da tempo de ver e frear); e so na mao em que o jogador vai
    bool fast = sp > kFastSpeed;
    std::vector<float> ds = fast ? std::vector<float>{380.f, 340.f, 420.f, 300.f} : std::vector<float>{300.f, 260.f, 220.f, 340.f};
    for (float d : ds) {
        Vector3 n; float h;
        if (!RoadNodeInTravelDir(s_ppos.x + fx * d, s_ppos.y + fy * d, fx, fy, n, h)) continue;
        float nd = D2(n, s_ppos);
        if (nd < 180.f || nd > (fast ? 460.f : 380.f)) continue;
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
            Engage(ped);
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
struct HeliSt { Vehicle veh; Ped pilot; HeliType type; int state; int at; Vector3 drop; bool land;
                float holdHeading = 0.f; int nextHold = 0;
                int nextSwap = 0, wi = 0, nextAttack = 0; }; // v10: helicoptero de ataque (so o piloto)
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
// v9.3: helicopteros com armas proprias (metralhadoras, canhoes, misseis)
static bool ArmedHeli(Hash m) {
    static const Hash list[] = {H("annihilator"), H("valkyrie"), H("valkyrie2"), H("buzzard"),
                                H("savage"),      H("hunter"),   H("akula")};
    for (Hash x : list) if (m == x) return true;
    return false;
}
// v10: armas do piloto por modelo (metralhadora/canhao e misseis). As que o helicoptero nao tiver sao puladas.
static const char *kAttackWeaps[] = {"VEHICLE_WEAPON_PLAYER_BUZZARD", "VEHICLE_WEAPON_SPACE_ROCKET",
                                     "VEHICLE_WEAPON_PLAYER_SAVAGE",  "VEHICLE_WEAPON_HUNTER_MG",
                                     "VEHICLE_WEAPON_HUNTER_MISSILE", "VEHICLE_WEAPON_HUNTER_BARRAGE",
                                     "VEHICLE_WEAPON_AKULA_MISSILE",  "VEHICLE_WEAPON_AKULA_BARRAGE"};
static void AttackHeliWeapon(HeliSt &st) {
    const int n = sizeof kAttackWeaps / sizeof *kAttackWeaps;
    for (int t = 0; t < n; ++t) {
        st.wi = (st.wi + 1) % n;
        if (WEAPON::SET_CURRENT_PED_VEHICLE_WEAPON(st.pilot, H(kAttackWeaps[st.wi]))) return;
    }
}
static void AttackHeliEngage(HeliSt &st) {
    PED::SET_PED_COMBAT_ATTRIBUTES(st.pilot, 52, TRUE); // usar o veiculo para atacar
    PED::SET_PED_COMBAT_ATTRIBUTES(st.pilot, 53, TRUE); // usar as armas montadas no veiculo
    PED::SET_PED_FIRING_PATTERN(st.pilot, 0xC6EE6B4C);  // rajada continua
    PED::SET_PED_SHOOT_RATE(st.pilot, 100);
    Engage(st.pilot);
}

// Ronda o jogador. Helicoptero armado entra em combate, para usar as armas dele em vez de so voar em volta.
static void PilotCircle(Ped pilot, Vehicle v, float radius) {
    if (ArmedHeli(ENTITY::GET_ENTITY_MODEL(v))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(v, TRUE, TRUE, FALSE);
        Engage(pilot);
        return;
    }
    AI::TASK_HELI_MISSION(pilot, v, 0, s_player, 0.f, 0.f, 0.f, 9, 35.f, radius, -1.f, 40, 25, -1.f, 0);
}
// v9.3: fica parado no ar, no mesmo ponto e direcao, enquanto a tropa desce de corda
static void PilotHold(Ped pilot, Vehicle v, const Vector3 &p, float heading) {
    AI::TASK_HELI_MISSION(pilot, v, 0, 0, p.x, p.y, p.z, 4, 1.0f, 0.f, heading, -1, -1, -1.f, 0);
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
    if (n == 1) return s_level >= 6 ? Pct(g_cfg.heliSecondPct, EscBonus() * 0.33f + (g_st.Pts() >= (int)(90 * PtsScale()) ? 5 : 0), g_cfg.heliSecondMax) : 0;
    if (n == 2) return s_level >= 8 ? Pct(g_cfg.heliThirdPct, TopBonus() * 0.3f, g_cfg.heliThirdMax) : 0;
    return 0;
}

static void SpawnHeli(bool air, bool sea = false) {
    const std::vector<Hash> &transport = !s_fac->helis.empty() ? s_fac->helis : s_genericHelis;
    HeliType type = HT_SHOOTERS;
    Hash model = 0;
    if (sea) { // v10.1: jogador em mar aberto: so helicoptero de ataque (o da faccao ou um Buzzard)
        static const Hash buzzard = H("buzzard");
        type = HT_ATTACK;
        model = !s_fac->attackHelis.empty() ? RandomOf(s_fac->attackHelis) : buzzard;
        if (!STREAMING::IS_MODEL_IN_CDIMAGE(model)) return;
    } else if (!s_fac->attackHelis.empty() && s_level >= g_cfg.heliAttackMinLevel && // v10: armados so do nivel 7 em diante
        Rand(100) < Pct(g_cfg.heliAttackPct, EscBonus() * 0.25f, g_cfg.heliAttackMax) + (air ? 10 : 0)) {
        type = HT_ATTACK;
        model = RandomOf(s_fac->attackHelis);
    } else if (!air && PlayerOnFootGround() && Rand(100) < g_cfg.heliRappelPct) { // v10: rapel so com o jogador a pe
        std::vector<Hash> rp;
        for (Hash m : transport) if (CanRappel(m)) rp.push_back(m);
        if (!rp.empty()) { type = HT_RAPPEL; model = RandomOf(rp); }
    }
    if (!model) model = RandomOf(transport);
    if (!model) return;
    // v11: na Desordem, nada de helicoptero com misseis ou canhao explosivo, menos o Buzzard
    if (g_cfg.mode == 4) {
        static const Hash banned[] = {H("valkyrie"), H("valkyrie2"), H("savage"), H("hunter"), H("akula")};
        auto isBanned = [&](Hash m) { for (Hash b : banned) if (m == b) return true; return false; };
        if (isBanned(model)) {
            if (type == HT_ATTACK) model = H("buzzard");
            else {
                std::vector<Hash> ok;
                for (Hash m : transport) if (!isBanned(m) && (type != HT_RAPPEL || CanRappel(m))) ok.push_back(m);
                model = ok.empty() ? H("maverick") : RandomOf(ok);
            }
        }
    }

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
    PED::SET_PED_ACCURACY(pilot, g_cfg.heliAccuracy); // v10: armas do helicoptero bem imprecisas
    PED::SET_PED_COMBAT_ATTRIBUTES(pilot, 3, FALSE); // piloto nao sai do helicoptero
    Hunter &hp = AddHunter(pilot);
    hp.stayInVeh = true;
    hp.rangeExempt = true;

    int maxP = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(v), crew = 0;
    {
        int first = maxP > 1 ? 1 : 0; // portas de tras; o copiloto fica vazio
        int last = type == HT_RAPPEL ? (maxP - 1 < 4 ? maxP - 1 : 4) : (maxP - 1 < 3 ? maxP - 1 : 3);
        if (type == HT_ATTACK) last = first - 1; // v10: Buzzard/Savage/Hunter/Akula: so o piloto, usando as armas
        for (int s = first; s <= last && Budget() > 0; ++s) {
            Ped ped = NewPedIn(v, RandomPed(), s);
            if (!ped) continue;
            SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
            PED::SET_PED_ACCURACY(ped, g_cfg.heliAccuracy); // v10: "stormtrooper" (NormalAim ao descer)
            PED::SET_PED_COMBAT_ATTRIBUTES(ped, 3, FALSE);
            if (type != HT_RAPPEL) Engage(ped);
            Hunter &h = AddHunter(ped);
            h.stayInVeh = true;
            h.heliRappel = type == HT_RAPPEL;
            // v9.3: quem senta numa torreta (ex.: metralhadoras laterais do Valkyrie) usa a arma do helicoptero
            if (invoke<BOOL>(0xE33FFA906CE74880, v, s)) { h.rangeExempt = true; PoorAim(ped, g_cfg.heliAccuracy); } // IS_TURRET_SEAT
            ++crew;
        }
    }

    // v5: ataque e rapel podem decidir pousar perto do jogador (so se ele estiver a pe)
    bool land = !air && type == HT_RAPPEL && crew > 0 && Rand(100) < g_cfg.heliLandPct; // v10: ataque nao pousa
    HeliSt st{v, pilot, type, 0, g_st.now, {}, land};
    if (type == HT_ATTACK) {
        PoorAim(pilot, g_cfg.heliAccuracy < g_cfg.explosiveAcc ? g_cfg.heliAccuracy : g_cfg.explosiveAcc); // misseis passam perto
        AttackHeliEngage(st); // v10: so o piloto, atacando com as armas do helicoptero
        AttackHeliWeapon(st);
        st.nextSwap = g_st.now + 6000;
        st.nextAttack = g_st.now + 8000;
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

        // v10: helicoptero de ataque alterna metralhadora/canhao e misseis e nao para de atacar
        if (st.type == HT_ATTACK) {
            if (g_st.now >= st.nextSwap) { AttackHeliWeapon(st); st.nextSwap = g_st.now + 5000 + Rand(4000); }
            if (g_st.now >= st.nextAttack) { AttackHeliEngage(st); st.nextAttack = g_st.now + 10000; }
        }

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
                if (st.type == HT_ATTACK) Engage(st.pilot);
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
                            NormalAim(h.ped);
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
            if (st.type == HT_ATTACK) Engage(st.pilot); // decola e volta a atacar
            else PilotCircle(st.pilot, st.veh, 55.f);
        }

        if (st.type == HT_RAPPEL && st.state == 0 && !st.land) {
            // segue o jogador se ele se afastar do ponto de descida
            if (D2(st.drop, s_ppos) > 45.f && g_st.now - st.at > 5000) {
                st.drop = DropPoint();
                st.at = g_st.now;
                PilotGoTo(st.pilot, st.veh, st.drop);
            }
            // v9.3: so joga as cordas quando estiver parado no ar (pairando) sobre o ponto
            Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(st.veh);
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
            bool overDrop = D2(hp, st.drop) < 20.f && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(st.veh) < 38.f;
            if (overDrop && speed > 2.5f) PilotHold(st.pilot, st.veh, hp, ENTITY::GET_ENTITY_HEADING(st.veh)); // freia
            if ((overDrop && speed <= 2.5f) || g_st.now - st.at > 60000) {
                st.drop = hp; // daqui em diante, st.drop = ponto em que ele fica parado
                st.holdHeading = ENTITY::GET_ENTITY_HEADING(st.veh);
                PilotHold(st.pilot, st.veh, st.drop, st.holdHeading);
                for (Hunter &h : s_h)
                    if (h.heliRappel && PED::IS_PED_IN_VEHICLE(h.ped, st.veh, FALSE))
                        invoke<Void>(0x09693B0312F91649, h.ped, 10.0f); // TASK_RAPPEL_FROM_HELI
                st.state = 1;
                st.at = g_st.now;
                st.nextHold = g_st.now + 2000;
                Log("  rapel: helicoptero parado, tropa descendo de corda.");
            }
        } else if (st.type == HT_RAPPEL && st.state == 1) {
            // continua parado no mesmo ponto ate todos descerem (sem esticar as cordas)
            if (g_st.now >= st.nextHold) {
                st.nextHold = g_st.now + 2000;
                PilotHold(st.pilot, st.veh, st.drop, st.holdHeading);
            }
            bool pending = false;
            for (Hunter &h : s_h) {
                if (!h.heliRappel) continue;
                bool out = !PED::IS_PED_IN_ANY_VEHICLE(h.ped, FALSE);
                if ((out && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(h.ped) < 2.5f) || g_st.now - st.at > 25000) {
                    h.heliRappel = false;
                    h.stayInVeh = false;
                    NormalAim(h.ped);
                    Engage(h.ped);
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
struct TankSt { Vehicle cargo = 0, rhino = 0; Ped pilot = 0, driver = 0; int state = -1, at = 0; Vector3 drop{}; bool planB = false;
                int nextShot = 0, openAt = 0; bool gunOpen = false, chasing = false; };
static TankSt s_tank;

// v10.1: trava de verdade das armas de veiculo. O DISABLE_VEHICLE_WEAPON e o SHOOT_RATE 0 nao seguram a IA
// (o log da v10 mostrava toda janela do Rhino "sem tiro": ele atirava fora delas). A municao restrita do
// veiculo segura: 0 = nao atira, 1 = um tiro, -1 = ilimitada (normal do jogo). Indices 0-3 das armas.
static std::vector<Vehicle> s_ammoTouched; // veiculos com a municao mexida (devolvidos ao normal depois)
static void VehAmmo(Vehicle v, int idx, int n) {
    invoke<Void>(0x44CD1F493DB2A0A6, v, idx, n); // SET_VEHICLE_WEAPON_RESTRICTED_AMMO
    if (n != -1 && std::find(s_ammoTouched.begin(), s_ammoTouched.end(), v) == s_ammoTouched.end()) s_ammoTouched.push_back(v);
}
static int VehAmmoGet(Vehicle v, int idx) { return invoke<int>(0x8181CE2F25CB9BB7, v, idx); }
static void VehAmmoAll(Vehicle v, int n) { for (int i = 0; i < 4; ++i) VehAmmo(v, i, n); }
// devolve a municao normal (veiculo liberado pelo mod ou tomado pelo jogador)
static void VehAmmoRestore(Vehicle v) {
    auto it = std::find(s_ammoTouched.begin(), s_ammoTouched.end(), v);
    if (it == s_ammoTouched.end()) return;
    if (ENTITY::DOES_ENTITY_EXIST(v)) for (int i = 0; i < 4; ++i) invoke<Void>(0x44CD1F493DB2A0A6, v, i, -1);
    s_ammoTouched.erase(it);
}
// usar (ou nao) as armas montadas do veiculo no combate
static void VehAttack(Ped p, bool on) {
    PED::SET_PED_COMBAT_ATTRIBUTES(p, 52, on);
    PED::SET_PED_COMBAT_ATTRIBUTES(p, 53, on);
}
// v10: espera depois que um tanque sai de combate (abatido, motorista morto ou roubado pelo jogador)
static int s_tankCdUntil = 0, s_khanCdUntil = 0;
static void TankCooldown(int &until, const char *name) {
    until = g_st.now + g_cfg.tankCooldown * 1000;
    Log("  %s: proximo so daqui a %ds.", name, g_cfg.tankCooldown);
}
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
        PoorAim(t.driver, g_cfg.explosiveAcc); // canhao do tanque impreciso
        // v9: pendurado no Cargobob o tanque nao atira; o canhao so libera depois de tocar o chao
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, TRUE);
        PED::SET_PED_SHOOT_RATE(t.driver, 0);
        VEHICLE::DISABLE_VEHICLE_WEAPON(TRUE, H("VEHICLE_WEAPON_TANK"), t.rhino, t.driver);
        VehAmmo(t.rhino, 0, 0); // v10.1: sem municao ate a primeira janela
        VehAttack(t.driver, false);
        AddCrew(t.driver).tankGunner = true;
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

// v10: o tanque viaja sem colisao preso no Cargobob; se ele acabar abaixo do chao (Cargobob caiu, PEM...),
// coloca de volta em cima do terreno
static bool FixUnderground(Vehicle v) {
    Vector3 p = ENTITY::GET_ENTITY_COORDS(v, TRUE);
    float gz = 0.f;
    if (!GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(p.x, p.y, p.z + 60.f, &gz, FALSE)) return false;
    if (p.z > gz - 0.6f) return false;
    ENTITY::SET_ENTITY_COORDS(v, p.x, p.y, gz + 1.5f, FALSE, FALSE, FALSE, FALSE);
    VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(v);
    Log("  tanque estava abaixo do chao: recolocado em cima do terreno.");
    return true;
}

static void UpdateTank() {
    TankSt &t = s_tank;
    if (t.state < 0) return;
    bool rhinoOk = ENTITY::DOES_ENTITY_EXIST(t.rhino) && !ENTITY::IS_ENTITY_DEAD(t.rhino);
    if (!rhinoOk) { t.state = -1; Log("Tanque destruido."); TankCooldown(s_tankCdUntil, "tanque no Cargobob"); return; }
    bool cargoOk = ENTITY::DOES_ENTITY_EXIST(t.cargo) && !ENTITY::IS_ENTITY_DEAD(t.cargo) && t.pilot &&
                   !PED::IS_PED_DEAD_OR_DYING(t.pilot, TRUE);
    if (t.state == 0) {
        // plano B: se o gancho nao pegou, prende o tanque direto embaixo do Cargobob
        if (cargoOk && !t.planB && g_st.now - t.at > 1500 && !VEHICLE::IS_VEHICLE_ATTACHED_TO_CARGOBOB(t.rhino, t.cargo)) {
            ENTITY::ATTACH_ENTITY_TO_ENTITY(t.rhino, t.cargo, 0, 0.f, 0.f, g_cfg.tankHangZ, 0.f, 0.f, 0.f, FALSE, FALSE, FALSE, FALSE, 2, TRUE);
            t.planB = true;
            Log("  tanque: gancho nao pegou, preso direto no Cargobob (plano B).");
        }
        bool there = cargoOk && D2(ENTITY::GET_ENTITY_COORDS(t.cargo, TRUE), t.drop) < 18.f &&
                     ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(t.cargo) < 42.f;
        // v10: Cargobob sem motor (PEM) ou baixo demais: solta o tanque antes dele atravessar o chao
        bool engineOff = cargoOk && !VEHICLE::_IS_VEHICLE_ENGINE_ON(t.cargo);
        bool low = g_st.now - t.at > 3000 && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(t.rhino) < 4.f;
        if (!cargoOk || there || engineOff || low || g_st.now - t.at > 70000) {
            ReleaseTankFromHook(t);
            FixUnderground(t.rhino);
            if (engineOff || low) Log("  tanque: Cargobob sem motor ou baixo demais, tanque solto.");
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
        FixUnderground(t.rhino);
        if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(t.rhino) < 1.5f || g_st.now - t.at > 8000) {
            ENTITY::SET_ENTITY_INVINCIBLE(t.rhino, FALSE);
            if (t.driver && !PED::IS_PED_DEAD_OR_DYING(t.driver, TRUE)) {
                // v9: no chao, libera o canhao e comeca a cacar
                // v10: o canhao continua travado; UpdateTank libera um tiro a cada ShotInterval segundos
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, FALSE);
                Engage(t.driver);
                t.nextShot = g_st.now + 3000;
                t.gunOpen = false;
            }
            t.state = 2;
            FixUnderground(t.rhino);
            Log("  tanque no chao, cacando o jogador.");
        }
    } else if (t.state == 2 && (!t.driver || PED::IS_PED_DEAD_OR_DYING(t.driver, TRUE))) {
        t.state = -1;
        Log("Motorista do tanque abatido.");
        TankCooldown(s_tankCdUntil, "tanque no Cargobob");
    } else if (t.state == 2) {
        // v10: um tiro do canhao a cada ShotInterval segundos, e so com o tanque no raio do blip.
        // Entre os tiros o canhao fica travado (reaplicado a cada atualizacao) e o tanque so persegue o jogador.
        // A janela de tiro dura no maximo 1,5 s (o canhao do Rhino recarrega mais devagar que isso), e depois
        // dela o intervalo e sempre o cheio, tendo atirado ou nao.
        static const Hash tankGun = H("VEHICLE_WEAPON_TANK");
        bool inRange = D2(ENTITY::GET_ENTITY_COORDS(t.rhino, TRUE), s_ppos) <= g_cfg.rangeRpg + 20.f; // v10
        if (!t.gunOpen && inRange && g_st.now >= t.nextShot) {
            VEHICLE::DISABLE_VEHICLE_WEAPON(FALSE, tankGun, t.rhino, t.driver);
            VehAmmo(t.rhino, 0, 1); // v10.1: exatamente um tiro nesta janela
            VehAttack(t.driver, true);
            PED::SET_PED_COMBAT_ATTRIBUTES(t.driver, 2, TRUE);
            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, FALSE);
            PED::SET_PED_SHOOT_RATE(t.driver, 100);
            Engage(t.driver);
            t.gunOpen = true;
            t.openAt = g_st.now;
        } else if (t.gunOpen) {
            bool shot = PED::IS_PED_SHOOTING(t.driver) || VehAmmoGet(t.rhino, 0) == 0; // v10.1: gastou o tiro
            if (shot || !inRange || g_st.now - t.openAt > 2500) {
                t.gunOpen = false;
                t.nextShot = g_st.now + g_cfg.tankShotInterval * 1000;
                t.chasing = false;
                Log("  tanque: janela de tiro fechada (%s); proximo tiro em %ds.", shot ? "atirou" : "sem tiro",
                    g_cfg.tankShotInterval);
            }
        }
        if (!t.gunOpen) {
            // travado: reaplica sempre, porque a IA de combate tenta religar o canhao
            VEHICLE::DISABLE_VEHICLE_WEAPON(TRUE, tankGun, t.rhino, t.driver);
            VehAmmo(t.rhino, 0, 0); // v10.1: a trava que a IA respeita
            VehAttack(t.driver, false);
            PED::SET_PED_SHOOT_RATE(t.driver, 0);
            PED::SET_PED_COMBAT_ATTRIBUTES(t.driver, 2, FALSE);
            if (!t.chasing) { // so persegue, sem combate, ate a proxima janela
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(t.driver, TRUE); // nao volta sozinho ao combate
                Chase(t.driver);
                AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(t.driver, 35.f);
                t.chasing = true;
            }
        }
    }
}

// ---------- tanque especial TM-02 Khanjali (v10) ----------
// Raro (5% fixo por wave, so no nivel 8 ate o maximo), vem por terra, um por vez. O canhao segue a mesma regra
// do Rhino: um tiro a cada ShotInterval segundos, so dentro do alcance, mira fraca.
struct KhanSt { Vehicle v = 0; Ped driver = 0; int nextShot = 0, openAt = 0; bool gunOpen = false, chasing = false; };
static KhanSt s_khan;
static bool KhanActive() {
    if (s_khan.v) return true;
    for (auto &p : s_q) if (p.k == P_KHAN) return true;
    return false;
}
// v10: so os canhoes seguem o intervalo; as metralhadoras ficam livres, com a mira fraca do piloto (bem espalhadas)
static const char *kKhanGuns[] = {"VEHICLE_WEAPON_KHANJALI_CANNON", "VEHICLE_WEAPON_KHANJALI_CANNON_HEAVY",
                                  "VEHICLE_WEAPON_KHANJALI_GL"};
static void KhanGuns(bool disable) {
    for (const char *g : kKhanGuns) VEHICLE::DISABLE_VEHICLE_WEAPON(disable, H(g), s_khan.v, s_khan.driver);
    // v10.1: metralhadora tambem so na janela (municao restrita em todas as armas)
    VehAmmoAll(s_khan.v, disable ? 0 : -1);
    VehAttack(s_khan.driver, !disable);
}
static void SpawnKhanjali() {
    Hash m = H("khanjali");
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(m)) { Log("  Khanjali: modelo nao encontrado."); return; }
    Vector3 p; float h, sp = 0.f;
    bool chase = PlayerFastInCar() && FindChasePos(p, h, sp);
    if (!chase && !FindRoadPos(g_cfg.vehMin, g_cfg.vehMax, p, h)) { Log("  Khanjali: nenhuma rua encontrada."); return; }
    KhanSt k;
    k.v = NewVehicle(m, p, h);
    if (!k.v) { Log("  Khanjali: falha ao criar."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(k.v, TRUE, TRUE, FALSE);
    if (chase) VEHICLE::SET_VEHICLE_FORWARD_SPEED(k.v, sp * 0.9f);
    k.driver = NewPedIn(k.v, RandomPed(), -1);
    if (!k.driver) { VEHICLE::DELETE_VEHICLE(&k.v); return; }
    SetupPed(k.driver, s_level, WeaponFor(s_level));
    PoorAim(k.driver, g_cfg.explosiveAcc);
    PED::SET_PED_COMBAT_ATTRIBUTES(k.driver, 3, FALSE); // nao sai do tanque
    PED::SET_DRIVER_ABILITY(k.driver, 1.0f);
    Hunter &hk = AddHunter(k.driver);
    hk.stayInVeh = true;
    hk.tankGunner = true; // o canhao e controlado aqui
    hk.farOk = true;
    s_khan = k;
    KhanGuns(true);
    PED::SET_PED_SHOOT_RATE(s_khan.driver, 0);
    s_khan.nextShot = g_st.now + 4000;
    s_v.push_back({s_khan.v, g_st.now, false});
    Log("  TANQUE ESPECIAL Khanjali (%s) a %.0fm, por terra", s_fac->name, D2(p, s_ppos));
}
static void UpdateKhanjali() {
    KhanSt &k = s_khan;
    if (!k.v) return;
    if (!ENTITY::DOES_ENTITY_EXIST(k.v) || ENTITY::IS_ENTITY_DEAD(k.v) || !k.driver || !ENTITY::DOES_ENTITY_EXIST(k.driver) ||
        PED::IS_PED_DEAD_OR_DYING(k.driver, TRUE) || !PED::IS_PED_IN_VEHICLE(k.driver, k.v, FALSE) ||
        D2(ENTITY::GET_ENTITY_COORDS(k.v, TRUE), s_ppos) > 1500.f) {
        Log("Khanjali fora de combate.");
        TankCooldown(s_khanCdUntil, "Khanjali");
        k = KhanSt{};
        return;
    }
    bool inRange = D2(ENTITY::GET_ENTITY_COORDS(k.v, TRUE), s_ppos) <= g_cfg.rangeRpg + 20.f;
    if (!k.gunOpen && inRange && g_st.now >= k.nextShot) {
        KhanGuns(false);
        PED::SET_PED_COMBAT_ATTRIBUTES(k.driver, 2, TRUE);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(k.driver, FALSE);
        PED::SET_PED_SHOOT_RATE(k.driver, 100);
        Engage(k.driver);
        k.gunOpen = true;
        k.openAt = g_st.now;
    } else if (k.gunOpen) {
        bool shot = PED::IS_PED_SHOOTING(k.driver);
        if (shot || !inRange || g_st.now - k.openAt > 1500) {
            k.gunOpen = false;
            k.chasing = false;
            k.nextShot = g_st.now + g_cfg.tankShotInterval * 1000;
        }
    }
    if (!k.gunOpen) {
        KhanGuns(true);
        PED::SET_PED_SHOOT_RATE(k.driver, 0); // v10.1: fora da janela nada atira
        if (!k.chasing) {
            PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(k.driver, TRUE);
            Chase(k.driver);
            AI::SET_TASK_VEHICLE_CHASE_IDEAL_PURSUIT_DISTANCE(k.driver, 35.f);
            k.chasing = true;
        }
    }
}

// ---------- jato (v6) ----------
// Lazer (padrao) ou Hydra (mais raro). Ataca alternando canhao e misseis e vai embora depois de um tempo.
struct JetSt { Vehicle v = 0; Ped pilot = 0; int born = 0, nextSwap = 0, wi = 0; bool leaving = false; };
static JetSt s_jet;
static int s_jetCooldownUntil = 0; // v10: sem caca ate esse tempo (depois de abatido ou de ir embora)
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
    PoorAim(j.pilot, g_cfg.explosiveAcc); // misseis e canhao do jato imprecisos
    PED::SET_PED_COMBAT_ATTRIBUTES(j.pilot, 3, FALSE);
    AddCrew(j.pilot);
    AI::TASK_PLANE_MISSION(j.pilot, j.v, 0, s_player, 0.f, 0.f, 0.f, 6, 80.f, 0.f, -1.f, 2600.f, 40.f); // atacar
    j.born = g_st.now;
    j.nextSwap = g_st.now + 5000;
    s_jet = j;
    s_v.push_back({j.v, g_st.now, false});
    Log("  JATO (%s) chegando a %.0fm", m == H("hydra") ? "Hydra" : "Lazer", D2(p, s_ppos));
}

// ---------- v10: evento raro - Titan passa alto e solta paraquedistas ----------
// So com poucos inimigos vivos, o jogador a pe e a partir do nivel 7. Fica no lugar da wave normal.
struct ParaSt { Vehicle plane = 0; Ped pilot = 0; int at = 0; bool dropped = false; };
static ParaSt s_para;
static int s_paraCdUntil = 0;
static bool ParaActive() {
    if (s_para.plane) return true;
    for (auto &p : s_q) if (p.k == P_PARA) return true;
    return false;
}
static void SpawnParadrop() {
    Hash m = H("titan");
    if (!STREAMING::IS_MODEL_IN_CDIMAGE(m)) { Log("  paraquedistas: Titan nao encontrado."); return; }
    float camH = CAM::GET_GAMEPLAY_CAM_ROT(2).z, dx, dy;
    Dir(camH + 180.f + RandF(-50.f, 50.f), dx, dy);
    float base = s_ppos.z > s_pgz ? s_ppos.z : s_pgz;
    Vector3 p = V3(s_ppos.x + dx * 1100.f, s_ppos.y + dy * 1100.f, base + 320.f);
    ParaSt ps;
    ps.plane = NewVehicle(m, p, HeadingTo(p, s_ppos), false);
    if (!ps.plane) { Log("  paraquedistas: falha ao criar o Titan."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(ps.plane, TRUE, TRUE, FALSE);
    VEHICLE::_SET_VEHICLE_LANDING_GEAR(ps.plane, 3);
    VEHICLE::SET_VEHICLE_FORWARD_SPEED(ps.plane, 60.f);
    ps.pilot = NewPedIn(ps.plane, RandomPed(), -1);
    if (!ps.pilot) { VEHICLE::DELETE_VEHICLE(&ps.plane); return; }
    PED::SET_PED_RELATIONSHIP_GROUP_HASH(ps.pilot, s_fac->group);
    PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ps.pilot, TRUE);
    ENTITY::SET_ENTITY_INVINCIBLE(ps.plane, TRUE); // so um transporte: passa e vai embora
    // passa por cima do jogador e segue reto ate bem longe
    Vector3 farPt = V3(s_ppos.x - dx * 2500.f, s_ppos.y - dy * 2500.f, base + 330.f);
    AI::TASK_PLANE_MISSION(ps.pilot, ps.plane, 0, 0, farPt.x, farPt.y, farPt.z, 4, 60.f, 0.f, -1.f, 2600.f, 300.f);
    ps.at = g_st.now;
    s_para = ps;
    Log("  EVENTO: Titan com paraquedistas (%s) chegando a %.0fm, %.0fm de altura", s_fac->name, D2(p, s_ppos), p.z - base);
}
static void UpdateParadrop() {
    ParaSt &ps = s_para;
    if (!ps.plane) return;
    bool alive = ENTITY::DOES_ENTITY_EXIST(ps.plane) && ENTITY::DOES_ENTITY_EXIST(ps.pilot);
    Vector3 pp = alive ? ENTITY::GET_ENTITY_COORDS(ps.plane, TRUE) : V3(0, 0, 0);
    if (!alive || (ps.dropped && D2(pp, s_ppos) > 1800.f) || g_st.now - ps.at > 120000) {
        if (ENTITY::DOES_ENTITY_EXIST(ps.pilot)) PED::DELETE_PED(&ps.pilot);
        if (ENTITY::DOES_ENTITY_EXIST(ps.plane)) VEHICLE::DELETE_VEHICLE(&ps.plane);
        ps = ParaSt{};
        return;
    }
    if (!ps.dropped && (D2(pp, s_ppos) < 160.f || g_st.now - ps.at > 45000)) {
        ps.dropped = true;
        int n = 4 + (s_level >= 8 ? 2 : 0) + Rand(3); // 4-6 no nivel 7, 6-8 no nivel 8
        int made = 0;
        float fx, fy;
        Dir(ENTITY::GET_ENTITY_HEADING(ps.plane), fx, fy);
        for (int i = 0; i < n && Budget() > 0; ++i) {
            Vector3 q = V3(pp.x - fx * (12.f + i * 7.f) + RandF(-4.f, 4.f), pp.y - fy * (12.f + i * 7.f) + RandF(-4.f, 4.f),
                           pp.z - 12.f);
            Ped ped = NewPedAt(RandomPed(), q, ENTITY::GET_ENTITY_HEADING(ps.plane));
            if (!ped) continue;
            SetupPed(ped, s_level, WeaponFor(s_level, BetterPct()));
            WEAPON::GIVE_WEAPON_TO_PED(ped, H("GADGET_PARACHUTE"), 1, FALSE, FALSE);
            PED::SET_PED_PARACHUTE_TINT_INDEX(ped, Rand(8));
            float a = RandF(0.f, 360.f), r = RandF(20.f, 60.f), tx, ty;
            Dir(a, tx, ty);
            AI::TASK_PARACHUTE_TO_TARGET(ped, s_ppos.x + tx * r, s_ppos.y + ty * r, s_pgz);
            Hunter &h = AddHunter(ped);
            h.para = true;
            h.reached = true; // nao conta o tempo de chegada
            ++made;
        }
        s_paraCdUntil = g_st.now + g_cfg.paraCooldown * 1000;
        Log("  paraquedistas: %d saltaram do Titan.", made);
    }
}

// ---------- v10: jogador pilotando aviao: so aeronaves perseguidoras armadas ----------
// Niveis baixos: avioes leves so com metralhadora. Niveis altos: cacas com metralhadora e misseis.
// Cada um ataca por um tempo (Jet Duration) e vai embora; o limite ao mesmo tempo sobe com o nivel.
struct AirSt { Vehicle v = 0; Ped pilot = 0; int born = 0, nextSwap = 0, wi = 0; bool leaving = false, mgOnly = true; };
static std::vector<AirSt> s_air;
static const std::vector<const char *> kAirTier[4] = {
    {"microlight", "seabreeze"},                                  // niveis 1-2: metralhadora
    {"seabreeze", "rogue", "starling"},                           // niveis 3-4: metralhadora
    {"rogue", "starling", "nokota", "molotok"},                   // niveis 5-6: metralhadora e misseis
    {"lazer", "hydra", "pyro", "molotok", "strikeforce"},         // niveis 7-8: tudo
};
static const char *kAirWeaps[] = {
    "VEHICLE_WEAPON_MICROLIGHT_MG", "VEHICLE_WEAPON_SEABREEZE_MG", "VEHICLE_WEAPON_ROGUE_MG",
    "VEHICLE_WEAPON_STARLING_MG",   "VEHICLE_WEAPON_NOKOTA_MG",    "VEHICLE_WEAPON_MOLOTOK_MG",
    "VEHICLE_WEAPON_PYRO_MG",       "VEHICLE_WEAPON_PLAYER_LAZER", "VEHICLE_WEAPON_STRIKEFORCE_CANNON",
    "VEHICLE_WEAPON_ROGUE_MISSILE", "VEHICLE_WEAPON_STARLING_MISSILE", "VEHICLE_WEAPON_NOKOTA_MISSILE",
    "VEHICLE_WEAPON_MOLOTOK_MISSILE", "VEHICLE_WEAPON_PYRO_MISSILE", "VEHICLE_WEAPON_PLANE_ROCKET",
    "VEHICLE_WEAPON_SPACE_ROCKET",  "VEHICLE_WEAPON_STRIKEFORCE_MISSILE", "VEHICLE_WEAPON_STRIKEFORCE_BARRAGE"};
static bool PlayerInPlane() {
    if (!PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE)) return false;
    return VEHICLE::IS_THIS_MODEL_A_PLANE(ENTITY::GET_ENTITY_MODEL(PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE))) != 0;
}
static int AirLimit() { return s_level <= 2 ? 1 : s_level <= 4 ? 2 : s_level <= 6 ? 2 : 3; }
static int AirQueued() { int n = 0; for (auto &q : s_q) if (q.k == P_AIR) ++n; return n; }
static void AirWeapon(AirSt &a) {
    const int n = sizeof kAirWeaps / sizeof *kAirWeaps;
    for (int t = 0; t < n; ++t) {
        a.wi = (a.wi + 1) % n;
        const char *w = kAirWeaps[a.wi];
        bool mg = strstr(w, "_MG") || strstr(w, "LAZER") || strstr(w, "CANNON");
        if (a.mgOnly && !mg) continue;
        if (WEAPON::SET_CURRENT_PED_VEHICLE_WEAPON(a.pilot, H(w))) return;
    }
}
static void SpawnAirChaser() {
    int tier = s_level <= 2 ? 0 : s_level <= 4 ? 1 : s_level <= 6 ? 2 : 3;
    std::vector<const char *> models;
    for (const char *n : kAirTier[tier]) if (STREAMING::IS_MODEL_IN_CDIMAGE(H(n))) models.push_back(n);
    if (models.empty()) { Log("  aviao perseguidor: nenhum modelo encontrado."); return; }
    const char *mname = models[Rand((int)models.size())];
    Hash m = H(mname);
    // nasce atras do jogador, na mesma altura, ja voando na velocidade dele
    Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE);
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(pv);
    float sp = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z), fx, fy;
    float ph = ENTITY::GET_ENTITY_HEADING(pv);
    Dir(ph, fx, fy);
    float d = RandF(450.f, 700.f), side = RandF(-120.f, 120.f);
    Vector3 p = V3(s_ppos.x - fx * d + fy * side, s_ppos.y - fy * d - fx * side, s_ppos.z + RandF(-15.f, 40.f));
    float gz = 0.f;
    if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(p.x, p.y, p.z + 200.f, &gz, FALSE) && p.z < gz + 60.f) p.z = gz + 60.f;
    AirSt a;
    a.v = NewVehicle(m, p, ph, false);
    if (!a.v) { Log("  aviao perseguidor: falha ao criar."); return; }
    VEHICLE::SET_VEHICLE_ENGINE_ON(a.v, TRUE, TRUE, FALSE);
    VEHICLE::_SET_VEHICLE_LANDING_GEAR(a.v, 3);
    VEHICLE::SET_VEHICLE_FORWARD_SPEED(a.v, sp > 30.f ? sp : 50.f);
    a.pilot = NewPedIn(a.v, RandomPed(), -1);
    if (!a.pilot) { VEHICLE::DELETE_VEHICLE(&a.v); return; }
    SetupPed(a.pilot, s_level, WeaponFor(s_level));
    PoorAim(a.pilot, tier >= 2 ? g_cfg.explosiveAcc : g_cfg.heliAccuracy); // tiros e misseis espalhados
    PED::SET_PED_COMBAT_ATTRIBUTES(a.pilot, 3, FALSE);
    PED::SET_PED_COMBAT_ATTRIBUTES(a.pilot, 52, TRUE);
    PED::SET_PED_COMBAT_ATTRIBUTES(a.pilot, 53, TRUE);
    AddCrew(a.pilot);
    a.mgOnly = tier <= 1;
    AirWeapon(a);
    AI::TASK_PLANE_MISSION(a.pilot, a.v, 0, s_player, 0.f, 0.f, 0.f, 6, 80.f, 0.f, -1.f, 2600.f, 40.f); // atacar
    a.born = g_st.now;
    a.nextSwap = g_st.now + 6000;
    s_air.push_back(a);
    s_v.push_back({a.v, g_st.now, false});
    Log("  AVIAO PERSEGUIDOR (%s, %s) a %.0fm", mname, a.mgOnly ? "metralhadora" : "metralhadora e misseis",
        D2(p, s_ppos));
}
static void UpdateAir() {
    for (size_t i = 0; i < s_air.size();) {
        AirSt &a = s_air[i];
        if (!ENTITY::DOES_ENTITY_EXIST(a.v) || ENTITY::IS_ENTITY_DEAD(a.v) || !ENTITY::DOES_ENTITY_EXIST(a.pilot) ||
            PED::IS_PED_DEAD_OR_DYING(a.pilot, TRUE)) {
            Log("Aviao perseguidor abatido ou fora de combate.");
            s_air.erase(s_air.begin() + i);
            continue;
        }
        if (!a.leaving && g_st.now >= a.nextSwap && !a.mgOnly) { AirWeapon(a); a.nextSwap = g_st.now + 5000 + Rand(3000); }
        if (!a.leaving && g_st.now - a.born > g_cfg.jetDuration * 1000) {
            Vector3 jp = ENTITY::GET_ENTITY_COORDS(a.v, TRUE);
            float ax = jp.x - s_ppos.x, ay = jp.y - s_ppos.y, al = std::hypot(ax, ay) + 0.01f;
            AI::TASK_PLANE_MISSION(a.pilot, a.v, 0, 0, jp.x + ax / al * 3000.f, jp.y + ay / al * 3000.f, jp.z + 300.f, 4,
                                   80.f, 0.f, -1.f, 2600.f, 200.f);
            a.leaving = true;
        }
        ++i;
    }
}

static void UpdateJet() {
    JetSt &j = s_jet;
    if (!j.v) return;
    if (!ENTITY::DOES_ENTITY_EXIST(j.v) || ENTITY::IS_ENTITY_DEAD(j.v) || !ENTITY::DOES_ENTITY_EXIST(j.pilot) ||
        PED::IS_PED_DEAD_OR_DYING(j.pilot, TRUE)) {
        bool shot = (ENTITY::DOES_ENTITY_EXIST(j.v) && ENTITY::IS_ENTITY_DEAD(j.v)) ||
                    (ENTITY::DOES_ENTITY_EXIST(j.pilot) && PED::IS_PED_DEAD_OR_DYING(j.pilot, TRUE));
        s_jetCooldownUntil = g_st.now + g_cfg.jetCooldown * 1000;
        Log("Jato %s; proximo caca so daqui a %ds.", shot ? "abatido" : "fora de combate", g_cfg.jetCooldown);
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
    // v11: na Desordem nao vem jato, aviao perseguidor nem tanque
    if (g_cfg.mode == 4 && (k == P_TANK || k == P_KHAN || k == P_JET || k == P_AIR)) return;
    // v9: cada unidade sai da fila com espacamento da anterior, em fluxo continuo entre as waves
    int at = g_st.now + delayMs;
    if (at < s_flowNext) at = s_flowNext;
    s_flowNext = at + FlowGapMs();
    s_q.push_back({at, k, n, role, bicycleOk});
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

    // v10: evento raro (nivel 7+): poucos inimigos vivos e jogador a pe -> Titan com paraquedistas no lugar da wave
    if (PlayerOnFootGround() && s_level >= g_cfg.paraMinLevel && HuntersAlive() <= g_cfg.paraFewAlive &&
        !ParaActive() && g_st.now >= s_paraCdUntil && free >= 4 && Rand(100) < g_cfg.paraChance) {
        Queue(1000, P_PARA);
        Log("  evento: lancamento de paraquedistas no lugar da wave.");
        return;
    }
    int heliCap = HeliLimit(air) - (int)s_helis.size() - HelisQueued();

    // v10.1: em mar aberto (de barco ou nadando): so helicopteros de ataque e barcos inimigos
    if (PlayerAtSea()) {
        Log("  jogador em mar aberto: so helicopteros de ataque e barcos.");
        if (s_level >= 3 && heliCap > 0 && free >= 4 && Rand(100) < NextHeliPct(HelisNow()) + 15) {
            Queue(3000, P_HELI, 0, 2);
            free -= 4;
        }
        if (free >= 2) { Queue(1000, P_BOAT); free -= 2; }
        if (free >= 2 && L.vehicles >= 2) Queue(6000, P_BOAT);
        return;
    }

    // v6: jato (so no ultimo nivel, um por vez; mais chance se o jogador estiver voando)
    if (g_cfg.jetEnabled && s_level >= g_cfg.jetMinLevel && !JetActive() && g_st.now >= s_jetCooldownUntil && free >= 1 &&
        Rand(100) < Pct(g_cfg.jetChance, (bonus + EscBonus() * 0.5f) * 0.3f, g_cfg.jetMax) + (air ? 5 : 0)) {
        Queue(2000, P_JET);
        free -= 1;
    }

    // v10: jogador num aviao: so aeronaves perseguidoras armadas, conforme o nivel
    if (PlayerInPlane()) {
        int want = AirLimit() - (int)s_air.size() - AirQueued();
        for (int i = 0; i < want && free > 0; ++i, --free) Queue(1500 + i * 6000, P_AIR);
        Log("  jogador num aviao: so avioes perseguidores (%d no ar, limite %d).", (int)s_air.size(), AirLimit());
        return;
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
    // v9: longe de qualquer estrada (mato, montanha) -> mais helicopteros, e qualquer faccao pode usar um
    bool remote = FarFromRoad();
    bool hasHeli = !s_fac->helis.empty() || !s_fac->attackHelis.empty() || (remote && s_level >= 2);
    if (remote) Log("  jogador longe das estradas: mais chance de helicoptero.");
    // v9: agua por perto -> barcos tambem vem pela agua (ate quem nao tem barco usa um bote)
    if (!boat && s_level >= 3 && free >= 2 && Rand(100) < g_cfg.waterBoatPct && WaterNearby()) {
        Queue(2000, P_BOAT);
        free -= 2;
        Log("  agua por perto: barco a caminho.");
    }
    // cada helicoptero a mais no ar tem a sua chance: 1o normal, 2o baixa, 3o bem rara (so no nivel 8)
    if (hasHeli && heliCap > 0 && free >= 4) {
        int n = HelisNow();
        if (Rand(100) < NextHeliPct(n) + (remote && n == 0 ? g_cfg.remoteHeliBonus : 0)) {
            Queue(4000, P_HELI);
            free -= 4;
            if (n == 0 && heliCap > 1 && free >= 4 && Rand(100) < NextHeliPct(1)) { Queue(12000, P_HELI); free -= 4; }
        }
    }

    // v10: tanque especial Khanjali, por terra: chance fixa, so do inicio do nivel 8 ate o maximo
    if (!boat && g_cfg.tankEnabled && s_level >= 8 && !g_cfg.levelStart.empty() && g_st.Pts() >= g_cfg.levelStart.back() &&
        !KhanActive() && g_st.now >= s_khanCdUntil && free >= 1 && Rand(100) < g_cfg.khanChance) {
        Queue(5000, P_KHAN);
        free -= 1;
    }

    // v6: tanque no Cargobob (so no ultimo nivel, um por vez, com o jogador no chao)
    if (!boat && g_cfg.tankEnabled && s_level >= g_cfg.tankMinLevel && !TankActive() && free >= 2 &&
        g_st.now >= s_tankCdUntil && // v10: espera de 1 min e escalada menor
        Rand(100) < Pct(g_cfg.tankChance, (bonus + EscBonus() * 0.5f) * 0.25f, g_cfg.tankMax)) {
        Queue(3000, P_TANK);
        free -= 2;
    }

    if (boat) {
        if (s_level >= 4 && !s_fac->boats.empty()) { Queue(1000, P_BOAT); if (L.vehicles >= 2) Queue(6000, P_BOAT); }
        else Log("  jogador de barco: esta faccao nao tem barcos neste nivel.");
        return;
    }

    // v10.1: o pesado (nivel 6+) ou o leve da faccao (niveis 3-6)
    bool jugHeavy = g_cfg.jugEnabled && HeavyJugOk();
    int jugPct = jugHeavy ? Pct(g_cfg.jugChance, (bonus + EscBonus()) * 0.4f, g_cfg.jugMax)
                          : Pct(g_cfg.jugLightChance, (bonus + EscBonus()) * 0.4f, g_cfg.jugLightMax);
    if ((jugHeavy || LightJugOk()) && !JuggAlive() && free >= 4 && Rand(100) < jugPct) {
        Queue(2500, P_JUGG);
        free -= 4;
    }

    if (inVeh) {
        // v10: correndo na estrada = perseguicao: bloqueio bem mais provavel e um perseguidor a mais
        bool fast = PlayerFastInCar();
        if (fast) Log("  jogador correndo de carro: perseguidores atras e bloqueio a frente.");
        if (g_cfg.rbEnabled && s_level >= g_cfg.rbMinLevel && free >= 4 &&
            Rand(100) < g_cfg.rbChance + bonus + (fast ? 35 : 0)) {
            Queue(0, P_ROADBLOCK);
            free -= 4;
        }
        for (int i = 0; i < L.vehicles + 1 + (fast ? 1 : 0) && free > 0; ++i, free -= 2) Queue(1500 + i * 4000, P_VEH);
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
        // v9: muita gente perto do jogador? os proximos esperam na fila (helicopteros, caca, tanque e bloqueio nao)
        PKind kk = s_q[i].k;
        if ((kk == P_FOOT || kk == P_VEH || kk == P_BOAT || kk == P_JUGG) && NearCount() >= NearMax()) {
            s_q[i].at = g_st.now + 3000;
            return;
        }
        // v10: so sai da fila se a unidade inteira couber no limite (teto de 30 somando tudo)
        static const int kCost[] = {0, 2, 2, 4, 4, 3, 2, 1, 1, 1, 4}; // FOOT(n) VEH BOAT JUGG RB HELI TANK JET KHAN AIR PARA
        int cost = kk == P_FOOT ? s_q[i].n : kCost[kk];
        if (Budget() < cost) {
            if (Budget() <= 0) return;
            continue; // espera vaga; tenta a proxima unidade da fila
        }
        Pending p = s_q[i];
        s_q.erase(s_q.begin() + i);
        switch (p.k) {
        case P_FOOT: SpawnFoot(p.n, p.role); break;
        case P_VEH: SpawnVehicle(p.bicycleOk); break;
        case P_BOAT: SpawnBoat(); break;
        case P_JUGG: SpawnJuggernaut(); break;
        case P_HELI: SpawnHeli(p.role == 1, p.role == 2); break;
        case P_TANK: if (s_tank.state < 0) SpawnTank(); break;
        case P_KHAN: if (!s_khan.v) SpawnKhanjali(); break;
        case P_JET: if (!s_jet.v) SpawnJet(); break;
        case P_AIR: if (PlayerInPlane()) SpawnAirChaser(); break;
        case P_PARA: if (!s_para.plane) SpawnParadrop(); break;
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
    if (s.blip) { UI::REMOVE_BLIP(&s.blip); s.blip = 0; }
    if (!del) VehAmmoRestore(s.veh);
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
    s_khan = KhanSt{};
    s_jet = JetSt{};
    s_air.clear();
    if (ENTITY::DOES_ENTITY_EXIST(s_para.pilot)) PED::DELETE_PED(&s_para.pilot);
    if (ENTITY::DOES_ENTITY_EXIST(s_para.plane)) VEHICLE::DELETE_VEHICLE(&s_para.plane);
    s_para = ParaSt{};
    s_nextWave = 0;
    if (n) Log("Retaliadores removidos (%s): %d.", why, n);
}

// v10: jogador fora das estradas (mato, montanha, campo) e fora de interiores: inimigos a pe ficam agressivos
static const float kWildRoadDist = 60.f;
static int s_wildCheck = 0;
static void UpdateWild() {
    if (g_st.now < s_wildCheck) return;
    s_wildCheck = g_st.now + 2000;
    bool wild = false;
    if (!PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) && INTERIOR::GET_INTERIOR_FROM_ENTITY(s_player) == 0) {
        Vector3 n;
        wild = !PATHFIND::GET_CLOSEST_VEHICLE_NODE(s_ppos.x, s_ppos.y, s_pgz, &n, 1, 3.0f, 0.f) ||
               D2(n, s_ppos) > kWildRoadDist;
    }
    if (wild != s_wild) Log(wild ? "Jogador fora das estradas: inimigos a pe avancam (IA agressiva)."
                                 : "Jogador perto das estradas: IA normal.");
    s_wild = wild;
}
static void SetWild(Hunter &h, bool on) {
    if (h.wild == on) return;
    h.wild = on;
    if (on) {
        PED::REMOVE_PED_DEFENSIVE_AREA(h.ped, FALSE);
        PED::SET_PED_COMBAT_MOVEMENT(h.ped, 2);            // avanca
        PED::SET_PED_COMBAT_RANGE(h.ped, 0);               // luta de perto
        PED::SET_PED_COMBAT_ATTRIBUTES(h.ped, 50, TRUE);   // pode avancar de frente
        PED::SET_PED_COMBAT_ATTRIBUTES(h.ped, 46, TRUE);
        h.nextPush = 0;
        h.walking = false; // quem vinha andando da estrada passa a avancar direto
    } else {
        h.pushing = false;
        PED::SET_PED_COMBAT_MOVEMENT(h.ped, h.tactical ? (Rand(100) < 70 ? 1 : 2) : 2); // como no SetupPed
        RangeClass rc = RangeOf(WEAPON::GET_SELECTED_PED_WEAPON(h.ped));
        PED::SET_PED_COMBAT_RANGE(h.ped, rc == RC_SHORT ? 0 : rc == RC_MEDIUM ? 1 : 2);
        if (h.tactical) PED::SET_PED_COMBAT_ATTRIBUTES(h.ped, 50, FALSE);
        Engage(h.ped);
    }
}
// avanca correndo e atirando ate ~20 m do jogador; ai luta normalmente (ofensivo)
static void WildPush(Hunter &h, float d) {
    if (g_st.now < h.nextPush) return;
    h.nextPush = g_st.now + 3500;
    if (d > 22.f) {
        AI::TASK_GO_TO_ENTITY_WHILE_AIMING_AT_ENTITY(h.ped, s_player, s_player, 2.0f, TRUE, 15.f, 6.f, TRUE, FALSE,
                                                     0xC6EE6B4C);
        h.pushing = true;
    } else if (h.pushing) {
        h.pushing = false;
        Engage(h.ped);
    }
}

static void UpdateHunters() {
    UpdateWild();
    bool onFoot = !PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE);
    float br = BlipRadius();
    for (size_t i = 0; i < s_h.size();) {
        Hunter &h = s_h[i];
        if (!ENTITY::DOES_ENTITY_EXIST(h.ped)) { RemoveBlip(h); s_h.erase(s_h.begin() + i); continue; }
        if (PED::IS_PED_DEAD_OR_DYING(h.ped, TRUE)) {
            if (h.juggernaut) {
                Log(h.lightJug ? "Heavy abatido!" : "Juggernaut abatido!");
                // v10.1: com o ragdoll desligado o corpo ficava travado/duro; liga e deixa cair
                PED::SET_PED_CAN_RAGDOLL(h.ped, TRUE);
                PED::SET_PED_TO_RAGDOLL(h.ped, 3000, 5000, 0, TRUE, TRUE, FALSE);
            }
            // v7: retaliador morto pelo jogador da pontos
            Entity k = PED::_GET_PED_KILLER(h.ped);
            Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            if (k && (k == s_player || (pv && k == pv))) AddPoints(g_cfg.killHunter, "retaliador morto");
            else if (DisorderOn() && D2(ENTITY::GET_ENTITY_COORDS(h.ped, FALSE), s_ppos) < g_cfg.disDeathRadius)
                AddPoints(g_cfg.disDeathPts, "retaliador morto na desordem"); // v11
            ReleasePed(h, false);
            s_h.erase(s_h.begin() + i);
            continue;
        }
        // v10.1: Juggernaut quase morto ja pode cair, para a morte sair natural
        if (h.juggernaut && !h.lightJug && !h.ragOk && ENTITY::GET_ENTITY_HEALTH(h.ped) < 350) {
            PED::SET_PED_CAN_RAGDOLL(h.ped, TRUE);
            h.ragOk = true;
        }
        Vector3 hp = ENTITY::GET_ENTITY_COORDS(h.ped, TRUE);
        float d = D2(hp, s_ppos);
        // v11: Desordem - o jogador feriu um retaliador: os retaliadores ficam contra ele
        if (g_cfg.mode == 4) {
            int life = ENTITY::GET_ENTITY_HEALTH(h.ped) + PED::GET_PED_ARMOUR(h.ped);
            Vehicle pv0 = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            if (h.lastHp >= 0 && life < h.lastHp &&
                (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(h.ped, s_player, TRUE) ||
                 (pv0 && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(h.ped, pv0, TRUE)))) {
                DisorderProvoke(false);
                ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(h.ped);
            }
            h.lastHp = life;
        }

        // blip vermelho so perto do jogador. v10: dentro de veiculo aparece o icone do veiculo
        bool pedInVeh = PED::IS_PED_IN_ANY_VEHICLE(h.ped, FALSE);
        if (h.blip && pedInVeh) RemoveBlip(h);
        if (!h.blip && d < br && !pedInVeh) {
            h.blip = UI::ADD_BLIP_FOR_ENTITY(h.ped);
            if (h.juggernaut && g_cfg.jugBlip > 0) UI::SET_BLIP_SPRITE(h.blip, g_cfg.jugBlip); // v10: caveira
            UI::SET_BLIP_COLOUR(h.blip, 1);
            // v10.1: Heavy com a caveira do tamanho antigo; o Juggernaut com a caveira maior
            UI::SET_BLIP_SCALE(h.blip, h.lightJug ? 1.0f : h.juggernaut ? 1.4f : 0.7f);
        } else if (h.blip && d > br + 15.f) {
            RemoveBlip(h);
        }

        if (h.para) { // v10: descendo de paraquedas
            bool landed = !PED::IS_PED_IN_PARACHUTE_FREE_FALL(h.ped) && PED::GET_PED_PARACHUTE_STATE(h.ped) <= 0 &&
                          ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(h.ped) < 2.5f;
            if (landed || g_st.now - h.born > 60000) {
                h.para = false;
                Engage(h.ped);
            } else { ++i; continue; }
        }
        if (d < 70.f) h.reached = true;
        // v11: Desordem - escolhe o alvo de novo quando muda a situacao (perto/longe do jogador, ou ele atacou)
        if (DisorderOn() && !h.walking && !h.leaving && !h.heliRappel && !h.tankGunner) {
            Vehicle hv0 = PED::GET_VEHICLE_PED_IS_IN(h.ped, FALSE);
            Hash hm0 = hv0 ? ENTITY::GET_ENTITY_MODEL(hv0) : 0;
            bool air0 = hv0 && (VEHICLE::IS_THIS_MODEL_A_HELI(hm0) || VEHICLE::IS_THIS_MODEL_A_PLANE(hm0));
            bool driver = hv0 && VEHICLE::GET_PED_IN_VEHICLE_SEAT(hv0, -1) == h.ped;
            int want = (!Calm() || d < g_cfg.disNearDist) ? 1 : 0;
            if (!(air0 && driver)) { // pilotos ficam com o controle do helicoptero
                if (want != h.onPlayer) {
                    h.onPlayer = want;
                    if (driver) Chase(h.ped); else Engage(h.ped);
                    h.nextChase = g_st.now + 8000;
                } else if (driver && !want && g_st.now >= h.nextChase) { // renova o destino (o jogador anda)
                    h.nextChase = g_st.now + 8000;
                    Chase(h.ped);
                }
            }
        }
        if (h.walking && (d < 80.f || g_st.now - h.born > 90000)) { // chegou perto: comeca a lutar
            h.walking = false;
            Engage(h.ped);
        }

        // v5: so atira dentro do alcance da arma.
        // v10: e so depois de aparecer no minimapa (dentro do raio do blip). Aeronaves sao a excecao.
        if (!h.tankGunner) {
            Vehicle hv = PED::GET_VEHICLE_PED_IS_IN(h.ped, FALSE);
            Hash hm = hv ? ENTITY::GET_ENTITY_MODEL(hv) : 0;
            bool air = hv && (VEHICLE::IS_THIS_MODEL_A_HELI(hm) || VEHICLE::IS_THIS_MODEL_A_PLANE(hm));
            // v10: limite por situacao: aeronave, veiculo (motorista/passageiro/torreta) ou a pe pela arma
            float r = air ? g_cfg.rangeAir
                      : hv ? g_cfg.rangeVehicle
                           : RangeMeters(RangeOf(WEAPON::GET_SELECTED_PED_WEAPON(h.ped)));
            if (!air && r > br) r = br; // e nunca antes de aparecer no minimapa
            bool ok = d <= r || Calm(); // v11: na Desordem calma o alvo nao e o jogador
            int want = ok ? 100 : 0;
            if (want != h.shootRate) { PED::SET_PED_SHOOT_RATE(h.ped, want); h.shootRate = want; }
            // a pe e fora do alcance: corre ate o jogador em vez de ficar atirando de longe
            bool free = !hv && !h.roadblock && !h.wild && !h.walking && !h.leaving && !h.heliRappel;
            if (free && !ok && d > r + 8.f && g_st.now >= h.nextApproach) {
                AI::TASK_GO_TO_ENTITY(h.ped, s_player, -1, r * 0.6f, 2.0f, 0, 0);
                h.approaching = true;
                h.nextApproach = g_st.now + 4000;
            } else if (h.approaching && (ok || !free)) {
                h.approaching = false;
                if (free) Engage(h.ped);
            }
        }
        bool timeout = !h.reached && !h.roadblock && g_st.now - h.born > (g_cfg.arriveTimeout + (h.farOk ? 120 : 0)) * 1000; // v10: quem vem de longe tem +2 min
        bool rbOld = h.roadblock && g_st.now - h.born > (g_cfg.arriveTimeout + 60) * 1000 && d > 150.f;
        if (d > (h.farOk ? 1500.f : g_cfg.despawnDist) || timeout || rbOld) {
            ReleasePed(h, !ENTITY::IS_ENTITY_ON_SCREEN(h.ped) || d > 200.f);
            s_h.erase(s_h.begin() + i);
            continue;
        }

        // descer do veiculo perto do jogador a pe
        bool inVeh = PED::IS_PED_IN_ANY_VEHICLE(h.ped, FALSE);
        if (inVeh && !h.stayInVeh && !h.leaving && (Calm() ? d < 60.f : onFoot && d < 45.f)) { // v11: na Desordem desce na area
            AI::TASK_LEAVE_VEHICLE(h.ped, PED::GET_VEHICLE_PED_IS_IN(h.ped, FALSE), 0);
            h.leaving = true;
            h.leftAt = g_st.now;
        } else if (h.leaving && (!inVeh || g_st.now - h.leftAt > 4000)) {
            h.leaving = false;
            if (h.juggernaut && !h.lightJug) ApplyJuggernautWalk(h.ped);
            Engage(h.ped);
        }

        // v10: fora das estradas, quem esta a pe avanca ate o jogador (menos snipers e quem ainda vem andando da estrada)
        bool canWild = !inVeh && !h.leaving && !h.sniper && !h.heliRappel;
        SetWild(h, s_wild && canWild && !Calm());
        if (h.wild) WildPush(h, d);

        // v10: sem visao do jogador (atras de parede/objeto e fora de cobertura): procura um caminho ate ele
        // e volta a atacar quando o enxergar de novo. Em cobertura, os inimigos mantem o comportamento normal.
        if (!Calm() && !inVeh && !h.leaving && !h.sniper && !h.heliRappel && !h.roadblock && !h.walking && !h.wild &&
            !h.approaching && d < 120.f && g_st.now >= h.nextLos) {
            h.nextLos = g_st.now + 900 + Rand(300);
            bool sees = ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(h.ped, s_player, 17) != 0;
            bool cover = PED::IS_PED_IN_COVER(s_player, FALSE) != 0;
            if (sees || cover) {
                h.losLostAt = 0;
                if (h.seeking) { h.seeking = false; Engage(h.ped); }
            } else {
                if (!h.losLostAt) h.losLostAt = g_st.now;
                if (g_st.now - h.losLostAt > 3000) {
                    // vai ate o jogador pelo caminho do jogo, mirando nele (atira assim que tiver visao)
                    AI::TASK_GO_TO_ENTITY_WHILE_AIMING_AT_ENTITY(h.ped, s_player, s_player, 2.0f, TRUE, 4.f, 2.f, TRUE,
                                                                 FALSE, 0xC6EE6B4C);
                    h.seeking = true;
                    h.losLostAt = g_st.now; // renova o caminho a cada ~3 s enquanto nao enxergar
                }
            }
        } else if (h.seeking && (inVeh || h.wild || h.approaching || Calm())) {
            h.seeking = false;
        }

        // posicionamento tatico: area defensiva em volta do jogador, a media distancia, com cobertura
        if (!h.wild && h.tactical && !inVeh && !h.roadblock && !h.sniper && !h.juggernaut && g_st.now >= h.nextArea) {
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
        if (!ENTITY::DOES_ENTITY_EXIST(s.veh)) {
            if (s.blip) UI::REMOVE_BLIP(&s.blip);
            s_v.erase(s_v.begin() + i);
            continue;
        }
        // v11: Desordem - o jogador atacou um veiculo dos retaliadores
        if (g_cfg.mode == 4 && !ENTITY::IS_ENTITY_DEAD(s.veh)) {
            int life = ENTITY::GET_ENTITY_HEALTH(s.veh);
            Vehicle pv0 = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            if (s.lastHp >= 0 && life < s.lastHp &&
                (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, s_player, TRUE) ||
                 (pv0 && pv0 != s.veh && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, pv0, TRUE)))) {
                DisorderProvoke(false);
                ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(s.veh);
            }
            s.lastHp = life;
        }
        // v7: veiculo/aeronave dos retaliadores destruido pelo jogador tambem da pontos
        if (!s.scored && ENTITY::IS_ENTITY_DEAD(s.veh)) {
            s.scored = true;
            Vehicle pv = PED::GET_VEHICLE_PED_IS_IN(s_player, TRUE);
            bool near0 = D2(ENTITY::GET_ENTITY_COORDS(s.veh, FALSE), s_ppos) < g_cfg.disDeathRadius;
            if (DisorderOn() && near0 && !ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, s_player, TRUE) &&
                !(pv && pv != s.veh && ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, pv, TRUE)))
                AddPoints(g_cfg.disVehPts, "veiculo dos retaliadores destruido na desordem"); // v11
            else if (ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(s.veh, s_player, TRUE) ||
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
        // v10: icone do veiculo enquanto tem retaliador dentro e esta no raio do blip
        {
            bool crew = false;
            for (Hunter &h : s_h)
                if (!PED::IS_PED_DEAD_OR_DYING(h.ped, TRUE) && PED::IS_PED_IN_VEHICLE(h.ped, s.veh, FALSE)) { crew = true; break; }
            int sp0 = VehBlipSprite(s.veh);
            float vr = BlipRadius() * (sp0 == 353 || sp0 == 424 ? 2.5f : 1.f); // aeronaves atiram de longe: aparecem antes
            bool show = crew && !ENTITY::IS_ENTITY_DEAD(s.veh) && d < vr && !drop;
            if (show && !s.blip) {
                s.blip = UI::ADD_BLIP_FOR_ENTITY(s.veh);
                int sp = sp0;
                UI::SET_BLIP_SPRITE(s.blip, sp);
                UI::SET_BLIP_COLOUR(s.blip, 1);
                UI::SET_BLIP_SCALE(s.blip, sp == 421 || sp == 424 || sp == 353 ? 1.0f : 0.85f);
            } else if (!show && s.blip && (!crew || d > vr + 15.f || drop || ENTITY::IS_ENTITY_DEAD(s.veh))) {
                UI::REMOVE_BLIP(&s.blip);
                s.blip = 0;
            }
            if (s.blip && sp0 != 353)
                UI::SET_BLIP_ROTATION(s.blip, (int)ENTITY::GET_ENTITY_HEADING(s.veh)); // aponta para onde vai
        }
        if (drop) {
            ReleaseVeh(s, d > 120.f && !ENTITY::IS_ENTITY_ON_SCREEN(s.veh));
            s_v.erase(s_v.begin() + i);
            continue;
        }
        ++i;
    }
}

// ---------- v10: jogador num tanque roubado ----------
// O mod trava os canhoes do Rhino/Khanjali entre os tiros dos retaliadores; a trava fica no veiculo.
// Com o jogador no banco do motorista, libera tudo para ele.
static void PlayerTankGuard() {
    static Vehicle last = 0;
    Vehicle pv = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE) : 0;
    if (!pv) { last = 0; return; }
    Hash m = ENTITY::GET_ENTITY_MODEL(pv);
    static const Hash rhino = H("rhino"), khan = H("khanjali");
    if (m != rhino && m != khan) { last = 0; return; }
    static const char *kAll[] = {"VEHICLE_WEAPON_TANK", "VEHICLE_WEAPON_KHANJALI_CANNON", "VEHICLE_WEAPON_KHANJALI_CANNON_HEAVY",
                                 "VEHICLE_WEAPON_KHANJALI_MG", "VEHICLE_WEAPON_KHANJALI_GL"};
    for (const char *g : kAll) VEHICLE::DISABLE_VEHICLE_WEAPON(FALSE, H(g), pv, s_player);
    VehAmmoRestore(pv);
    if (pv != last) {
        last = pv;
        Log("Jogador entrou num tanque (%s): armas liberadas.", m == rhino ? "Rhino" : "Khanjali");
        if (pv == s_tank.rhino && s_tank.state >= 0) { s_tank.state = -1; TankCooldown(s_tankCdUntil, "tanque no Cargobob"); }
        if (pv == s_khan.v) { s_khan = KhanSt{}; TankCooldown(s_khanCdUntil, "Khanjali"); }
    }
}

// ---------- v10.1: armas de veiculo em rajadas ----------
// Helicopteros armados, avioes, caca e torretas dos retaliadores: FireSeconds liberadas, RestSeconds travadas
// (municao restrita 0). Os tanques do mod tem o controle proprio (UpdateTank/UpdateKhanjali).
struct GunGate { Vehicle v; int next; bool open; };
static std::vector<GunGate> s_gates;
static void UpdateVehGuns() {
    Vehicle pv = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE) : 0;
    for (SquadVeh &s : s_v) {
        if (s.veh == pv || (s.veh == s_tank.rhino && s_tank.state >= 0) || s.veh == s_khan.v) continue;
        bool known = false;
        for (GunGate &g : s_gates) if (g.v == s.veh) { known = true; break; }
        if (known || !ENTITY::DOES_ENTITY_EXIST(s.veh) || !VEHICLE::DOES_VEHICLE_HAVE_WEAPONS(s.veh)) continue;
        // comeca travado, com espera sorteada para os veiculos nao atirarem todos juntos
        s_gates.push_back({s.veh, g_st.now + 1000 + Rand(std::max(1, g_cfg.vehGunRest) * 1000), false});
        Log("  armas de veiculo em rajadas (modelo %u): %ds atirando, %ds parado.",
            (unsigned)ENTITY::GET_ENTITY_MODEL(s.veh), g_cfg.vehGunFire, g_cfg.vehGunRest);
    }
    for (size_t i = 0; i < s_gates.size();) {
        GunGate &g = s_gates[i];
        bool tracked = false;
        for (SquadVeh &s : s_v) if (s.veh == g.v) { tracked = true; break; }
        if (!tracked || !ENTITY::DOES_ENTITY_EXIST(g.v) || g.v == pv) {
            VehAmmoRestore(g.v);
            s_gates.erase(s_gates.begin() + i);
            continue;
        }
        if (g_st.now >= g.next) {
            g.open = !g.open;
            g.next = g_st.now + (g.open ? g_cfg.vehGunFire : g_cfg.vehGunRest) * 1000;
        }
        VehAmmoAll(g.v, g.open ? -1 : 0);
        ++i;
    }
}

// ---------- v10.1: civis e transito com o jogador procurado ----------
// Procurado, o jogo esvazia as ruas (a policia e os retaliadores ocupam o orcamento de pedestres).
// Todo frame, enquanto houver estrelas ou retaliacao: orcamento maximo e densidade normal de civis e carros.
static void PopulationTick() {
    if (!g_cfg.popKeep) return;
    if (PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID()) <= 0 && s_level < 1) return;
    invoke<Void>(0x8C95333CFC3340F3, 3); // SET_PED_POPULATION_BUDGET (3 = maximo)
    invoke<Void>(0xCB9E1EB3BE2AF4E9, 3); // SET_VEHICLE_POPULATION_BUDGET
    PED::SET_PED_DENSITY_MULTIPLIER_THIS_FRAME(g_cfg.popPeds);
    PED::SET_SCENARIO_PED_DENSITY_MULTIPLIER_THIS_FRAME(g_cfg.popPeds, g_cfg.popPeds);
    VEHICLE::SET_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(g_cfg.popTraffic);
    VEHICLE::SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(g_cfg.popTraffic);
    VEHICLE::SET_PARKED_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(g_cfg.popTraffic);
}

// ---------- v10: jogador na torreta ----------
// Quando o jogador assume a torreta de um veiculo (dos retaliadores ou nao), ninguem pode tira-lo de la e a
// tripulacao inimiga que estava dentro desce e luta a pe. Tudo fica no log para diagnostico.
static Vehicle s_turVeh = 0;
static int s_turSeat = -2, s_turSince = 0;
static void PlayerTurretGuard() {
    Vehicle pv = PED::IS_PED_IN_ANY_VEHICLE(s_player, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(s_player, FALSE) : 0;
    int seat = -2;
    if (pv) {
        int maxP = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(pv);
        for (int st = 0; st < maxP; ++st)
            if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(pv, st) == s_player) { seat = st; break; }
        if (seat != -2 && !invoke<BOOL>(0xE33FFA906CE74880, pv, seat)) seat = -2; // IS_TURRET_SEAT
    }
    if (seat != -2) {
        if (pv != s_turVeh || seat != s_turSeat) {
            s_turVeh = pv; s_turSeat = seat; s_turSince = g_st.now;
            PED::SET_PED_CAN_BE_DRAGGED_OUT(s_player, FALSE);
            int crew = 0;
            for (Hunter &h : s_h)
                if (PED::IS_PED_IN_VEHICLE(h.ped, pv, FALSE) && !PED::IS_PED_DEAD_OR_DYING(h.ped, TRUE)) {
                    h.stayInVeh = false;
                    h.leaving = true;
                    h.leftAt = g_st.now;
                    AI::TASK_LEAVE_VEHICLE(h.ped, pv, 0);
                    ++crew;
                }
            Log("Jogador na torreta (veiculo %u, assento %d); %d retaliador(es) mandados descer.",
                (unsigned)ENTITY::GET_ENTITY_MODEL(pv), seat, crew);
        }
        PED::SET_PED_CAN_BE_DRAGGED_OUT(s_player, FALSE);
        // alguem que entrar depois tambem nao fica no mesmo veiculo
        for (Hunter &h : s_h)
            if (!h.leaving && PED::IS_PED_IN_VEHICLE(h.ped, pv, FALSE) && !PED::IS_PED_DEAD_OR_DYING(h.ped, TRUE)) {
                h.stayInVeh = false; h.leaving = true; h.leftAt = g_st.now;
                AI::TASK_LEAVE_VEHICLE(h.ped, pv, 0);
            }
    } else if (s_turVeh) {
        bool dead = PLAYER::IS_PLAYER_DEAD(PLAYER::PLAYER_ID());
        Log("Jogador saiu da torreta depois de %.1fs (%s).", (g_st.now - s_turSince) / 1000.f,
            dead ? "morreu" : pv ? "trocou de assento" : "fora do veiculo");
        PED::SET_PED_CAN_BE_DRAGGED_OUT(s_player, TRUE);
        s_turVeh = 0; s_turSeat = -2;
    }
}

// ---------- v10: lista das roupas do personagem do Online (para montar o traje do Juggernaut) ----------
// Grava Retaliation_roupas.txt na pasta do jogo uma vez: componente, peca, textura e o nome da roupa no jogo.
struct ShopComp { // layout do jogo: cada campo ocupa 8 bytes, depois o nome (64 bytes)
    alignas(8) unsigned lockHash; alignas(8) unsigned nameHash; alignas(8) int locate; alignas(8) int drawable;
    alignas(8) int texture; alignas(8) int unk1; alignas(8) int compType; alignas(8) int unk2; alignas(8) int unk3;
    char label[64];
};
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

    PlayerTurretGuard(); // v10: todo frame, mesmo com o mod pausado
    PlayerTankGuard();
    PopulationTick(); // v10.1
    if (g_st.now - s_lastUpdate >= 200) {
        s_lastUpdate = g_st.now;
        UpdateHunters();
        UpdateHelis();
        UpdateTank();
        UpdateKhanjali();
        UpdateVehGuns(); // v10.1
        UpdateJet();
        UpdateAir();
        UpdateParadrop();
    }
    if (dead) return;

    s_level = LevelOf(g_st.Pts());
    bool active = s_level >= 1 && !g_st.paused && g_st.now >= g_st.cooldownUntil;
    if (s_level < 1) s_activeSince = 0;
    if (!active) {
        s_q.clear();
        if (s_level < 1) s_nextWave = 0;
        return;
    }
    if (!s_activeSince) { s_activeSince = g_st.now; s_flowNext = g_st.now; } // v9: comeca a pressao crescente
    s_fac = FactionFor(s_level, RegionAt(s_ppos.x, s_ppos.y));
    if (!s_fac || s_fac->peds.empty()) return;

    if (!s_nextWave) s_nextWave = g_st.now + g_cfg.firstWaveDelay * 1000;
    if (g_st.now >= s_nextWave) {
        s_nextWave = g_st.now + g_cfg.lv[s_level - 1].interval * 1000;
        BuildWave();
    }
    RunQueue();
}
