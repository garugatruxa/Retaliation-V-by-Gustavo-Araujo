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

int Rand(int n);          // 0..n-1
float RandF(float a, float b);

// spawner.cpp
void SpawnerInit();
void SpawnerTick(Ped player);
void DespawnAll(bool hard, const char *why);
int HuntersAlive();

// points.cpp: retaliadores nao dao pontos
void PointsIgnorePed(Ped p);
void PointsIgnoreVeh(Vehicle v);
