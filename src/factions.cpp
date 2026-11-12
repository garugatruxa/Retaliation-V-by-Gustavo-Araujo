// Faccoes, arsenal por nivel, regioes e grupos de relacionamento.
// Modelos que nao existem no jogo sao descartados na inicializacao (e anotados no log).
#include "spawn.h"

static Hash H(const char *s) { return GAMEPLAY::GET_HASH_KEY((char *)s); }

// ---------- dados ----------
struct FactionDef {
    const char *id, *name;
    std::vector<const char *> peds, cars, bikes, bicycles, boats;
    int bikePct;          // chance de um veiculo ser moto (se houver)
    bool policeAlly;      // FIB/IAA: aliados da policia
    const char *ambientGroup; // gangue ambiente do jogo que e aliada (ou nullptr)
};

// v6: veiculos do Online "pe no chao" (lowriders e customizados nas gangues, off-road no interior,
// blindados na elite). Nada de veiculos com nitro, pulo ou misseis.
// v10: veiculos com torreta so na elite (e no cartel), e dependem de [Vehicles] TurretVehicles no .ini.
static const std::vector<FactionDef> kDefs = {
    {"MARABUNTA", "Marabunta Grande",
     {"g_m_y_salvagoon_01", "g_m_y_salvagoon_02", "g_m_y_salvagoon_03", "g_m_y_salvaboss_01"},
     {"voodoo2", "emperor2", "primo", "tornado3", "primo2", "buccaneer2", "minivan2", "faction3"}, {"faggio2", "manchez"},
     {"bmx", "cruiser"}, {}, 25, false, "AMBIENT_GANG_SALVA"},
    {"CAIPIRAS", "Caipiras de Sandy Shores",
     {"a_m_m_hillbilly_01", "a_m_m_hillbilly_02", "a_m_m_rurmeth_01", "a_m_y_methhead_01", "a_m_m_salton_02"},
     {"bodhi2", "sadler", "rancherxl", "rebel", "riata", "kalahari", "yosemite3", "bfinjection"},
     {"sanchez", "sanchez2", "blazer", "verus", "manchez2"}, {"bmx"}, {}, 40, false, "AMBIENT_GANG_HILLBILLY"},
    {"VAGOS", "Vagos",
     {"g_m_y_mexgoon_01", "g_m_y_mexgoon_02", "g_m_y_mexgoon_03", "g_m_y_mexgang_01", "g_f_y_vagos_01"},
     {"chino", "tornado", "peyote", "manana", "moonbeam", "chino2", "moonbeam2", "tornado5", "sabregt2", "virgo2",
      "peyote2", "manana2"},
     {"ruffian", "manchez"}, {"scorcher"}, {}, 20, false, "AMBIENT_GANG_MEXICAN"},
    {"ONEIL", "Irmaos O'Neil",
     {"a_m_m_farmer_01", "a_m_m_hillbilly_02", "a_m_m_salton_01", "a_m_m_salton_03", "a_m_m_rurmeth_01"},
     {"rebel2", "bison", "bfinjection", "dloader", "marshall", "kamacho", "riata", "yosemite2"}, {"sanchez", "manchez2"},
     {"bmx"}, {}, 25, false, "AMBIENT_GANG_HILLBILLY"},
    {"BALLAS", "Ballas",
     {"g_m_y_ballaorig_01", "g_m_y_ballasout_01", "g_m_y_ballaeast_01", "g_f_y_ballas_01"},
     {"baller", "buccaneer", "faction", "virgo", "cavalcade", "buccaneer2", "faction2", "virgo3", "voodoo", "baller3",
      "glendale2"},
     {"pcj", "bagger"}, {}, {}, 25, false, "AMBIENT_GANG_BALLAS"},
    {"ALTRUISTAS", "Altruistas",
     {"a_m_m_acult_01", "a_m_o_acult_01", "a_m_o_acult_02", "a_m_y_acult_01", "a_m_y_acult_02"},
     {"dune", "rebel", "bodhi2", "outlaw", "vagrant"}, {"blazer", "sanchez", "verus", "manchez2"}, {}, {}, 35, false,
     "AMBIENT_GANG_CULT"},
    {"KKANGPAE", "Kkangpae",
     {"g_m_y_korean_01", "g_m_y_korean_02", "g_m_y_korlieut_01", "g_m_m_korboss_01"},
     {"sultan", "futo", "penumbra", "fusilade", "sultan2", "futo2", "kuruma", "penumbra2"}, {"akuma", "vader"}, {},
     {"dinghy", "seashark"}, 30, false, nullptr},
    {"LOST", "The Lost MC",
     {"g_m_y_lost_01", "g_m_y_lost_02", "g_m_y_lost_03", "g_f_y_lost_01"},
     {"gburrito", "slamvan", "monster", "gburrito2", "slamvan3"},
     {"daemon", "hexer", "innovation", "sovereign", "zombiea", "zombieb", "chimera", "daemon2", "wolfsbane", "nightblade",
      "avarus", "gargoyle"},
     {}, {}, 80, false, "AMBIENT_GANG_LOST"},
    {"TRIADS", "Triads",
     {"g_m_m_chigoon_01", "g_m_m_chigoon_02", "g_m_m_chiboss_01", "g_m_m_chicold_01"},
     {"fugitive", "washington", "elegy2", "feltzer2", "schafter3", "elegy", "kuruma", "cog55"},
     {"bati", "double", "hakuchou"}, {}, {"squalo", "jetmax"}, 35, false, "AMBIENT_GANG_WEICHENG"},
    {"MADRAZO", "Cartel Madrazo",
     {"g_m_m_mexboss_01", "g_m_m_mexboss_02", "a_m_y_mexthug_01", "csb_ramp_mex"},
     {"cavalcade2", "granger", "dubsta", "baller2", "sandking", "baller6", "xls2", "contender", "toros", "technical"},
     {"sanchez", "manchez"}, {}, {"tropic", "seashark"}, 20, false, nullptr},
    // v10: FIB/IAA e militares sairam (sao do governo e nao brigam com a policia).
    // No lugar: a seguranca de El Rubio (Cayo Perico) e os mercenarios russos do Bogdan (Juizo Final).
    {"CAYO", "Seguranca de Cayo Perico",
     {"g_m_m_cartelguards_01", "g_m_m_cartelguards_02"},
     {"winky", "mesa3", "kamacho", "patriot", "dubsta3", "kuruma2"}, {"manchez2", "verus"}, {},
     {"dinghy", "longfin", "dinghy5", "patrolboat"},
     20, false, nullptr},
    {"CACADORES", "Cacadores de recompensa",
     {"ig_cletus", "ig_hunter", "s_m_y_blackops_03", "mp_g_m_pros_01"},
     {"kuruma", "buffalo", "granger", "mesa", "kuruma2", "nightshark", "baller6", "dubsta2", "technical", "dukes2"},
     {"bati", "sanchez", "manchez2"}, {}, {"jetmax"}, 20, false, nullptr},
    {"BOGDAN", "Mercenarios russos do Bogdan",
     {"mp_m_bogdangoon"},
     {"insurgent2", "insurgent", "nightshark", "ardent", "dukes2", "kuruma2", "dune3", "kamacho"}, {"manchez2"}, {},
     {"dinghy"}, 10, false,
     nullptr},
    {"MERRYWEATHER", "Merryweather",
     {"s_m_y_blackops_01", "s_m_y_blackops_02", "csb_mweather"},
     {"mesa3", "insurgent2", "insurgent", "kuruma2", "nightshark", "dune3", "baller6", "dukes2"}, {}, {},
     {"dinghy3", "dinghy", "patrolboat"}, 0, false, nullptr},
};
// nivel (1..8) -> faccao na cidade / no interior (indices em kDefs)
static const int kLevelFaction[8][2] = {
    {0, 1},   // 1: Marabunta | Caipiras
    {2, 3},   // 2: Vagos | O'Neil
    {4, 5},   // 3: Ballas | Altruistas
    {6, 7},   // 4: Kkangpae | Lost
    {8, 9},   // 5: Triads | Madrazo
    {10, 11}, // 6: Cayo Perico | Cacadores (v10)
    {12, 12}, // 7: Mercenarios do Bogdan (v10)
    {13, 13}, // 8: Merryweather
};

