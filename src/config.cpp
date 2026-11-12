#include "common.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

Config g_cfg;
std::string g_dir;

// ---------- log ----------
static FILE *g_log = nullptr;

void LogOpen(const std::string &path) {
    if (g_log) fclose(g_log);
    g_log = fopen(path.c_str(), "w");
}

void Log(const char *fmt, ...) {
    if (!g_log) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(g_log, "[%02d:%02d:%02d] ", t.wHour, t.wMinute, t.wSecond);
    va_list a;
    va_start(a, fmt);
    vfprintf(g_log, fmt, a);
    va_end(a);
    fputc('\n', g_log);
    fflush(g_log);
}

// ---------- ini ----------
static const char *kDefaultIni =
    "; Retaliation V " RETALIATION_VERSION " - configuracao\r\n"
    "; Apague este arquivo para recriar com os valores padrao.\r\n"
    "\r\n"
    "[General]\r\n"
    "; 0 = mod desligado (padrao). Liga e desliga tambem pelo menu (tecla Insert)\r\n"
    "Enabled=0\r\n"
    "; 1 = emboscadas continuam durante missoes da historia\r\n"
    "ActiveInMissions=0\r\n"
    "; modo (tambem muda pelo menu, tecla Insert): 1 = junto com a policia, 2 = substitui a policia,\r\n"
    "; 3 = ciclo policia -> Retaliation -> livre\r\n"
    "Mode=1\r\n"
    "; Modo Diretor (Rockstar Editor): 1 = Retaliation livre com qualquer NPC (comeca zerado, sobe por crimes ou\r\n"
    "; pelas teclas de teste, sem o ciclo da policia); 0 = remove os retaliadores e pausa\r\n"
    "DirectorMode=1\r\n"
    "; Teclas de teste: segure PageDown e aperte PageUp para subir um nivel;\r\n"
    "; segure PageUp e aperte PageDown para descer um nivel\r\n"
    "DebugKeys=1\r\n"
    "\r\n"
    "[Points]\r\n"
    "KillCivilian=2\r\n"
    "KillCop=2\r\n"
    "KillGang=2\r\n"
    "StealVehicle=2\r\n"
    "BlowVehicle=4\r\n"
    "; retaliador morto pelo jogador\r\n"
    "KillHunter=2\r\n"
    "; helicoptero ou aviao destruido\r\n"
    "BlowAircraft=6\r\n"
    "; segundos sem crimes para perder pontos, e quanto perde\r\n"
    "DecayInterval=45\r\n"
    "DecayAmount=5\r\n"
    "Max=100\r\n"
    "\r\n"
    "[Levels]\r\n"
    "; pontos em que cada nivel comeca (nivel 1, 2, 3...)\r\n"
    "Start=10,21,32,43,54,65,76,88\r\n"
    "\r\n"
    "[HotTimer]\r\n"
    "; acima de Points por Seconds sem morrer nem ser preso: cai de volta para o nivel do [Cycle] StartLevel\r\n"
    "Enabled=1\r\n"
    "Points=70\r\n"
    "Seconds=300\r\n"
    "\r\n"
    "[Cycle]\r\n"
    "; ciclo policia -> Retaliation -> livre: com Wanted estrelas por Seconds sem morrer nem ser preso,\r\n"
    "; o Retaliation comeca no nivel StartLevel. Por JointSeconds a policia mantem as estrelas mas ignora o\r\n"
    "; jogador e so enfrenta os retaliadores; depois as estrelas somem e a policia fica bloqueada ate ficar livre\r\n"
    "Wanted=5\r\n"
    "Seconds=180\r\n"
    "StartLevel=2\r\n"
    "JointSeconds=300\r\n"
    "\r\n"
    "[Hud]\r\n"
    "Enabled=1\r\n"
    "; ajuste fino da posicao da barra (fracao da tela)\r\n"
    "OffsetX=0.0\r\n"
    "OffsetY=0.0085\r\n"
    "Height=0.0055\r\n"
    "; inicio e fim da barra, em fracao da largura do minimapa (0.0 a 1.0 = largura inteira; so o especial = 0.75 a 1.0)\r\n"
    "BarStart=0.0\r\n"
    "BarEnd=1.0\r\n"
    "Color=200,25,25\r\n"
    "; 1 = mostra o numero do nivel ao lado da barra\r\n"
    "ShowLevel=0\r\n"
