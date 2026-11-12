#pragma once
#include "common.h"

enum Region { RG_CITY = 0, RG_COUNTRY = 1 };

struct Faction {
    const char *id, *name;
    std::vector<Hash> peds, cars, bikes, bicycles, boats;
    std::vector<Hash> helis, attackHelis; // v5
    int bikePct = 0;
    bool policeAlly = false;
    Hash group = 0, ambient = 0;
};
extern std::vector<Faction> g_factions;

void FactionsInit();
void RelationsApply();
Region RegionAt(float x, float y);
Faction *FactionFor(int level, Region r);
Hash WeaponFor(int level, int betterPct = 0);
Hash SniperFor(int level);
Hash HeavyFor(int level);
void ApplyWeaponMods(Ped ped, Hash weapon, int level); // v10

int Rand(int n);          // 0..n-1
float RandF(float a, float b);

// spawner.cpp
void SpawnerInit();
void SpawnerTick(Ped player);
void DespawnAll(bool hard, const char *why);
int HuntersAlive();
Ped NearestHunterTo(const Vector3 &at, float maxD, float zoneR = 1e9f); // v11.1; v11.2: so os que estao a ate zoneR m do jogador
Ped NearestCopTo(const Vector3 &at, float maxD);    // v11.1 (points.cpp)

// points.cpp: retaliadores nao dao pontos
void PointsIgnorePed(Ped p);
void PointsIgnoreVeh(Vehicle v);
// v11: Desordem - retaliadores neutros com o jogador (calm) e contra os civis
void RelationsDisorder(bool on, bool calm);
void RelationsPolice(bool war);
Hash ArmedCivGroup(); // v11.1: grupo dos civis armados da Desordem // v11.1: false = retaliadores e policia do mesmo lado (modo 1)