// Arsenal por nivel (1..8), v3: o maximo de variedade, sem mosquete e sem Marksman Pistol ("trabuco").
// Corpo a corpo so nos niveis 1 e 2 (faca, taco, cacetete). Armas que o jogo nao conhece sao descartadas.
static const std::vector<const char *> kWeapons[8] = {
    // v10: sem armas MK2 (so as versoes normais do Online)
    {"WEAPON_SNSPISTOL", "WEAPON_VINTAGEPISTOL", "WEAPON_PISTOL", "WEAPON_PISTOL", "WEAPON_CERAMICPISTOL",
     "WEAPON_KNIFE", "WEAPON_BAT", "WEAPON_NIGHTSTICK"},
    {"WEAPON_COMBATPISTOL", "WEAPON_PISTOL", "WEAPON_MICROSMG", "WEAPON_MINISMG", "WEAPON_SAWNOFFSHOTGUN",
     "WEAPON_DBSHOTGUN", "WEAPON_KNIFE", "WEAPON_BAT", "WEAPON_PISTOLXM3"},
    {"WEAPON_SMG", "WEAPON_MACHINEPISTOL", "WEAPON_MINISMG", "WEAPON_PUMPSHOTGUN", "WEAPON_BULLPUPSHOTGUN",
     "WEAPON_PISTOL50", "WEAPON_REVOLVER", "WEAPON_HEAVYPISTOL", "WEAPON_DOUBLEACTION", "WEAPON_NAVYREVOLVER",
     "WEAPON_APPISTOL", "WEAPON_TECPISTOL", "WEAPON_GADGETPISTOL"},
    {"WEAPON_COMPACTRIFLE", "WEAPON_ASSAULTRIFLE", "WEAPON_GUSENBERG", "WEAPON_ASSAULTSMG", "WEAPON_HEAVYSHOTGUN",
     "WEAPON_SMG", "WEAPON_COMBATPDW", "WEAPON_AUTOSHOTGUN"},
    {"WEAPON_ASSAULTRIFLE", "WEAPON_CARBINERIFLE", "WEAPON_BULLPUPRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_MG",
     "WEAPON_COMBATPDW", "WEAPON_COMBATSHOTGUN", "WEAPON_MILITARYRIFLE", "WEAPON_TACTICALRIFLE"},
    {"WEAPON_ADVANCEDRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_CARBINERIFLE",
     "WEAPON_MILITARYRIFLE", "WEAPON_TACTICALRIFLE", "WEAPON_HEAVYRIFLE", "WEAPON_COMBATMG",
     "WEAPON_PUMPSHOTGUN", "WEAPON_BATTLERIFLE"},
    {"WEAPON_SPECIALCARBINE", "WEAPON_CARBINERIFLE", "WEAPON_ASSAULTRIFLE", "WEAPON_BULLPUPRIFLE",
     "WEAPON_ADVANCEDRIFLE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_COMBATMG", "WEAPON_HEAVYRIFLE", "WEAPON_MG",
     "WEAPON_BATTLERIFLE", "WEAPON_MILITARYRIFLE"},
    {"WEAPON_CARBINERIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_ADVANCEDRIFLE", "WEAPON_COMBATMG",
     "WEAPON_ASSAULTSHOTGUN", "WEAPON_HEAVYRIFLE", "WEAPON_TACTICALRIFLE", "WEAPON_COMBATSHOTGUN",
     "WEAPON_BATTLERIFLE"},
};
// papeis especiais: sniper (nivel 6+) e armas pesadas (nivel 7+)
static const std::vector<const char *> kSniper[8] = {
    {}, {}, {}, {}, {},
    {"WEAPON_SNIPERRIFLE", "WEAPON_MARKSMANRIFLE"},
    {"WEAPON_HEAVYSNIPER", "WEAPON_MARKSMANRIFLE", "WEAPON_PRECISIONRIFLE"},
    {"WEAPON_HEAVYSNIPER", "WEAPON_MARKSMANRIFLE", "WEAPON_PRECISIONRIFLE"},
};
static const std::vector<const char *> kHeavy[8] = {
    {}, {}, {}, {}, {}, {},
    {"WEAPON_GRENADELAUNCHER", "WEAPON_COMPACTLAUNCHER", "WEAPON_COMBATMG"},
    {"WEAPON_RPG", "WEAPON_RPG", "WEAPON_GRENADELAUNCHER", "WEAPON_COMPACTLAUNCHER", "WEAPON_MINIGUN"},
};
static const char *kMelee[] = {"WEAPON_KNIFE", "WEAPON_BAT", "WEAPON_NIGHTSTICK"};
// v5: armas que nunca podem aparecer, mesmo se alguem colocar na lista
static const char *kBanned[] = {"WEAPON_MUSKET",      "WEAPON_MARKSMANPISTOL", "WEAPON_RAILGUN",     "WEAPON_RAILGUNXM3",
                                "WEAPON_RAYPISTOL",   "WEAPON_RAYCARBINE",     "WEAPON_RAYMINIGUN",  "WEAPON_EMPLAUNCHER",
                                "WEAPON_STUNGUN",     "WEAPON_STUNGUN_MP",     "WEAPON_SNOWLAUNCHER", "WEAPON_FLAREGUN",
                                "WEAPON_FIREWORK",    "WEAPON_CANDYCANE",      "WEAPON_ACIDPACKAGE", "WEAPON_HAZARDCAN"};
