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
// blindados e com torreta na elite). Nada de veiculos com nitro, pulo ou misseis.
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
     {"cavalcade2", "granger", "dubsta", "baller2", "sandking", "baller6", "xls2", "contender", "toros"},
     {"sanchez", "manchez"}, {}, {"tropic", "seashark"}, 20, false, nullptr},
    {"FIB", "Agentes FIB / IAA",
     {"s_m_m_fibsec_01", "s_m_m_fiboffice_01", "s_m_m_fiboffice_02", "s_m_m_ciasec_01"},
     {"fbi", "fbi2", "washington", "granger2", "baller5"}, {}, {}, {"dinghy"}, 0, true, nullptr},
    {"CACADORES", "Cacadores de recompensa",
     {"ig_cletus", "ig_hunter", "s_m_y_marine_03", "s_m_m_marine_01", "g_m_m_cartelguards_01",
      "g_m_m_cartelguards_02", "s_m_y_blackops_03"},
     {"kuruma", "buffalo", "granger", "mesa", "kuruma2", "technical", "caracara", "nightshark", "menacer"},
     {"bati", "sanchez", "manchez2"}, {}, {"jetmax"}, 20, false, nullptr},
    {"MERRYWEATHER", "Merryweather",
     {"s_m_y_blackops_01", "s_m_y_blackops_02", "csb_mweather"},
     {"mesa3", "crusader", "insurgent2", "insurgent", "menacer", "halftrack", "barrage", "caracara"}, {}, {},
     {"dinghy3", "dinghy", "patrolboat"}, 0, false, nullptr},
};
// nivel (1..8) -> faccao na cidade / no interior (indices em kDefs)
static const int kLevelFaction[8][2] = {
    {0, 1},   // 1: Marabunta | Caipiras
    {2, 3},   // 2: Vagos | O'Neil
    {4, 5},   // 3: Ballas | Altruistas
    {6, 7},   // 4: Kkangpae | Lost
    {8, 9},   // 5: Triads | Madrazo
    {10, 10}, // 6: FIB/IAA
    {11, 11}, // 7: Cacadores
    {12, 12}, // 8: Merryweather
};

