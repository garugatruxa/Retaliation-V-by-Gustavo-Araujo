#pragma once
// Retaliation V - cabecalho comum (log, config, utilidades).
#include "../sdk/inc/natives.h"
#include <string>
#include <vector>

#define RETALIATION_VERSION "0.3"

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
    int killCivilian = 4, killCop = 5, killGang = 2, stealVehicle = 2, blowVehicle = 4;
    int decayInterval = 45, decayAmount = 5, maxPoints = 100;
    // [Levels] pontos em que cada nivel comeca (nivel 1..N)
    std::vector<int> levelStart{10, 21, 32, 43, 54, 65, 76, 88};
    // [MaxHeat]
    bool maxHeatEnabled = true;
    int maxHeatPoints = 95, maxHeatSeconds = 120, maxHeatWanted = 5, maxHeatCooldown = 60;
    // [Hud]
    bool hudEnabled = true;
    float hudOffsetX = 0.0f, hudOffsetY = 0.0085f, hudHeight = 0.0055f;
    float hudStart = 0.75f, hudEnd = 1.0f; // trecho da largura do minimapa (barra do especial = ultimo quarto)
    int hudR = 200, hudG = 25, hudB = 25;
    bool hudShowLevel = false;
    // [Spawn]
    int maxAlive = 16;
    float footMin = 60, footMax = 100, vehMin = 170, vehMax = 240, despawnDist = 350;
    int arriveTimeout = 150, firstWaveDelay = 8;
    float blipRadius = 120; // blips vermelhos so aparecem nesse raio
    // [LevelN] 1..8
    struct Lv { int interval, foot, vehicles, accuracy, health, armor; };
    Lv lv[8] = {{50, 2, 1, 15, 150, 0},  {45, 3, 1, 20, 175, 0},  {45, 3, 2, 25, 200, 10},
                {42, 4, 2, 30, 200, 25}, {40, 4, 2, 35, 225, 50}, {38, 4, 3, 40, 250, 75},
                {35, 5, 3, 45, 275, 100}, {30, 5, 3, 50, 300, 100}};
    // [Roadblock]
    bool rbEnabled = true;
    int rbMinLevel = 3, rbChance = 30;
    // [Juggernaut]
    bool jugEnabled = true;
    int jugMinLevel = 7, jugChance = 20, jugHealth = 3000;
    // [TopBonus] % extra por ponto acima do inicio do ultimo nivel
    float topBonusPerPoint = 1.5f;
    // [Gear] a partir desses pontos aumenta a chance de colete, capacete e armas melhores
    int gearStart = 40;
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