// v6: veiculos "apeloes" que nunca entram (nitro, pulo, misseis, voadores)
static const char *kBannedVeh[] = {"oppressor", "oppressor2", "toreador", "ruiner2",  "scramjet", "vigilante",
                                   "deluxo",    "stromberg",  "thruster", "khanjali", "chernobog", "tampa3",
                                   "dune4",      "dune5",    "apc",      "blazer5",  "boxville5",
                                   "technical2", "technical3", "insurgent3", "zr380",  "imperator", "dominator4",
                                   "limo2", "trailersmall2", "brickade2",
                                   // v10: no modo historia o jogo expulsa o jogador da torreta destes (testado no Menacer)
                                   "menacer", "caracara", "halftrack", "barrage", "patrolboat", "dinghy5",
                                   // v10: veiculos militares (Fort Zancudo) nao combinam com as faccoes
                                   "crusader", "barracks", "barracks2", "barracks3", "squaddie", "vetir", "patriot3"};
// v10: veiculos com torreta que funcionam no modo historia (Insurgent testado). So com [Vehicles] TurretVehicles=1
static const char *kTurretVeh[] = {"technical", "insurgent", "dune3"}; // dune3 (Dune FAV): em teste

static bool BannedName(const char *n, const char *const *list, size_t count) {
    for (size_t i = 0; i < count; ++i) if (!_stricmp(n, list[i])) return true;
    return false;
}

