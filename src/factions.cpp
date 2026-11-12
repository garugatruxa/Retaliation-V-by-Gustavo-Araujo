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

static const std::vector<FactionDef> kDefs = {
    {"MARABUNTA", "Marabunta Grande",
     {"g_m_y_salvagoon_01", "g_m_y_salvagoon_02", "g_m_y_salvagoon_03", "g_m_y_salvaboss_01"},
     {"voodoo2", "emperor2", "primo", "tornado3"}, {"faggio2"}, {"bmx", "cruiser"}, {}, 25, false,
     "AMBIENT_GANG_SALVA"},
    {"CAIPIRAS", "Caipiras de Sandy Shores",
     {"a_m_m_hillbilly_01", "a_m_m_hillbilly_02", "a_m_m_rurmeth_01", "a_m_y_methhead_01", "a_m_m_salton_02"},
     {"bodhi2", "sadler", "rancherxl", "rebel"}, {"sanchez", "sanchez2", "blazer"}, {"bmx"}, {}, 40, false,
     "AMBIENT_GANG_HILLBILLY"},
    {"VAGOS", "Vagos",
     {"g_m_y_mexgoon_01", "g_m_y_mexgoon_02", "g_m_y_mexgoon_03", "g_m_y_mexgang_01", "g_f_y_vagos_01"},
     {"chino", "tornado", "peyote", "manana", "moonbeam"}, {"ruffian"}, {"scorcher"}, {}, 20, false,
     "AMBIENT_GANG_MEXICAN"},
    {"ONEIL", "Irmaos O'Neil",
     {"a_m_m_farmer_01", "a_m_m_hillbilly_02", "a_m_m_salton_01", "a_m_m_salton_03", "a_m_m_rurmeth_01"},
     {"rebel2", "bison", "bfinjection", "dloader", "marshall"}, {"sanchez"}, {"bmx"}, {}, 25, false,
     "AMBIENT_GANG_HILLBILLY"},
    {"BALLAS", "Ballas",
     {"g_m_y_ballaorig_01", "g_m_y_ballasout_01", "g_m_y_ballaeast_01", "g_f_y_ballas_01"},
     {"baller", "buccaneer", "faction", "virgo", "cavalcade"}, {"pcj", "bagger"}, {}, {}, 25, false,
     "AMBIENT_GANG_BALLAS"},
    {"ALTRUISTAS", "Altruistas",
     {"a_m_m_acult_01", "a_m_o_acult_01", "a_m_o_acult_02", "a_m_y_acult_01", "a_m_y_acult_02"},
     {"dune", "rebel", "bodhi2"}, {"blazer", "sanchez"}, {}, {}, 35, false, "AMBIENT_GANG_CULT"},
    {"KKANGPAE", "Kkangpae",
     {"g_m_y_korean_01", "g_m_y_korean_02", "g_m_y_korlieut_01", "g_m_m_korboss_01"},
     {"sultan", "futo", "penumbra", "fusilade"}, {"akuma", "vader"}, {}, {"dinghy", "seashark"}, 30, false,
     nullptr},
    {"LOST", "The Lost MC",
     {"g_m_y_lost_01", "g_m_y_lost_02", "g_m_y_lost_03", "g_f_y_lost_01"},
     {"gburrito", "slamvan", "monster"}, {"daemon", "hexer", "innovation", "sovereign"}, {}, {}, 80, false,
     "AMBIENT_GANG_LOST"},
    {"TRIADS", "Triads",
     {"g_m_m_chigoon_01", "g_m_m_chigoon_02", "g_m_m_chiboss_01", "g_m_m_chicold_01"},
     {"fugitive", "washington", "elegy2", "feltzer2"}, {"bati", "double", "hakuchou"}, {}, {"squalo", "jetmax"},
     35, false, "AMBIENT_GANG_WEICHENG"},
    {"MADRAZO", "Cartel Madrazo",
     {"g_m_m_mexboss_01", "g_m_m_mexboss_02", "a_m_y_mexthug_01", "csb_ramp_mex"},
     {"cavalcade2", "granger", "dubsta", "baller2", "sandking"}, {"sanchez"}, {}, {"tropic", "seashark"}, 20,
     false, nullptr},
    {"FIB", "Agentes FIB / IAA",
     {"s_m_m_fibsec_01", "s_m_m_fiboffice_01", "s_m_m_fiboffice_02", "s_m_m_ciasec_01"},
     {"fbi", "fbi2", "washington"}, {}, {}, {"dinghy"}, 0, true, nullptr},
    {"CACADORES", "Cacadores de recompensa",
     {"ig_cletus", "ig_hunter", "s_m_y_marine_03", "s_m_m_marine_01", "g_m_m_cartelguards_01",
      "g_m_m_cartelguards_02", "s_m_y_blackops_03"},
     {"kuruma", "buffalo", "granger", "mesa"}, {"bati", "sanchez"}, {}, {"jetmax"}, 20, false, nullptr},
    {"MERRYWEATHER", "Merryweather",
     {"s_m_y_blackops_01", "s_m_y_blackops_02", "csb_mweather"},
     {"mesa3", "crusader", "insurgent2"}, {}, {}, {"dinghy3", "dinghy"}, 0, false, nullptr},
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

// arsenal por nivel (1..8); cada unidade tambem pode puxar armas do nivel anterior
static const std::vector<const char *> kWeapons[8] = {
    {"WEAPON_SNSPISTOL", "WEAPON_VINTAGEPISTOL", "WEAPON_PISTOL", "WEAPON_PISTOL", "WEAPON_KNIFE", "WEAPON_BAT",
     "WEAPON_MACHETE"},
    {"WEAPON_COMBATPISTOL", "WEAPON_MICROSMG", "WEAPON_SAWNOFFSHOTGUN", "WEAPON_DBSHOTGUN", "WEAPON_MUSKET",
     "WEAPON_PISTOL"},
    {"WEAPON_SMG", "WEAPON_MACHINEPISTOL", "WEAPON_PUMPSHOTGUN", "WEAPON_BULLPUPSHOTGUN", "WEAPON_PISTOL50",
     "WEAPON_REVOLVER"},
    {"WEAPON_COMPACTRIFLE", "WEAPON_ASSAULTRIFLE", "WEAPON_GUSENBERG", "WEAPON_ASSAULTSMG", "WEAPON_HEAVYSHOTGUN",
     "WEAPON_SMG"},
    {"WEAPON_ASSAULTRIFLE", "WEAPON_CARBINERIFLE", "WEAPON_BULLPUPRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_MG"},
    {"WEAPON_ADVANCEDRIFLE", "WEAPON_SPECIALCARBINE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_MARKSMANRIFLE",
     "WEAPON_CARBINERIFLE"},
    {"WEAPON_SPECIALCARBINE", "WEAPON_ADVANCEDRIFLE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_COMBATMG",
     "WEAPON_CARBINERIFLE"},
    {"WEAPON_SPECIALCARBINE", "WEAPON_ADVANCEDRIFLE", "WEAPON_ASSAULTSHOTGUN", "WEAPON_COMBATMG",
     "WEAPON_CARBINERIFLE"},
};
// papeis especiais: sniper (nivel 6+) e explosivos (nivel 8)
static const char *kSniper[8] = {nullptr, nullptr, nullptr, nullptr, nullptr, "WEAPON_SNIPERRIFLE",
                                 "WEAPON_HEAVYSNIPER", "WEAPON_HEAVYSNIPER"};
static const std::vector<const char *> kHeavy = {"WEAPON_RPG", "WEAPON_GRENADELAUNCHER"};

// ---------- estado resolvido ----------
std::vector<Faction> g_factions;
static std::vector<Hash> s_weap[8];
static Hash s_sniper[8];
static std::vector<Hash> s_heavy;
static Hash s_player, s_cop, s_army;

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
        if (f.peds.empty()) Log("  [%s] AVISO: nenhum modelo de NPC valido!", d.id);
        g_factions.push_back(f);
    }
    for (int i = 0; i < 8; ++i) {
        s_weap[i].clear();
        for (const char *w : kWeapons[i]) s_weap[i].push_back(H(w));
        s_sniper[i] = kSniper[i] ? H(kSniper[i]) : 0;
    }
    s_heavy.clear();
    for (const char *w : kHeavy) s_heavy.push_back(H(w));
    RelationsApply();
    Log("Faccoes carregadas: %d.", (int)g_factions.size());
}

// Odio ao jogador, guerra com a policia e entre faccoes; FIB e aliado da policia.
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
            if (&g != &f) PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, f.group, g.group);
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

Hash WeaponFor(int level) {
    if (level < 1) level = 1;
    if (level > 8) level = 8;
    int li = level - 1;
    if (li > 0 && Rand(100) < 30) li -= 1; // 30%: arma do nivel anterior
    const auto &pool = s_weap[li];
    return pool[Rand((int)pool.size())];
}
Hash SniperFor(int level) { return level >= 1 && level <= 8 ? s_sniper[level - 1] : 0; }
Hash HeavyWeapon() { return s_heavy[Rand((int)s_heavy.size())]; }
