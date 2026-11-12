// Retaliation V - ponto de entrada (ScriptHookV).
#include "spawn.h"

State g_st;
const char *kCharName[CH_COUNT] = {"Michael", "Franklin", "Trevor", "Outro"};


static Character CharOf(Ped p) {
    static const Hash m = GAMEPLAY::GET_HASH_KEY((char *)"player_zero");
    static const Hash f = GAMEPLAY::GET_HASH_KEY((char *)"player_one");
    static const Hash t = GAMEPLAY::GET_HASH_KEY((char *)"player_two");
    Hash h = ENTITY::GET_ENTITY_MODEL(p);
    return h == m ? CH_MICHAEL : h == f ? CH_FRANKLIN : h == t ? CH_TREVOR : CH_OTHER;
}

static bool KeyPressed(int vk) {
    static bool down[256];
    bool d = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool hit = d && !down[vk & 0xFF];
    down[vk & 0xFF] = d;
    return hit;
}

static FILETIME IniTime() {
    WIN32_FILE_ATTRIBUTE_DATA a{};
    GetFileAttributesExA((g_dir + "Retaliation.ini").c_str(), GetFileExInfoStandard, &a);
    return a.ftLastWriteTime;
}

// v7: sem Ctrl (que o jogo usa). Segurar PageDown e apertar PageUp sobe um nivel;
// segurar PageUp e apertar PageDown desce um nivel.
static void DebugKeys() {
    bool up = KeyPressed(VK_PRIOR), dn = KeyPressed(VK_NEXT), end = KeyPressed(VK_END);
    bool holdUp = (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0, holdDn = (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0;
    if (!g_cfg.debugKeys) return;
    const auto &ls = g_cfg.levelStart;
    int pts = g_st.Pts(), lv = LevelOf(pts);
    if (end && holdDn) { // v8: copia a roupa atual como roupa do Juggernaut
        OutfitCapture(PLAYER::PLAYER_PED_ID());
        Notify(Tr("~r~Retaliation V~s~: roupa do Juggernaut copiada.", "~r~Retaliation V~s~: Juggernaut outfit saved.",
                   "~r~Retaliation V~s~: ropa del Juggernaut guardada."));
        return;
    }
    if (up && holdDn) { // sobe um nivel
        int to = lv < (int)ls.size() ? ls[lv] : g_cfg.maxPoints;
        if (to > pts) AddPointsForce(to - pts, "(teste) segurar PageDown + PageUp: sobe nivel");
    } else if (dn && holdUp) { // desce um nivel
        int to = lv >= 2 ? ls[lv - 2] : 0;
        if (pts > to) AddPoints(to - pts, "(teste) segurar PageUp + PageDown: desce nivel");
    }
}

// Motivo para pausar as emboscadas (vazio = ativo).
// v7: o Modo Diretor (Rockstar Editor) roda como "missao"; aqui ele e reconhecido separadamente.
static bool InDirectorMode() {
    static const Hash dm = GAMEPLAY::GET_HASH_KEY((char *)"director_mode");
    return SCRIPT::_GET_NUMBER_OF_INSTANCES_OF_STREAMED_SCRIPT(dm) > 0;
}

static const char *PauseReason(Ped player) {
    if (CUTSCENE::IS_CUTSCENE_PLAYING()) return "cutscene";
    if (InDirectorMode()) {
        if (!g_cfg.directorMode) return "modo diretor"; // limpa tudo e pausa
    } else if (!g_cfg.activeInMissions && GAMEPLAY::GET_MISSION_FLAG()) {
        return "missao";
    }
    if (INTERIOR::GET_INTERIOR_FROM_ENTITY(player) != 0) return "interior";
    return "";
}

static void ScriptMain() {
    Log("Script iniciado (modo historia).");
    ConfigLoad();
    FILETIME iniTime = IniTime();

    g_st = State{};
    g_st.now = (int)GAMEPLAY::GET_GAME_TIMER();
    PointsReset();
    SpawnerInit();
    int lastNow = g_st.now, lastIniCheck = g_st.now;

    while (true) {
        g_st.now = (int)GAMEPLAY::GET_GAME_TIMER();

        // jogo carregado (o relogio voltou): estado zerado, como no SA
        if (g_st.now + 1000 < lastNow) {
            Log("Jogo carregado: estado reiniciado.");
            DespawnAll(true, "jogo carregado");
            PointsReset();
            g_st.lastCrime = g_st.lastDecay = g_st.now;
        }
        lastNow = g_st.now;

        // .ini recarrega sozinho quando o arquivo muda
        if (g_st.now - lastIniCheck > 2000) {
            lastIniCheck = g_st.now;
            FILETIME t = IniTime();
            if (CompareFileTime(&t, &iniTime) != 0) {
                iniTime = t;
                Log("Retaliation.ini alterado: recarregando.");
                ConfigLoad();
            }
        }

        Ped player = PLAYER::PLAYER_PED_ID();
        Player pid = PLAYER::PLAYER_ID();
        bool switching = STREAMING::IS_PLAYER_SWITCH_IN_PROGRESS();
        if (switching && HuntersAlive()) DespawnAll(true, "troca de personagem");

        // v8: menu do Insert; ligar/desligar ou trocar de modo (pelo menu ou pelo .ini) limpa tudo
        MenuTick();
        static int lastEnabled = -1, lastMode = -1;
        if (lastEnabled != (int)g_cfg.enabled || lastMode != g_cfg.mode) {
            if (lastEnabled != -1) {
                DespawnAll(true, g_cfg.enabled ? "troca de modo" : "mod desativado");
                for (int &p : g_st.points) p = 0;
                g_st.hotSince = -1;
                g_st.lastCrime = g_st.lastDecay = g_st.now;
                PoliceRestore(pid);
                Log("Mod %s, modo %d (%s).", g_cfg.enabled ? "ativado" : "desativado", g_cfg.mode,
                    kModeName[g_cfg.mode]);
            }
            lastEnabled = g_cfg.enabled;
            lastMode = g_cfg.mode;
        }
        if (!g_cfg.enabled || !ENTITY::DOES_ENTITY_EXIST(player) || switching) {
            WAIT(0);
            continue;
        }

        Character ch = CharOf(player);
        if (ch != g_st.ch) {
            Log("Personagem: %s -> %s (pontos dele: %d)", kCharName[g_st.ch], kCharName[ch], g_st.points[ch]);
            g_st.ch = ch;
            g_st.maxHeatSince = -1;
            g_st.lastCrime = g_st.lastDecay = g_st.now;
            DespawnAll(true, "troca de personagem");
        }

        // v7: entrar ou sair do Modo Diretor limpa os retaliadores e zera a retaliacao
        bool director = g_cfg.directorMode && InDirectorMode();
        if (director != g_st.director) {
            DespawnAll(true, director ? "entrou no Modo Diretor" : "saiu do Modo Diretor");
            for (int &p : g_st.points) p = 0;
            g_st.hotSince = -1;
            g_st.lastCrime = g_st.lastDecay = g_st.now;
            g_st.director = director;
            Log(director ? "Modo Diretor: Retaliation zerado e liberado (sobe por crimes ou teclas de teste)."
                         : "Fim do Modo Diretor: Retaliation zerado, ciclo da policia normal.");
        }

        const char *why = PauseReason(player);
        bool paused = *why != 0;
        if (paused != g_st.paused) {
            Log(paused ? "Emboscadas pausadas (%s)." : "Emboscadas retomadas.", why);
            g_st.paused = paused;
            g_st.pauseReason = why;
        }

        PointsTick(player, pid);
        SpawnerTick(player);
        DebugKeys();
        HudTick();
        WAIT(0);
    }
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        char path[MAX_PATH];
        GetModuleFileNameA(mod, path, MAX_PATH);
        g_dir = path;
        g_dir = g_dir.substr(0, g_dir.find_last_of("\\/") + 1);
        LogOpen(g_dir + "Retaliation.log");
        Log("Retaliation V %s carregado. Pasta: %s", RETALIATION_VERSION, g_dir.c_str());
        if (!shvInit(g_dir.c_str())) {
            Log("ERRO: ScriptHookV.dll nao encontrado ou incompativel (codigo %lu). O mod nao vai rodar.",
                GetLastError());
            return FALSE;
        }
        Log("ScriptHookV encontrado. Aguardando o modo historia...");
        scriptRegister(mod, ScriptMain);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (shv::scriptUnregister) scriptUnregister(mod);
    }
    return TRUE;
}