// v5: helicopteros por faccao. transport = atiradores ou rapel; attack = helicoptero de ataque (Buzzard).
struct HeliDef { const char *id; std::vector<const char *> transport, attack; };
static const std::vector<HeliDef> kHelis = {
    {"ONEIL", {"frogger"}, {}},
    {"BALLAS", {"frogger"}, {}},
    {"KKANGPAE", {"maverick", "frogger"}, {}},
    {"TRIADS", {"supervolito", "supervolito2", "maverick"}, {}},
    {"MADRAZO", {"maverick", "frogger"}, {}},
    {"CAYO", {"annihilator2", "maverick"}, {}},
    {"CACADORES", {"maverick", "frogger"}, {"buzzard"}},
    {"BOGDAN", {"maverick", "valkyrie"}, {"buzzard", "buzzard", "buzzard", "akula"}}, // v10.1: Akula mais rara
    {"MERRYWEATHER", {"annihilator", "maverick", "valkyrie", "annihilator2"},
     {"buzzard", "buzzard", "buzzard", "buzzard", "buzzard", "buzzard", "savage", "hunter"}}, // v10.1: Savage/Hunter mais raros
};

// ---------- estado resolvido ----------
std::vector<Faction> g_factions;
static std::vector<Hash> s_weap[8], s_sniper[8], s_heavy[8];
static std::vector<Hash> s_melee;
static Hash s_player, s_cop, s_army;