#ifdef RET_SPAWN
    "\r\n"
    "[Spawn]\r\n"
    "; distancias (metros): grupos a pe, veiculos, remocao\r\n"
    "FootMin=110\r\n"
    "FootMax=170\r\n"
    "VehMin=320\r\n"
    "VehMax=450\r\n"
    "DespawnDist=600\r\n"
    "; segundos para chegar perto do jogador antes de ser removido\r\n"
    "ArriveTimeout=150\r\n"
    "; segundos ate a primeira wave depois de entrar no nivel 1\r\n"
    "FirstWaveDelay=8\r\n"
    "; distancia (m) em que o inimigo aparece no minimapa\r\n"
    "BlipRadius=120\r\n"
    "\r\n"
    "[Scaling]\r\n"
    "; mais dificil chegar a 100: a partir de Start pontos, cada Step pontos acima tiram PerStep de cada ganho\r\n"
    "; (matar, explodir, roubar). Cada acao vale no minimo MinGain. As fracoes se acumulam\r\n"
    "Start=50\r\n"
    "Step=5\r\n"
    "PerStep=0.2\r\n"
    "MinGain=0.2\r\n"
    "\r\n"
    "[Flow]\r\n"
    "; chegada aos poucos: o limite de inimigos comeca em RampStart e sobe ate o do nivel em RampSeconds\r\n"
    "RampSeconds=180\r\n"
    "RampStart=4\r\n"
    "; espaco entre uma unidade e a proxima (1.0 = 7-12 s no nivel 1, 3.5-7 s no nivel 8; maior = mais espacado)\r\n"
    "GapScale=1.0\r\n"
    "; no maximo NearBase + nivel inimigos a menos de 60 m do jogador; o resto espera\r\n"
    "NearBase=3\r\n"
    "; % dos grupos a pe que nascem mais longe (chegam depois)\r\n"
    "FarFootChance=35\r\n"
    "; longe de estrada (metros ate a rua mais proxima): helicopteros ficam mais comuns (+%)\r\n"
    "RemoteRoadDist=150\r\n"
    "RemoteHeliBonus=30\r\n"
    "; agua (mar, lago, rio) por perto: chance (%) de vir um barco na wave, nivel 3+\r\n"
    "WaterBoatChance=35\r\n"
    "\r\n"
    "; Por nivel: Interval (s entre waves), Foot (inimigos a pe), Vehicles, Accuracy (0-100),\r\n"
    "; Health (200 = normal), Armor (0-100), MaxAlive (limite de inimigos vivos, contando os dos veiculos)\r\n"
    "[Level1]\r\nInterval=50\r\nFoot=2\r\nVehicles=1\r\nAccuracy=15\r\nHealth=150\r\nArmor=0\r\nMaxAlive=10\r\n"
    "[Level2]\r\nInterval=45\r\nFoot=3\r\nVehicles=1\r\nAccuracy=20\r\nHealth=175\r\nArmor=0\r\nMaxAlive=12\r\n"
    "[Level3]\r\nInterval=45\r\nFoot=3\r\nVehicles=2\r\nAccuracy=25\r\nHealth=200\r\nArmor=10\r\nMaxAlive=14\r\n"
    "[Level4]\r\nInterval=42\r\nFoot=4\r\nVehicles=2\r\nAccuracy=30\r\nHealth=200\r\nArmor=25\r\nMaxAlive=16\r\n"
    "[Level5]\r\nInterval=40\r\nFoot=4\r\nVehicles=2\r\nAccuracy=35\r\nHealth=225\r\nArmor=50\r\nMaxAlive=18\r\n"
    "[Level6]\r\nInterval=38\r\nFoot=5\r\nVehicles=3\r\nAccuracy=40\r\nHealth=250\r\nArmor=75\r\nMaxAlive=21\r\n"
    "[Level7]\r\nInterval=35\r\nFoot=6\r\nVehicles=3\r\nAccuracy=45\r\nHealth=275\r\nArmor=100\r\nMaxAlive=24\r\n"
    "[Level8]\r\nInterval=30\r\nFoot=6\r\nVehicles=4\r\nAccuracy=50\r\nHealth=300\r\nArmor=100\r\nMaxAlive=28\r\n"
    "\r\n"
    "[Roadblock]\r\n"
    "; bloqueio de estrada a frente quando o jogador esta dirigindo (chance por wave, %)\r\n"
    "Enabled=1\r\n"
    "MinLevel=3\r\n"
    "Chance=30\r\n"
    "\r\n"
    "[Juggernaut]\r\n"
    "; mini-boss com armadura pesada e minigun, chega no Insurgent (chance por wave, %)\r\n"
    "Enabled=1\r\n"
    "MinLevel=7\r\n"
    "; chance por wave: base e teto (sobe com os pontos ate o teto)\r\n"
    "Chance=10\r\n"
    "Max=30\r\n"
    "Health=3000\r\n"
    "; armas: Combat MG e escopeta automatica (comuns) ou minigun (rara: base e teto)\r\n"
    "MinigunChance=10\r\n"
    "MinigunMax=25\r\n"
    "; a roupa do Juggernaut fica em [JuggernautOutfit]. Para copiar a armadura pesada do assalto de Paleto Bay:\r\n"
    "; repita a missao The Paleto Score e, ja de armadura, segure PageDown e aperte End\r\n"
    "\r\n"
    "[TopBonus]\r\n"
    "; % extra de chance dos especiais por ponto acima do inicio do ultimo nivel\r\n"
    "PerPoint=1.5\r\n"
    "\r\n"
    "[Weapons]\r\n"
    "; chance (%) por wave de vir um sniper (nivel 6+) e alguem com RPG/lanca-granadas (nivel 7+, base e teto)\r\n"
    "SniperChance=50\r\n"
    "HeavyChance=12\r\n"
    "HeavyMax=20\r\n"
    "; mira (0-100) de quem usa explosivos (RPG, lanca-granadas, tanque, jato, helicoptero de ataque).\r\n"
    "; baixa de proposito: os tiros passam perto e explodem em volta, mais cinematografico\r\n"
    "ExplosiveAccuracy=8\r\n"
    "\r\n"
    "[Range]\r\n"
    "; distancia (m) em que comecam a atirar: pistolas/SMGs/escopetas e fuzis/metralhadoras.\r\n"
    "; snipers e RPG atiram assim que veem o jogador\r\n"
    "Short=45\r\n"
    "Medium=110\r\n"
    "\r\n"
    "[Heli]\r\n"
    "; chance (%) de helicoptero por wave, do nivel 1 ao 8\r\n"
    "Chance=0,0,8,12,16,22,28,35\r\n"
    "; limite ao mesmo tempo (no maximo 3): niveis 3-5, niveis 7-8 (nivel 6 = 2), e com o jogador voando\r\n"
    "Max=1\r\n"
    "MaxTop=3\r\n"
    "MaxAir=3\r\n"
    "; 2o e 3o helicoptero no ar ao mesmo tempo (base e teto). O 3o so no nivel 8\r\n"
    "SecondChance=10\r\n"
    "SecondMax=20\r\n"
    "ThirdChance=3\r\n"
    "ThirdMax=8\r\n"
    "; % de ser helicoptero de ataque (Buzzard/Savage/Hunter, base e teto) e % de rapel (jogador no chao)\r\n"
    "AttackChance=10\r\n"
    "AttackMax=20\r\n"
    "RappelChance=40\r\n"
    "; % de o helicoptero de ataque ou de rapel pousar perto do jogador a pe e a tropa descer\r\n"
    "LandChance=30\r\n"
    "\r\n"
    "[Tank]\r\n"
    "; Rhino entregue pelo Cargobob (um por vez). Chance por wave (%)\r\n"
    "Enabled=1\r\n"
    "MinLevel=8\r\n"
    "Chance=8\r\n"
    "Max=18\r\n"
    "\r\n"
    "[Jet]\r\n"
    "; caca Lazer (ou Hydra, HydraChance %) que ataca por Duration segundos e vai embora. Um por vez\r\n"
    "Enabled=1\r\n"
    "MinLevel=8\r\n"
    "Chance=5\r\n"
    "Max=15\r\n"
    "Duration=90\r\n"
    "HydraChance=25\r\n"
    "\r\n"
    "[Escalation]\r\n"
    "; a partir desses pontos, cada ponto a mais soma PerPoint % a Juggernaut, minigun e helicoptero de ataque\r\n"
    "Start=70\r\n"
    "PerPoint=1.0\r\n"
    "\r\n"
    "[Gear]\r\n"
    "; a partir desses pontos aumenta a chance de colete, capacete e armas melhores\r\n"
    "Start=40\r\n"
