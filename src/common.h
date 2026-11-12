#pragma once
// Retaliation V - cabecalho comum (log, config, utilidades).
#include "../sdk/inc/natives.h"
#include <string>
#include <vector>

#define RETALIATION_VERSION "11.2"

// ---------- log ----------
void LogOpen(const std::string &path);
void Log(const char *fmt, ...);

// ---------- config ----------
struct Config {
    // [General]
    bool enabled = false; // v9.2: comeca desligado (liga pelo menu, tecla Insert)
    bool activeInMissions = false;
    // v8: 1 = junto com a policia (original), 2 = substitui a policia, 3 = ciclo (policia -> Retaliation -> livre)
    // v11: 4 = Desordem (policia x retaliadores, o jogador so e atacado de perto ou se atacar primeiro)
    int mode = 1; // v9.2: padrao = junto com a policia
    bool directorMode = true; // v7: 1 = Retaliation livre no Modo Diretor (comeca zerado); 0 = limpa e pausa ao entrar
    bool debugKeys = true;
    bool levelLock = false; // v10: trava de nivel (menu): os pontos so mudam pelas teclas PageUp/PageDown
    // [Points]
    // v10: escala de 0 a 500 pontos
    int killCivilian = 4, killCop = 4, killGang = 4, stealVehicle = 4, blowVehicle = 8, blowAircraft = 12;
    int decayInterval = 45, decayAmount = 15, maxPoints = 500;
    // [Levels] pontos em que cada nivel comeca (nivel 1..N)
    std::vector<int> levelStart{10, 80, 140, 200, 260, 320, 380, 440};
    // [Hud]
    bool hudEnabled = true;
    float hudOffsetX = 0.0f, hudOffsetY = 0.0085f, hudHeight = 0.0055f;
    float hudStart = 0.0f, hudEnd = 1.0f; // v7: largura inteira do minimapa, alinhada com as barras de vida/especial // trecho da largura do minimapa (barra do especial = ultimo quarto)
    int hudR = 200, hudG = 25, hudB = 25;
    bool hudShowLevel = false;
    // [Spawn]
    float footMin = 95, footMax = 150, vehMin = 320, vehMax = 450, despawnDist = 600; // v9: mais longe
    // v9: longe de estrada (> RemoteRoadDist m) = +RemoteHeliBonus % de helicoptero; agua perto = WaterBoatChance % de barco
    float remoteRoadDist = 150;
    int remoteHeliBonus = 30, waterBoatPct = 35;
    int arriveTimeout = 150, firstWaveDelay = 8;
    float blipRadius = 180; // v10: era 120. blips vermelhos so aparecem nesse raio
    // [Scaling] v9: a partir de Start pontos, cada Step pontos acima tiram PerStep de cada ganho (minimo MinGain)
    int scaleStart = 250, scaleStep = 25;
    float scalePerStep = 0.25f, scaleMinGain = 0.5f;
    // [Flow] v9: chegada aos poucos
    int flowRampSeconds = 180, flowRampStart = 4, flowNearBase = 3, flowFarFootPct = 35;
    float flowGapScale = 1.0f;
    // [LevelN] 1..8
    // maxAlive = limite de retaliadores vivos no nivel (a pe + em veiculos), 28 no nivel maximo
    struct Lv { int interval, foot, vehicles, accuracy, health, armor, maxAlive; };
    Lv lv[8] = {{50, 2, 1, 15, 150, 0, 10},  {45, 3, 1, 20, 175, 0, 12},  {45, 3, 2, 25, 200, 10, 14},
                {42, 4, 2, 30, 200, 25, 16}, {40, 4, 2, 35, 225, 50, 18}, {38, 5, 3, 40, 250, 75, 21},
                {35, 6, 3, 45, 275, 100, 24}, {30, 6, 4, 50, 300, 100, 30}};
    // [HotTimer] acima desses pontos por X segundos sem morrer/ser preso: perde metade dos pontos
    bool hotEnabled = true;
    int hotPoints = 350, hotSeconds = 300;
    // [Cycle] v7: com Wanted estrelas por Seconds, a policia desiste e o Retaliation comeca em StartLevel
    int cycleWanted = 5, cycleSeconds = 300, cycleStartLevel = 2; // v10: 5 minutos de policia
    int cycleJointSeconds = 0; // v10: 0 = sem fase conjunta (alterna direto). >0 = policia ignora o jogador e enfrenta os retaliadores
    int killHunter = 4; // [Points] retaliador morto
    // [Roadblock]
    bool rbEnabled = true;
    int rbMinLevel = 3, rbChance = 30;
    // [Juggernaut]
    bool jugEnabled = true;
    // [JuggernautOutfit] v8: roupa copiada do jogo (armadura do assalto de Paleto Bay vestida por um protagonista)
    struct Outfit { bool valid = false; unsigned model = 0; int comp[12][3] = {}; int prop[8][2] = {}; };
    Outfit jugOutfits[4]; // v9.3: 0 = antiga (v8), 1 = Michael, 2 = Franklin, 3 = Trevor
    // v10.1: o pesado vem mais (nivel 6+, chance 18 ate 40)
    int jugMinLevel = 6, jugChance = 18, jugHealth = 2000, jugMinigunPct = 10, jugMax = 40, jugMinigunMax = 25;
    // v10.1: [Heavy] Juggernaut leve de cada faccao (niveis 3-6), 1000 de vida, armas menores
    bool jugLightEnabled = true;
    int jugLightMinLevel = 3, jugLightMaxLevel = 6, jugLightChance = 12, jugLightMax = 25, jugLightHealth = 1000;
    // v10: modelo do Juggernaut (NPC blindado do Online, todo preto) e icone de caveira vermelha no mapa
    std::string jugModel = "mp_m_freemode_01"; // v10: personagem do Online com o traje pesado "Noturno pesado"
    int jugBlip = 303;
    int jugMinigunAcc = 0; // v10: minigun do Juggernaut: mira minima = rajada bem espalhada
    // v10: mira dos helicopteros (atiradores, torretas e piloto) - "stormtrooper"
    int heliAccuracy = 2;
    // v10: tanque: segundos entre tiros do canhao e altura do tanque preso no Cargobob
    int tankShotInterval = 15; // v10.1: era 10
    int khanChance = 2; // v10: % fixa por wave do tanque especial Khanjali (nivel 8 ate o maximo)
    float tankHangZ = -4.6f;
    // v10: [Vehicles] veiculos com torreta (com a protecao contra o jogador ser tirado da torreta)
    bool turretVehicles = true;
    // v10.1: [VehicleGuns] armas de veiculo (helicopteros armados, avioes, torretas) em rajadas:
    // FireSeconds liberadas e RestSeconds travadas
    int vehGunFire = 3, vehGunRest = 8;
    // v10.1: [Population] civis e transito continuam normais com o jogador procurado ou com a retaliacao ativa
    // (multiplicadores por frame; 1.0 = normal do jogo). WithPolicePct: % do limite de retaliadores quando a
    // policia tambem esta em cima do jogador, para sobrar espaco para civis e policia
    bool popKeep = true;
    float popPeds = 1.0f, popTraffic = 1.0f;
    int popWithPolicePct = 75;
    // v11: [Disorder] modo 4 (Desordem)
    int disStartLevel = 3;     // nivel inicial sem nivel escolhido no menu
    int disHoldSeconds = 60;   // nada baixa no comeco (e ao escolher o nivel no menu)
    float disFightRadius = 50.f; // v11.2: so se enfrentam ate essa distancia do jogador; mais longe, vem ate ele
    int disAggroSeconds = 60;  // quanto tempo um lado fica contra o jogador depois que ele ataca esse lado
    float disDeathRadius = 200.f; // mortes e destruicao de qualquer um ate essa distancia sobem a pressao
    int disDeathPts = 2, disVehPts = 4; // pontos por morte / veiculo destruido que nao foram do jogador
    int disArmedCivPct = 15;   // % dos civis com arma curta
    bool disRestartOnDeath = true;
    int disRestartDelay = 10;  // segundos depois de morrer/ser preso para recomecar
    // v10: [WeaponMods] chance (%) de a arma vir com acessorios: Base no nivel 1 + PerLevel por nivel acima
    bool modsEnabled = true;
    int modsBase = 10, modsPerLevel = 10;
    // v10: [Paradrop] evento raro do Titan com paraquedistas
    int paraMinLevel = 7, paraChance = 10, paraFewAlive = 4, paraCooldown = 180;
    // [TopBonus] % extra por ponto acima do inicio do ultimo nivel
    float topBonusPerPoint = 0.3f;
    // [Gear] a partir desses pontos aumenta a chance de colete, capacete e armas melhores
    int gearStart = 200;
    // [Range] v5: alcance (m) em que cada classe de arma comeca a atirar
    // v10: lutas mais proximas. Sniper e RPG acima dos outros; veiculos e aeronaves com limite proprio
    float rangeShort = 35, rangeMedium = 70, rangeSniper = 130, rangeRpg = 110, rangeVehicle = 100, rangeAir = 220;
    int dragPct = 60; // v10: % dos inimigos comuns que arrastam companheiros feridos (a elite sempre arrasta)
    // [Heli] v5: chance (%) de helicoptero por wave em cada nivel, e limite ao mesmo tempo
    int heliChance[8] = {0, 0, 8, 12, 16, 22, 28, 35};
    int heliMax = 1, heliMaxTop = 3, heliMaxAir = 3, heliAttackPct = 3, heliRappelPct = 50, heliLandPct = 30;
    int heliAttackMinLevel = 7; // v10: helicopteros armados so a partir deste nivel
    int heliAttackMax = 6, heliSecondPct = 10, heliSecondMax = 20, heliThirdPct = 3, heliThirdMax = 8;
    // [Weapons] chance (%) por wave de vir um sniper e de vir alguem com RPG/lanca-granadas (com teto)
    int sniperPct = 50, heavyPct = 12, heavyMax = 20;
    int explosiveAcc = 1; // v10: era 8. mira (0-100) de quem usa explosivos: RPG, lanca-granadas, tanque, jato, helicoptero de ataque
    // [Escalation] v5: a partir desses pontos, mais especiais (Juggernaut, minigun, ataque)
    // [Tank] / [Jet] v6: so no ultimo nivel, um de cada por vez
    bool tankEnabled = true, jetEnabled = true;
    int tankMinLevel = 8, tankChance = 3, tankMax = 6, tankCooldown = 60, jetMinLevel = 8, jetChance = 2, jetMax = 5, jetDuration = 90, jetCooldown = 60,
        jetHydraPct = 25;
    int escStart = 350;
    float escPerPoint = 0.2f;
};
static const int kHardCap = 30; // v10: nunca mais de 30 inimigos ativos ao mesmo tempo, somando tudo
extern Config g_cfg;
// v10: fator da escala de pontos (1.0 = escala antiga de 100 pontos, 5.0 = 500 pontos)
inline float PtsScale() { return g_cfg.maxPoints > 0 ? g_cfg.maxPoints / 100.f : 1.f; }
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
    bool director = false;    // v7: no Modo Diretor com o Retaliation liberado (sem ciclo da policia)
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
void AddPointsForce(int amount, const char *why); // ignora o bloqueio de fora do ciclo (teste e inicio do ciclo)
void CycleTick(Player pid);
void PoliceRestore(Player pid); // devolve a policia ao normal (mod desligado ou troca de modo)
void ConfigSetInt(const char *sec, const char *key, int v); // grava no .ini
extern const char *kOutfitSection[4];
void OutfitCapture(Ped p, int slot); // salva a roupa do ped como roupa do Juggernaut (slot 0 = provador)
void ConfigSetStr(const char *sec, const char *key, const char *v);
void OutfitApply(Ped p, const Config::Outfit &o);
void MenuTick(); // menu do Insert (v8)
extern const char *kModeName[5];
// v11: modo Desordem (points.cpp)
bool DisorderCalm();      // modo 4 ativo e os retaliadores nao estao contra o jogador
bool DisorderOn();        // modo 4 ativo (fora de missao)
void DisorderProvoke(bool police); // o jogador atacou a policia (true) ou os retaliadores (false)
void DisorderMenuLevel(); // nivel escolhido no menu: recomeca a espera sem baixar
int UiLang(); // 0 portugues, 1 ingles, 2 espanhol (segue o idioma do jogo)
const char *Tr(const char *pt, const char *en, const char *es);

// hud.cpp
void HudTick();
void Notify(const char *text);