static std::vector<Hash> ResolveWeapons(const std::vector<const char *> &names) {
    std::vector<Hash> out;
    for (const char *n : names) {
        // v10: nada de MK2 nem de armas proibidas
        if (strstr(n, "_MK2") || BannedName(n, kBanned, sizeof kBanned / sizeof *kBanned)) continue;
        Hash h = H(n);
        if (WEAPON::IS_WEAPON_VALID(h)) out.push_back(h);
        else Log("  arma nao encontrada no jogo: %s", n);
    }
    return out;
}
static bool IsMelee(Hash w) {
    for (Hash m : s_melee) if (m == w) return true;
    return false;
}

static std::vector<Hash> Resolve(const std::vector<const char *> &names, const char *fac, bool vehicle) {
    std::vector<Hash> out;
    for (const char *n : names) {
        if (vehicle && BannedName(n, kBannedVeh, sizeof kBannedVeh / sizeof *kBannedVeh)) continue; // v10
        if (vehicle && !g_cfg.turretVehicles && BannedName(n, kTurretVeh, sizeof kTurretVeh / sizeof *kTurretVeh)) continue;
        Hash h = H(n);
        if (STREAMING::IS_MODEL_IN_CDIMAGE(h) && (!vehicle || STREAMING::IS_MODEL_A_VEHICLE(h))) out.push_back(h);
        else Log("  [%s] modelo nao encontrado no jogo: %s", fac, n);
    }
    return out;
}

void FactionsInit() {
    g_factions.clear();
    s_player = H("PLAYER");
    s_cop = H("COP");
    s_army = H("ARMY");
    for (const FactionDef &d : kDefs) {
        Faction f;
        f.id = d.id;
        f.name = d.name;
        f.peds = Resolve(d.peds, d.id, false);
        f.cars = Resolve(d.cars, d.id, true);
        f.bikes = Resolve(d.bikes, d.id, true);
        f.bicycles = Resolve(d.bicycles, d.id, true);
        f.boats = Resolve(d.boats, d.id, true);
        f.bikePct = d.bikePct;
        f.policeAlly = d.policeAlly;
        std::string g = std::string("RET_") + d.id;
        PED::ADD_RELATIONSHIP_GROUP((char *)g.c_str(), &f.group);
        f.ambient = d.ambientGroup ? H(d.ambientGroup) : 0;
        for (const HeliDef &hd : kHelis)
            if (!strcmp(hd.id, d.id)) {
                f.helis = Resolve(hd.transport, d.id, true);
                f.attackHelis = Resolve(hd.attack, d.id, true);
            }
        if (f.peds.empty()) Log("  [%s] AVISO: nenhum modelo de NPC valido!", d.id);
        g_factions.push_back(f);
    }
    for (int i = 0; i < 8; ++i) {
        s_weap[i] = ResolveWeapons(kWeapons[i]);
        s_sniper[i] = ResolveWeapons(kSniper[i]);
        s_heavy[i] = ResolveWeapons(kHeavy[i]);
    }
    s_melee.clear();
    for (const char *m : kMelee) s_melee.push_back(H(m));
    RelationsApply();
    Log("Faccoes carregadas: %d.", (int)g_factions.size());
}

// Odio ao jogador e guerra com a policia; faccoes se respeitam (v4); FIB e aliado da policia.
void RelationsApply() {
    for (Faction &f : g_factions) {
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, f.group, f.group);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, f.group, s_player);
        int pol = f.policeAlly ? 1 : 5;
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, f.group, s_cop);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, s_cop, f.group);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, f.group, s_army);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, s_army, f.group);
        if (f.ambient) {
            PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, f.group, f.ambient);
            PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, f.ambient, f.group);
        }
        for (Faction &g : g_factions)
            if (&g != &f) PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, f.group, g.group); // v4: faccoes nao brigam entre si
    }
}

