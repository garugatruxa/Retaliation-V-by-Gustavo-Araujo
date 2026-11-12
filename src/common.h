#pragma once
// Retaliation V - cabecalho comum (log, config, utilidades).
#include "../sdk/inc/natives.h"
#include <string>
#include <vector>

#define RETALIATION_VERSION "0.6"

// ---------- log ----------
void LogOpen(const std::string &path);
void Log(const char *fmt, ...);

// ---------- config ----------
struct Config {
    // [General]
    bool enabled = true;
    bool activeInMissions = false;
    bool debugKeys = true;
    // [Points]
    int killCivilian = 4, killCop = 5, killGang = 2, stealVehicle = 2, blowVehicle = 4, blowAircraft = 6;
    int decayInterval = 45, decayAmount = 5, maxPoints = 100;
    // [Levels] pontos em que cada nivel comeca (nivel 1..N)
    std::vector<int> levelStart{10, 21, 32, 43, 54, 65, 76, 88};
    // [Hud]
    bool hudEnabled = true;
    float hudOffsetX = 0.0f, hudOffsetY = 0.0085f, hudHeight = 0.0055f;
    float hudStart = 0.75f, hudEnd = 1.0f; // trecho da largura do minimapa (barra do especial = ultimo quarto)
    int hudR = 200, hudG = 25, hudB = 25;
    bool hudShowLevel = false;
    // [Spawn]
    float footMin = 80, footMax = 125, vehMin = 260, vehMax = 350, despawnDist = 480;
    int arriveTimeout = 150, firstWaveDelay = 8;
    float blipRadius = 120; // blips vermelhos so aparecem nesse raio
    // [LevelN] 1..8
    // maxAlive = limite de retaliadores vivos no nivel (a pe + em veiculos), 28 no nivel maximo
    struct Lv { int interval, foot, vehicles, accuracy, health, armor, maxAlive; };
    Lv lv[8] = {{50, 2, 1, 15, 150, 0, 10},  {45, 3, 1, 20, 175, 0, 12},  {45, 3, 2, 25, 200, 10, 14},
                {42, 4, 2, 30, 200, 25, 16}, {40, 4, 2, 35, 225, 50, 18}, {38, 5, 3, 40, 250, 75, 21},
                {35, 6, 3, 45, 275, 100, 24}, {30, 6, 4, 50, 300, 100, 28}};
    // [HotTimer] acima desses pontos por X segundos sem morrer/ser preso: perde metade dos pontos
    bool hotEnabled = true;
    int hotPoints = 80, hotSeconds = 300;
    // [WantedRelief] 5 estrelas por X segundos: cai para N estrelas
    bool reliefEnabled = true;
    int reliefSeconds = 300, reliefTo = 2;
    // [Roadblock]
    bool rbEnabled = true;
    int rbMinLevel = 3, rbChance = 30;
    // [Juggernaut]
    bool jugEnabled = true;
    int jugMinLevel = 7, jugChance = 10, jugHealth = 3000, jugMinigunPct = 10, jugMax = 30, jugMinigunMax = 25;
    // [TopBonus] % extra por ponto acima do inicio do ultimo nivel
    float topBonusPerPoint = 1.5f;
    // [Gear] a partir desses pontos aumenta a chance de colete, capacete e armas melhores
    int gearStart = 40;
    // [Range] v5: alcance (m) em que cada classe de arma comeca a atirar
    float rangeShort = 45, rangeMedium = 110;
    // [Heli] v5: chance (%) de helicoptero por wave em cada nivel, e limite ao mesmo tempo
    int heliChance[8] = {0, 0, 8, 12, 16, 22, 28, 35};
    int heliMax = 1, heliMaxTop = 3, heliMaxAir = 3, heliAttackPct = 10, heliRappelPct = 40, heliLandPct = 30;
    int heliAttackMax = 20, heliSecondPct = 10, heliSecondMax = 20, heliThirdPct = 3, heliThirdMax = 8;
    // [Weapons] chance (%) por wave de vir um sniper e de vir alguem com RPG/lanca-granadas (com teto)
    int sniperPct = 50, heavyPct = 12, heavyMax = 20;
    int explosiveAcc = 8; // mira (0-100) de quem usa explosivos: RPG, lanca-granadas, tanque, jato, helicoptero de ataque
    // [Escalation] v5: a partir desses pontos, mais especiais (Juggernaut, minigun, ataque)
    // [Tank] / [Jet] v6: so no ultimo nivel, um de cada por vez
    bool tankEnabled = true, jetEnabled = true;
    int tankMinLevel = 8, tankChance = 8, tankMax = 18, jetMinLevel = 8, jetChance = 5, jetMax = 15, jetDuration = 90,
        jetHydraPct = 25;
    int escStart = 70;
    float escPerPoint = 1.0f;
};
extern Config g_cfg;
extern std::string g_dir; // pasta do .asi (raiz do jogo)
void ConfigLoad();

// ---------- estado ----------
enum Character { CH_MICHAEL, CH_FRANKLIN, CH_TREVOR, CH_OTHER, CH_COUNT };
extern const char *kCharName[CH_COUNT];

struct State {
    int points[CH_COUNT] = {};
    Character ch = CH_OTHER;
    int now = 0;              // GET_GAME_TIMER
    int lastCrime = 0;        // ultimo crime (ms)
    int lastDecay = 0;        // ultimo decaimento (ms)
    int maxHeatSince = -1;    // quando entrou na faixa de calor maximo (-1 = fora)
    int cooldownUntil = 0;    // retaliacao desligada ate esse tempo
    int hotSince = -1;        // desde quando esta acima de HotTimer.Points
    int wantedMaxSince = -1;  // desde quando esta com 5 estrelas
    bool paused = false;      // emboscadas pausadas (missao, cutscene, interior)
    const char *pauseReason = "";
    int &Pts() { return points[ch]; }
};
extern State g_st;

int LevelOf(int points);

// points.cpp
void PointsReset();
void PointsSeed();
void PointsTick(Ped player, Player pid);
void AddPoints(int amount, const char *why);

// hud.cpp
void HudTick();
void Notify(const char *text);