#endif
    ;

static std::string g_ini;

static int IniInt(const char *sec, const char *key, int def) {
    return (int)GetPrivateProfileIntA(sec, key, def, g_ini.c_str());
}
static std::string IniStr(const char *sec, const char *key, const char *def) {
    char buf[512];
    GetPrivateProfileStringA(sec, key, def, buf, sizeof buf, g_ini.c_str());
    return buf;
}
static float IniFloat(const char *sec, const char *key, float def) {
    char d[32];
    snprintf(d, sizeof d, "%g", def);
    return (float)atof(IniStr(sec, key, d).c_str());
}
static std::vector<int> IniInts(const char *sec, const char *key, const std::vector<int> &def) {
    std::string s = IniStr(sec, key, "");
    if (s.empty()) return def;
    std::vector<int> out;
    const char *p = s.c_str();
    while (*p) {
        char *end;
        long v = strtol(p, &end, 10);
        if (end == p) { ++p; continue; }
        out.push_back((int)v);
        p = end;
    }
    return out.empty() ? def : out;
}

void ConfigLoad() {
    g_ini = g_dir + "Retaliation.ini";
    if (GetFileAttributesA(g_ini.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (FILE *f = fopen(g_ini.c_str(), "wb")) {
            fputs(kDefaultIni, f);
            fclose(f);
            Log("Retaliation.ini criado com valores padrao.");
        }
    }
    Config c;
    c.enabled = IniInt("General", "Enabled", 0);
    c.activeInMissions = IniInt("General", "ActiveInMissions", 0);
    c.mode = IniInt("General", "Mode", 1);
    if (c.mode < 1 || c.mode > 3) c.mode = 3;
    c.directorMode = IniInt("General", "DirectorMode", 1);
    c.debugKeys = IniInt("General", "DebugKeys", 1);
    c.killCivilian = IniInt("Points", "KillCivilian", c.killCivilian);
    c.killCop = IniInt("Points", "KillCop", c.killCop);
    c.killGang = IniInt("Points", "KillGang", c.killGang);
    c.stealVehicle = IniInt("Points", "StealVehicle", c.stealVehicle);
    c.blowVehicle = IniInt("Points", "BlowVehicle", c.blowVehicle);
    c.blowAircraft = IniInt("Points", "BlowAircraft", c.blowAircraft);
    c.hotEnabled = IniInt("HotTimer", "Enabled", 1);
    c.hotPoints = IniInt("HotTimer", "Points", c.hotPoints);
    c.hotSeconds = IniInt("HotTimer", "Seconds", c.hotSeconds);
    c.cycleWanted = IniInt("Cycle", "Wanted", c.cycleWanted);
    c.cycleSeconds = IniInt("Cycle", "Seconds", c.cycleSeconds);
    c.cycleStartLevel = IniInt("Cycle", "StartLevel", c.cycleStartLevel);
    c.cycleJointSeconds = IniInt("Cycle", "JointSeconds", c.cycleJointSeconds);
    c.killHunter = IniInt("Points", "KillHunter", c.killHunter);
    c.decayInterval = IniInt("Points", "DecayInterval", c.decayInterval);
    c.decayAmount = IniInt("Points", "DecayAmount", c.decayAmount);
    c.maxPoints = IniInt("Points", "Max", c.maxPoints);
    c.levelStart = IniInts("Levels", "Start", c.levelStart);
    c.hudEnabled = IniInt("Hud", "Enabled", 1);
    c.hudOffsetX = IniFloat("Hud", "OffsetX", c.hudOffsetX);
    c.hudOffsetY = IniFloat("Hud", "OffsetY", c.hudOffsetY);
    c.hudHeight = IniFloat("Hud", "Height", c.hudHeight);
    c.hudStart = IniFloat("Hud", "BarStart", c.hudStart);
    c.hudEnd = IniFloat("Hud", "BarEnd", c.hudEnd);
    std::vector<int> col = IniInts("Hud", "Color", {c.hudR, c.hudG, c.hudB});
    if (col.size() >= 3) { c.hudR = col[0]; c.hudG = col[1]; c.hudB = col[2]; }
    c.hudShowLevel = IniInt("Hud", "ShowLevel", 0);
    c.footMin = IniFloat("Spawn", "FootMin", c.footMin);
    c.footMax = IniFloat("Spawn", "FootMax", c.footMax);
    c.vehMin = IniFloat("Spawn", "VehMin", c.vehMin);
    c.vehMax = IniFloat("Spawn", "VehMax", c.vehMax);
    c.despawnDist = IniFloat("Spawn", "DespawnDist", c.despawnDist);
    c.arriveTimeout = IniInt("Spawn", "ArriveTimeout", c.arriveTimeout);
    c.firstWaveDelay = IniInt("Spawn", "FirstWaveDelay", c.firstWaveDelay);
    c.blipRadius = IniFloat("Spawn", "BlipRadius", c.blipRadius);
    c.scaleStart = IniInt("Scaling", "Start", c.scaleStart);
    c.scaleStep = IniInt("Scaling", "Step", c.scaleStep);
    c.scalePerStep = IniFloat("Scaling", "PerStep", c.scalePerStep);
    c.scaleMinGain = IniFloat("Scaling", "MinGain", c.scaleMinGain);
    c.flowRampSeconds = IniInt("Flow", "RampSeconds", c.flowRampSeconds);
    c.flowRampStart = IniInt("Flow", "RampStart", c.flowRampStart);
    c.flowGapScale = IniFloat("Flow", "GapScale", c.flowGapScale);
    c.flowNearBase = IniInt("Flow", "NearBase", c.flowNearBase);
    c.flowFarFootPct = IniInt("Flow", "FarFootChance", c.flowFarFootPct);
    c.remoteRoadDist = IniFloat("Flow", "RemoteRoadDist", c.remoteRoadDist);
    c.remoteHeliBonus = IniInt("Flow", "RemoteHeliBonus", c.remoteHeliBonus);
    c.waterBoatPct = IniInt("Flow", "WaterBoatChance", c.waterBoatPct);
    for (int i = 0; i < 8; ++i) {
        char sec[16];
        snprintf(sec, sizeof sec, "Level%d", i + 1);
        Config::Lv &l = c.lv[i];
        l.interval = IniInt(sec, "Interval", l.interval);
        l.foot = IniInt(sec, "Foot", l.foot);
        l.vehicles = IniInt(sec, "Vehicles", l.vehicles);
        l.accuracy = IniInt(sec, "Accuracy", l.accuracy);
        l.health = IniInt(sec, "Health", l.health);
        l.armor = IniInt(sec, "Armor", l.armor);
        l.maxAlive = IniInt(sec, "MaxAlive", l.maxAlive);
    }
    c.rbEnabled = IniInt("Roadblock", "Enabled", 1);
    c.rbMinLevel = IniInt("Roadblock", "MinLevel", c.rbMinLevel);
    c.rbChance = IniInt("Roadblock", "Chance", c.rbChance);
    c.jugEnabled = IniInt("Juggernaut", "Enabled", 1);
    c.jugMinLevel = IniInt("Juggernaut", "MinLevel", c.jugMinLevel);
    c.jugChance = IniInt("Juggernaut", "Chance", c.jugChance);
    c.jugHealth = IniInt("Juggernaut", "Health", c.jugHealth);
    c.jugMinigunPct = IniInt("Juggernaut", "MinigunChance", c.jugMinigunPct);
    c.rangeShort = IniFloat("Range", "Short", c.rangeShort);
    c.rangeMedium = IniFloat("Range", "Medium", c.rangeMedium);
    {
        std::vector<int> hc = IniInts("Heli", "Chance", std::vector<int>(c.heliChance, c.heliChance + 8));
        for (int i = 0; i < 8 && i < (int)hc.size(); ++i) c.heliChance[i] = hc[i];
    }
    c.heliMax = IniInt("Heli", "Max", c.heliMax);
    c.heliMaxTop = IniInt("Heli", "MaxTop", c.heliMaxTop);
    c.heliMaxAir = IniInt("Heli", "MaxAir", c.heliMaxAir);
    c.heliAttackPct = IniInt("Heli", "AttackChance", c.heliAttackPct);
    c.heliRappelPct = IniInt("Heli", "RappelChance", c.heliRappelPct);
    c.heliLandPct = IniInt("Heli", "LandChance", c.heliLandPct);
    c.heliAttackMax = IniInt("Heli", "AttackMax", c.heliAttackMax);
    c.heliSecondPct = IniInt("Heli", "SecondChance", c.heliSecondPct);
    c.heliSecondMax = IniInt("Heli", "SecondMax", c.heliSecondMax);
    c.heliThirdPct = IniInt("Heli", "ThirdChance", c.heliThirdPct);
    c.heliThirdMax = IniInt("Heli", "ThirdMax", c.heliThirdMax);
    c.sniperPct = IniInt("Weapons", "SniperChance", c.sniperPct);
    c.heavyPct = IniInt("Weapons", "HeavyChance", c.heavyPct);
    c.heavyMax = IniInt("Weapons", "HeavyMax", c.heavyMax);
    c.explosiveAcc = IniInt("Weapons", "ExplosiveAccuracy", c.explosiveAcc);
    c.jugMax = IniInt("Juggernaut", "Max", c.jugMax);
    c.jugMinigunMax = IniInt("Juggernaut", "MinigunMax", c.jugMinigunMax);
    c.tankMax = IniInt("Tank", "Max", c.tankMax);
    c.jetMax = IniInt("Jet", "Max", c.jetMax);
    c.tankEnabled = IniInt("Tank", "Enabled", 1);
    c.tankMinLevel = IniInt("Tank", "MinLevel", c.tankMinLevel);
    c.tankChance = IniInt("Tank", "Chance", c.tankChance);
    c.jetEnabled = IniInt("Jet", "Enabled", 1);
    c.jetMinLevel = IniInt("Jet", "MinLevel", c.jetMinLevel);
    c.jetChance = IniInt("Jet", "Chance", c.jetChance);
    c.jetDuration = IniInt("Jet", "Duration", c.jetDuration);
    c.jetHydraPct = IniInt("Jet", "HydraChance", c.jetHydraPct);
    c.escStart = IniInt("Escalation", "Start", c.escStart);
    c.escPerPoint = IniFloat("Escalation", "PerPoint", c.escPerPoint);
    c.topBonusPerPoint = IniFloat("TopBonus", "PerPoint", c.topBonusPerPoint);
    c.gearStart = IniInt("Gear", "Start", c.gearStart);
    // roupas do Juggernaut copiadas do jogo (ver OutfitCapture): uma por protagonista + a antiga da v8
    for (int s = 0; s < 4; ++s) {
        const char *sec = kOutfitSection[s];
        Config::Outfit &o = c.jugOutfits[s];
        std::string m = IniStr(sec, "Model", "");
        o.model = (unsigned)strtoul(m.c_str(), nullptr, 10);
        o.valid = o.model != 0;
        for (int i = 0; i < 12; ++i) {
            char k[8]; snprintf(k, sizeof k, "C%d", i);
            std::vector<int> v = IniInts(sec, k, {-1, 0, 0});
            for (int j = 0; j < 3; ++j) o.comp[i][j] = j < (int)v.size() ? v[j] : 0;
        }
        for (int i = 0; i < 8; ++i) {
            char k[8]; snprintf(k, sizeof k, "P%d", i);
            std::vector<int> v = IniInts(sec, k, {-1, 0});
            for (int j = 0; j < 2; ++j) o.prop[i][j] = j < (int)v.size() ? v[j] : 0;
        }
    }
    g_cfg = c;

    std::string lv;
    for (int v : g_cfg.levelStart) lv += std::to_string(v) + " ";
    Log("Config: niveis comecam em [ %s], max %d, decaimento %d a cada %ds, timer de %ds acima de %d pts.",
        lv.c_str(), g_cfg.maxPoints, g_cfg.decayAmount, g_cfg.decayInterval, g_cfg.hotSeconds, g_cfg.hotPoints);
}

static void IniWrite(const char *sec, const char *key, const char *v) {
    WritePrivateProfileStringA(sec, key, v, g_ini.c_str());
}

// secoes do .ini: 0 = antiga (v8), 1 = Michael, 2 = Franklin, 3 = Trevor
const char *kOutfitSection[4] = {"JuggernautOutfit", "JuggernautOutfit_Michael", "JuggernautOutfit_Franklin",
                                 "JuggernautOutfit_Trevor"};

// v8/v9.3: copia a roupa que o protagonista esta vestindo (armadura pesada na missao The Paleto Score).
// Cada protagonista tem a sua secao; o Juggernaut sorteia entre as que existirem.
void OutfitCapture(Ped p) {
    char buf[64];
    int slot = g_st.ch == CH_MICHAEL ? 1 : g_st.ch == CH_FRANKLIN ? 2 : g_st.ch == CH_TREVOR ? 3 : 0;
    const char *sec = kOutfitSection[slot];
    snprintf(buf, sizeof buf, "%u", (unsigned)ENTITY::GET_ENTITY_MODEL(p));
    IniWrite(sec, "Model", buf);
    for (int i = 0; i < 12; ++i) {
        char k[8]; snprintf(k, sizeof k, "C%d", i);
        snprintf(buf, sizeof buf, "%d,%d,%d", PED::GET_PED_DRAWABLE_VARIATION(p, i), PED::GET_PED_TEXTURE_VARIATION(p, i),
                 PED::GET_PED_PALETTE_VARIATION(p, i));
        IniWrite(sec, k, buf);
    }
    for (int i = 0; i < 8; ++i) {
        char k[8]; snprintf(k, sizeof k, "P%d", i);
        snprintf(buf, sizeof buf, "%d,%d", PED::GET_PED_PROP_INDEX(p, i), PED::GET_PED_PROP_TEXTURE_INDEX(p, i));
        IniWrite(sec, k, buf);
    }
    Log("Roupa do Juggernaut copiada (%s, modelo %u) em [%s].", kCharName[g_st.ch],
        (unsigned)ENTITY::GET_ENTITY_MODEL(p), sec);
}

void OutfitApply(Ped p, const Config::Outfit &o) {
    for (int i = 0; i < 12; ++i)
        if (o.comp[i][0] >= 0) PED::SET_PED_COMPONENT_VARIATION(p, i, o.comp[i][0], o.comp[i][1], o.comp[i][2]);
    PED::CLEAR_ALL_PED_PROPS(p);
    for (int i = 0; i < 8; ++i)
        if (o.prop[i][0] >= 0) PED::SET_PED_PROP_INDEX(p, i, o.prop[i][0], o.prop[i][1], TRUE);
}

void ConfigSetInt(const char *sec, const char *key, int v) {
    char buf[16];
    snprintf(buf, sizeof buf, "%d", v);
    WritePrivateProfileStringA(sec, key, buf, g_ini.c_str());
}

int LevelOf(int points) {
    int lv = 0;
    for (size_t i = 0; i < g_cfg.levelStart.size(); ++i)
        if (points >= g_cfg.levelStart[i]) lv = (int)i + 1;
    return lv;
}