// v11.1: civis armados da Desordem ficam num grupo proprio (o grupo comum dos civis tem tambem os desarmados)
Hash ArmedCivGroup() {
    static Hash g = 0;
    if (!g) PED::ADD_RELATIONSHIP_GROUP((char *)"RET_ARMED_CIV", &g);
    return g;
}

// v11.1: guerra com a policia. So na Desordem (modo 4) os retaliadores e a policia se enfrentam. Nos outros modos
// ficam do mesmo lado: nao se atacam e todos vao atras do jogador. Reaplica a cada 5 s.
void RelationsPolice(bool war) {
    static int last = -1, lastAt = 0;
    int now = (int)GAMEPLAY::GET_GAME_TIMER();
    if ((int)war == last && now - lastAt < 5000) return;
    if ((int)war != last) Log(war ? "Retaliadores em guerra com a policia." : "Retaliadores e policia do mesmo lado (todos contra o jogador).");
    last = war;
    lastAt = now;
    for (Faction &f : g_factions) {
        int pol = f.policeAlly || !war ? 1 : 5;
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, f.group, s_cop);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, s_cop, f.group);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, f.group, s_army);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(pol, s_army, f.group);
    }
}

// v11: modo Desordem. on: os retaliadores odeiam os civis (e os civis armados revidam); calm: ficam neutros com
// o jogador, para so atacarem quem estiver perto ou se o jogador atacar primeiro. Reaplica a cada 5 s.
void RelationsDisorder(bool on, bool calm) {
    static int last = -1, lastAt = 0;
    int key = on ? (calm ? 2 : 1) : 0;
    int now = (int)GAMEPLAY::GET_GAME_TIMER();
    if (key == last && (key == 0 || now - lastAt < 5000)) return;
    last = key;
    lastAt = now;
    static const Hash civm = H("CIVMALE"), civf = H("CIVFEMALE");
    // civis armados: odeiam a policia e os retaliadores, e sao odiados por eles; neutros com o jogador
    Hash ac = ArmedCivGroup();
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, ac, s_cop);
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, s_cop, ac);
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, ac, s_army);
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, s_army, ac);
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(3, ac, s_player);
    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, ac, ac);
    for (Faction &f : g_factions) {
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, f.group, ac);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on ? 5 : 3, ac, f.group);
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(on && calm ? 3 : 5, f.group, s_player);
        for (Hash c : {civm, civf}) {
            if (on) {
                PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, f.group, c);
                PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, c, f.group);
            } else {
                PED::CLEAR_RELATIONSHIP_BETWEEN_GROUPS(5, f.group, c);
                PED::CLEAR_RELATIONSHIP_BETWEEN_GROUPS(5, c, f.group);
            }
        }
    }
}

// Cidade = Los Santos; interior = Blaine County, desertos, montanhas e litoral norte.
Region RegionAt(float x, float y) {
    bool city = y < 1150.f && x > -2200.f && x < 1750.f;
    return city ? RG_CITY : RG_COUNTRY;
}

Faction *FactionFor(int level, Region r) {
    if (level < 1) return nullptr;
    if (level > 8) level = 8;
    int idx = kLevelFaction[level - 1][r];
    return idx < (int)g_factions.size() ? &g_factions[idx] : nullptr;
}

static Hash Pick(const std::vector<Hash> &v) { return v.empty() ? 0 : v[Rand((int)v.size())]; }

// betterPct: chance de puxar uma arma do nivel seguinte (inimigos melhor equipados no alto).
Hash WeaponFor(int level, int betterPct) {
    if (level < 1) level = 1;
    if (level > 8) level = 8;
    int li = level - 1;
    if (li < 7 && Rand(100) < betterPct) li += 1;
    else if (li > 0 && Rand(100) < 25) li -= 1; // 25%: arma do nivel anterior
    for (int tries = 0; tries < 6; ++tries) {
        Hash w = Pick(s_weap[li]);
        if (!w) break;
        if (level >= 3 && IsMelee(w)) continue; // corpo a corpo so nas primeiras waves
        return w;
    }
    return Pick(s_weap[level - 1]);
}
// v10: acessorios aleatorios nas armas (lanterna, carregador estendido, mira, empunhadura, supressor).
// Quanto maior o nivel, maior a chance de vir algum e mais acessorios podem vir juntos.
static const char *kFlash[] = {"COMPONENT_AT_AR_FLSH", "COMPONENT_AT_PI_FLSH", "COMPONENT_AT_PI_FLSH_02",
                               "COMPONENT_AT_PI_FLSH_03"};
