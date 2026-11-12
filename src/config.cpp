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
    "Enabled=1\r\n"
    "; 1 = emboscadas continuam durante missoes da historia\r\n"
    "ActiveInMissions=0\r\n"
    "; Teclas de teste: Ctrl+PageUp (+10 pontos), Ctrl+PageDown (-10), Ctrl+End (zera)\r\n"
    "DebugKeys=1\r\n"
    "\r\n"
    "[Points]\r\n"
    "KillCivilian=4\r\n"
    "KillCop=5\r\n"
    "KillGang=2\r\n"
    "StealVehicle=2\r\n"
    "BlowVehicle=4\r\n"
    "; segundos sem crimes para perder pontos, e quanto perde\r\n"
    "DecayInterval=45\r\n"
    "DecayAmount=5\r\n"
    "Max=100\r\n"
    "\r\n"
    "[Levels]\r\n"
    "; pontos em que cada nivel comeca (nivel 1, 2, 3...)\r\n"
    "Start=10,21,32,43,54,65,76,88\r\n"
    "\r\n"
    "[MaxHeat]\r\n"
    "Enabled=1\r\n"
    "Points=95\r\n"
    "Seconds=120\r\n"
    "WantedLevel=5\r\n"
    "Cooldown=60\r\n"
    "\r\n"
    "[Hud]\r\n"
    "Enabled=1\r\n"
    "; ajuste fino da posicao da barra (fracao da tela)\r\n"
    "OffsetX=0.0\r\n"
    "OffsetY=0.0115\r\n"
    "Height=0.0055\r\n"
    "; inicio e fim da barra, em fracao da largura do minimapa (barra do especial = 0.75 a 1.0)\r\n"
    "BarStart=0.75\r\n"
    "BarEnd=1.0\r\n"
    "Color=200,25,25\r\n"
    "ShowLevel=1\r\n"