// Arsenal por nivel (1..8), v3: o maximo de variedade, sem mosquete e sem Marksman Pistol ("trabuco").
// Corpo a corpo so nos niveis 1 e 2 (faca, taco, cacetete). Armas que o jogo nao conhece sao descartadas.
static const std::vector<const char *> kWeapons[8] = {
    {"WEAPON_SNSPISTOL", "WEAPON_VINTAGEPISTOL", "WEAPON_PISTOL", "WEAPON_PISTOL", "WEAPON_CERAMICPISTOL",
     "WEAPON_KNIFE", "WEAPON_BAT", "WEAPON_NIGHTSTICK", "WEAPON_SNSPISTOL_MK2"},
    {"WEAPON_COMBATPISTOL", "WEAPON_PISTOL", "WEAPON_MICROSMG", "WEAPON_MINISMG", "WEAPON_SAWNOFFSHOTGUN",
     "WEAPON_DBSHOTGUN", "WEAPON_KNIFE", "WEAPON_BAT", "WEAPON_PISTOLXM3"},
    {"WEAPON_SMG", "WEAPON_MACHINEPISTOL", "WEAPON_MINISMG", "WEAPON_PUMPSHOTGUN", "WEAPON_BULLPUPSHOTGUN",
     "WEAPON_PISTOL50", "WEAPON_REVOLVER", "WEAPON_HEAVYPISTOL", "WEAPON_DOUBLEACTION", "WEAPON_NAVYREVOLVER",
     "WEAPON_APPISTOL", "WEAPON_TECPISTOL", "WEAPON_REVOLVER_MK2", "WEAPON_GADGETPISTOL"},
    {"WEAPON_COMPACTRIFLE", "WEAPON_ASSAULTRIFLE", "WEAPON_GUSENBERG", "WEAPON_ASSAULTSMG", "WEAPON_HEAVYSHOTGUN",
     "WEAPON_SMG", "WEAPON_COMBATPDW", "WEAPON_AUTOSHOTGUN", "WEAPON_PISTOL_MK2"},
    {"WEAPON_ASSAULTRIFLE", "WEAPON_CARBINERIFLE", "WEAPON_BULLPUPRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_MG",
     "WEAPON_COMBATPDW", "WEAPON_COMBATSHOTGUN", "WEAPON_SMG_MK2", "WEAPON_MILITARYRIFLE", "WEAPON_TACTICALRIFLE"},
    {"WEAPON_ADVANCEDRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_CARBINERIFLE",
     "WEAPON_MILITARYRIFLE", "WEAPON_TACTICALRIFLE", "WEAPON_HEAVYRIFLE", "WEAPON_COMBATMG",
     "WEAPON_PUMPSHOTGUN_MK2", "WEAPON_BATTLERIFLE"},
    {"WEAPON_SPECIALCARBINE_MK2", "WEAPON_CARBINERIFLE_MK2", "WEAPON_ASSAULTRIFLE_MK2", "WEAPON_BULLPUPRIFLE_MK2",
     "WEAPON_ADVANCEDRIFLE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_COMBATMG", "WEAPON_HEAVYRIFLE", "WEAPON_MG",
     "WEAPON_BATTLERIFLE", "WEAPON_MILITARYRIFLE"},
    {"WEAPON_CARBINERIFLE_MK2", "WEAPON_SPECIALCARBINE_MK2", "WEAPON_ASSAULTRIFLE_MK2", "WEAPON_COMBATMG_MK2",
     "WEAPON_ASSAULTSHOTGUN", "WEAPON_HEAVYRIFLE", "WEAPON_TACTICALRIFLE", "WEAPON_COMBATSHOTGUN",
     "WEAPON_BATTLERIFLE"},
};
// papeis especiais: sniper (nivel 6+) e armas pesadas (nivel 7+)
static const std::vector<const char *> kSniper[8] = {
    {}, {}, {}, {}, {},
    {"WEAPON_SNIPERRIFLE", "WEAPON_MARKSMANRIFLE"},
    {"WEAPON_HEAVYSNIPER", "WEAPON_MARKSMANRIFLE_MK2", "WEAPON_PRECISIONRIFLE"},
    {"WEAPON_HEAVYSNIPER_MK2", "WEAPON_HEAVYSNIPER", "WEAPON_MARKSMANRIFLE_MK2"},
};
static const std::vector<const char *> kHeavy[8] = {
    {}, {}, {}, {}, {}, {},
    {"WEAPON_GRENADELAUNCHER", "WEAPON_COMPACTLAUNCHER", "WEAPON_COMBATMG_MK2"},
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
                                   "dune3",     "dune4",      "dune5",    "apc",      "blazer5",  "boxville5",
                                   "technical2", "technical3", "insurgent3", "zr380",  "imperator", "dominator4"};

// v5: helicopteros por faccao. transport = atiradores ou rapel; attack = helicoptero de ataque (Buzzard).
struct HeliDef { const char *id; std::vector<const char *> transport, attack; };
static const std::vector<HeliDef> kHelis = {
    {"ONEIL", {"frogger"}, {}},
    {"BALLAS", {"frogger"}, {}},
    {"KKANGPAE", {"maverick", "frogger"}, {}},
    {"TRIADS", {"supervolito", "supervolito2", "maverick"}, {}},
    {"MADRAZO", {"maverick", "frogger"}, {}},
    {"FIB", {"maverick", "frogger"}, {}},
    {"CACADORES", {"maverick", "frogger"}, {"buzzard"}},
    {"MERRYWEATHER", {"annihilator", "maverick", "valkyrie", "annihilator2"},
     {"buzzard", "buzzard", "buzzard", "savage", "hunter"}}, // Buzzard mais comum
};

// ---------- estado resolvido ----------
std::vector<Faction> g_factions;
static std::vector<Hash> s_weap[8], s_sniper[8], s_heavy[8];
static std::vector<Hash> s_melee;
static Hash s_player, s_cop, s_army;

static std::vector<Hash> ResolveWeapons(const std::vector<const char *> &names) {
    std::vector<Hash> out;
    for (const char *n : names) {
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
Hash SniperFor(int level) { return level >= 1 && level <= 8 ? Pick(s_sniper[level - 1]) : 0; }
Hash HeavyFor(int level) { return level >= 1 && level <= 8 ? Pick(s_heavy[level - 1]) : 0; }