static const char *kScope[] = {"COMPONENT_AT_SCOPE_MACRO", "COMPONENT_AT_SCOPE_MACRO_02", "COMPONENT_AT_SCOPE_SMALL",
                               "COMPONENT_AT_SCOPE_SMALL_02", "COMPONENT_AT_SCOPE_MEDIUM", "COMPONENT_AT_PI_RDOT"};
static const char *kGrip[] = {"COMPONENT_AT_AR_AFGRIP", "COMPONENT_AT_AR_AFGRIP_02"};
static const char *kSupp[] = {"COMPONENT_AT_AR_SUPP", "COMPONENT_AT_AR_SUPP_02", "COMPONENT_AT_PI_SUPP",
                              "COMPONENT_AT_PI_SUPP_02", "COMPONENT_AT_SR_SUPP"};
static const char *WeaponName(Hash w) {
    for (int i = 0; i < 8; ++i) {
        for (const char *n : kWeapons[i]) if (H(n) == w) return n;
        for (const char *n : kSniper[i]) if (H(n) == w) return n;
        for (const char *n : kHeavy[i]) if (H(n) == w) return n;
    }
    return nullptr;
}
static Hash FitOne(Hash w, const char *const *list, int n) {
    std::vector<Hash> ok;
    for (int i = 0; i < n; ++i) { Hash c = H(list[i]); if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(w, c)) ok.push_back(c); }
    return ok.empty() ? 0 : ok[Rand((int)ok.size())];
}
void ApplyWeaponMods(Ped ped, Hash w, int level) {
    if (!g_cfg.modsEnabled || !w || level < 1) return;
    int pct = g_cfg.modsBase + (level - 1) * g_cfg.modsPerLevel;
    if (Rand(100) >= pct) return;
    // candidatos que cabem nesta arma, um por tipo
    std::vector<Hash> cand;
    if (Hash c = FitOne(w, kFlash, 4)) cand.push_back(c);
    if (Hash c = FitOne(w, kScope, 6)) cand.push_back(c);
    if (Hash c = FitOne(w, kGrip, 2)) cand.push_back(c);
    if (const char *wn = WeaponName(w)) { // carregador: COMPONENT_<ARMA>_CLIP_02 (estendido) ou _03 (tambor)
        std::string base = std::string("COMPONENT_") + (wn + 7);
        Hash c2 = H((base + "_CLIP_02").c_str()), c3 = H((base + "_CLIP_03").c_str());
        bool ok2 = WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(w, c2), ok3 = WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(w, c3);
        if (ok3 && (!ok2 || Rand(100) < 25)) cand.push_back(c3);
        else if (ok2) cand.push_back(c2);
    }
    if (level >= 4 && Rand(100) < 15) if (Hash c = FitOne(w, kSupp, 5)) cand.push_back(c); // supressor: raro
    if (cand.empty()) return;
    int maxMods = 1 + (level - 1) / 3; // niveis 1-3: 1, 4-6: ate 2, 7-8: ate 3
    int n = 1 + Rand(maxMods);
    for (int i = 0; i < n && !cand.empty(); ++i) {
        int k = Rand((int)cand.size());
        WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(ped, w, cand[k]);
        cand.erase(cand.begin() + k);
    }
}

Hash SniperFor(int level) { return level >= 1 && level <= 8 ? Pick(s_sniper[level - 1]) : 0; }
Hash HeavyFor(int level) { return level >= 1 && level <= 8 ? Pick(s_heavy[level - 1]) : 0; }