#ifdef RET_SPAWN
    "\r\n"
    "[Spawn]\r\n"
    "; limite de retaliadores vivos ao mesmo tempo\r\n"
    "MaxAlive=16\r\n"
    "; distancias (metros): grupos a pe, veiculos, remocao\r\n"
    "FootMin=60\r\n"
    "FootMax=100\r\n"
    "VehMin=170\r\n"
    "VehMax=240\r\n"
    "DespawnDist=350\r\n"
    "; segundos para chegar perto do jogador antes de ser removido\r\n"
    "ArriveTimeout=150\r\n"
    "; segundos ate a primeira wave depois de entrar no nivel 1\r\n"
    "FirstWaveDelay=8\r\n"
    "\r\n"
    "; Por nivel: Interval (s entre waves), Foot (inimigos a pe), Vehicles, Accuracy (0-100),\r\n"
    "; Health (200 = normal), Armor (0-100)\r\n"
    "[Level1]\r\nInterval=50\r\nFoot=2\r\nVehicles=1\r\nAccuracy=15\r\nHealth=150\r\nArmor=0\r\n"
    "[Level2]\r\nInterval=45\r\nFoot=3\r\nVehicles=1\r\nAccuracy=20\r\nHealth=175\r\nArmor=0\r\n"
    "[Level3]\r\nInterval=45\r\nFoot=3\r\nVehicles=2\r\nAccuracy=25\r\nHealth=200\r\nArmor=10\r\n"
    "[Level4]\r\nInterval=42\r\nFoot=4\r\nVehicles=2\r\nAccuracy=30\r\nHealth=200\r\nArmor=25\r\n"
    "[Level5]\r\nInterval=40\r\nFoot=4\r\nVehicles=2\r\nAccuracy=35\r\nHealth=225\r\nArmor=50\r\n"
    "[Level6]\r\nInterval=38\r\nFoot=4\r\nVehicles=3\r\nAccuracy=40\r\nHealth=250\r\nArmor=75\r\n"
    "[Level7]\r\nInterval=35\r\nFoot=5\r\nVehicles=3\r\nAccuracy=45\r\nHealth=275\r\nArmor=100\r\n"
    "[Level8]\r\nInterval=30\r\nFoot=5\r\nVehicles=3\r\nAccuracy=50\r\nHealth=300\r\nArmor=100\r\n"
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
    "Chance=20\r\n"
    "Health=3000\r\n"
    "\r\n"
    "[TopBonus]\r\n"
    "; % extra de chance dos especiais por ponto acima do inicio do ultimo nivel\r\n"
    "PerPoint=1.5\r\n"
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
    c.enabled = IniInt("General", "Enabled", 1);
    c.activeInMissions = IniInt("General", "ActiveInMissions", 0);
    c.debugKeys = IniInt("General", "DebugKeys", 1);
    c.killCivilian = IniInt("Points", "KillCivilian", c.killCivilian);
    c.killCop = IniInt("Points", "KillCop", c.killCop);
    c.killGang = IniInt("Points", "KillGang", c.killGang);
    c.stealVehicle = IniInt("Points", "StealVehicle", c.stealVehicle);
    c.blowVehicle = IniInt("Points", "BlowVehicle", c.blowVehicle);
    c.decayInterval = IniInt("Points", "DecayInterval", c.decayInterval);
    c.decayAmount = IniInt("Points", "DecayAmount", c.decayAmount);
    c.maxPoints = IniInt("Points", "Max", c.maxPoints);
    c.levelStart = IniInts("Levels", "Start", c.levelStart);
    c.maxHeatEnabled = IniInt("MaxHeat", "Enabled", 1);
    c.maxHeatPoints = IniInt("MaxHeat", "Points", c.maxHeatPoints);
    c.maxHeatSeconds = IniInt("MaxHeat", "Seconds", c.maxHeatSeconds);
    c.maxHeatWanted = IniInt("MaxHeat", "WantedLevel", c.maxHeatWanted);
    c.maxHeatCooldown = IniInt("MaxHeat", "Cooldown", c.maxHeatCooldown);
    c.hudEnabled = IniInt("Hud", "Enabled", 1);
    c.hudOffsetX = IniFloat("Hud", "OffsetX", c.hudOffsetX);
    c.hudOffsetY = IniFloat("Hud", "OffsetY", c.hudOffsetY);
    c.hudHeight = IniFloat("Hud", "Height", c.hudHeight);
    c.hudStart = IniFloat("Hud", "BarStart", c.hudStart);
    c.hudEnd = IniFloat("Hud", "BarEnd", c.hudEnd);
    std::vector<int> col = IniInts("Hud", "Color", {c.hudR, c.hudG, c.hudB});
    if (col.size() >= 3) { c.hudR = col[0]; c.hudG = col[1]; c.hudB = col[2]; }
    c.hudShowLevel = IniInt("Hud", "ShowLevel", 1);
    c.maxAlive = IniInt("Spawn", "MaxAlive", c.maxAlive);
    c.footMin = IniFloat("Spawn", "FootMin", c.footMin);
    c.footMax = IniFloat("Spawn", "FootMax", c.footMax);
    c.vehMin = IniFloat("Spawn", "VehMin", c.vehMin);
    c.vehMax = IniFloat("Spawn", "VehMax", c.vehMax);
    c.despawnDist = IniFloat("Spawn", "DespawnDist", c.despawnDist);
    c.arriveTimeout = IniInt("Spawn", "ArriveTimeout", c.arriveTimeout);
    c.firstWaveDelay = IniInt("Spawn", "FirstWaveDelay", c.firstWaveDelay);
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
    }
    c.rbEnabled = IniInt("Roadblock", "Enabled", 1);
    c.rbMinLevel = IniInt("Roadblock", "MinLevel", c.rbMinLevel);
    c.rbChance = IniInt("Roadblock", "Chance", c.rbChance);
    c.jugEnabled = IniInt("Juggernaut", "Enabled", 1);
    c.jugMinLevel = IniInt("Juggernaut", "MinLevel", c.jugMinLevel);
    c.jugChance = IniInt("Juggernaut", "Chance", c.jugChance);
    c.jugHealth = IniInt("Juggernaut", "Health", c.jugHealth);
    c.topBonusPerPoint = IniFloat("TopBonus", "PerPoint", c.topBonusPerPoint);
    g_cfg = c;

    std::string lv;
    for (int v : g_cfg.levelStart) lv += std::to_string(v) + " ";
    Log("Config: niveis comecam em [ %s], max %d, decaimento %d a cada %ds, calor maximo %d pts por %ds.",
        lv.c_str(), g_cfg.maxPoints, g_cfg.decayAmount, g_cfg.decayInterval, g_cfg.maxHeatPoints,
        g_cfg.maxHeatSeconds);
}

int LevelOf(int points) {
    int lv = 0;
    for (size_t i = 0; i < g_cfg.levelStart.size(); ++i)
        if (points >= g_cfg.levelStart[i]) lv = (int)i + 1;
    return lv;
}

