// Substitui o main.h do SDK do ScriptHookV para compilar com llvm-mingw.
// O ScriptHookV.lib exporta nomes no formato do MSVC, que o mingw nao gera.
// Por isso as funcoes sao buscadas no ScriptHookV.dll com GetProcAddress (shvInit).
// O original esta em main_original.h.
#pragma once

#include <windows.h>

enum eGameVersion : int { VER_UNK = -1 };

typedef void (*KeyboardHandler)(DWORD, WORD, BYTE, BOOL, BOOL, BOOL, BOOL);
typedef void (*PresentCallback)(void *);

namespace shv {
    inline void (*scriptWait)(DWORD) = nullptr;
    inline void (*scriptRegister)(HMODULE, void (*)()) = nullptr;
    inline void (*scriptUnregister)(HMODULE) = nullptr;
    inline void (*nativeInit)(UINT64) = nullptr;
    inline void (*nativePush64)(UINT64) = nullptr;
    inline PUINT64 (*nativeCall)() = nullptr;
    inline UINT64 *(*getGlobalPtr)(int) = nullptr;
    inline int (*worldGetAllVehicles)(int *, int) = nullptr;
    inline int (*worldGetAllPeds)(int *, int) = nullptr;
    inline int (*worldGetAllObjects)(int *, int) = nullptr;
    inline BYTE *(*getScriptHandleBaseAddress)(int) = nullptr;
    inline int (*createTexture)(const char *) = nullptr;
    inline void (*drawTexture)(int, int, int, int, float, float, float, float, float, float, float, float,
                               float, float, float, float) = nullptr;
    inline void (*keyboardHandlerRegister)(KeyboardHandler) = nullptr;
    inline void (*keyboardHandlerUnregister)(KeyboardHandler) = nullptr;
}

template <typename F> static inline bool shvGet(HMODULE m, F &fn, const char *name) {
    fn = reinterpret_cast<F>(GetProcAddress(m, name));
    return fn != nullptr;
}

// Retorna false se o ScriptHookV.dll nao estiver carregado ou faltar alguma funcao essencial.
// dir = pasta do .asi; o ScriptHookV.dll e carregado de la se ainda nao estiver na memoria
// (um .asi linkado com o .lib do SDK faria isso automaticamente pela tabela de importacao).
static inline bool shvInit(const char *dir) {
    HMODULE m = GetModuleHandleA("ScriptHookV.dll");
    if (!m) {
        char path[MAX_PATH];
        lstrcpynA(path, dir, MAX_PATH - 20);
        lstrcatA(path, "ScriptHookV.dll");
        m = LoadLibraryA(path);
    }
    if (!m) return false;
    bool ok = true;
    ok &= shvGet(m, shv::scriptWait, "?scriptWait@@YAXK@Z");
    ok &= shvGet(m, shv::scriptRegister, "?scriptRegister@@YAXPEAUHINSTANCE__@@P6AXXZ@Z");
    ok &= shvGet(m, shv::scriptUnregister, "?scriptUnregister@@YAXPEAUHINSTANCE__@@@Z");
    ok &= shvGet(m, shv::nativeInit, "?nativeInit@@YAX_K@Z");
    ok &= shvGet(m, shv::nativePush64, "?nativePush64@@YAX_K@Z");
    ok &= shvGet(m, shv::nativeCall, "?nativeCall@@YAPEA_KXZ");
    ok &= shvGet(m, shv::worldGetAllVehicles, "?worldGetAllVehicles@@YAHPEAHH@Z");
    ok &= shvGet(m, shv::worldGetAllPeds, "?worldGetAllPeds@@YAHPEAHH@Z");
    shvGet(m, shv::getGlobalPtr, "?getGlobalPtr@@YAPEA_KH@Z");
    shvGet(m, shv::worldGetAllObjects, "?worldGetAllObjects@@YAHPEAHH@Z");
    shvGet(m, shv::getScriptHandleBaseAddress, "?getScriptHandleBaseAddress@@YAPEAEH@Z");
    shvGet(m, shv::createTexture, "?createTexture@@YAHPEBD@Z");
    shvGet(m, shv::drawTexture, "?drawTexture@@YAXHHHHMMMMMMMMMMMM@Z");
    shvGet(m, shv::keyboardHandlerRegister, "?keyboardHandlerRegister@@YAXP6AXKGEHHHH@Z@Z");
    shvGet(m, shv::keyboardHandlerUnregister, "?keyboardHandlerUnregister@@YAXP6AXKGEHHHH@Z@Z");
    return ok;
}

static inline void scriptWait(DWORD t) { shv::scriptWait(t); }
static inline void scriptRegister(HMODULE m, void (*f)()) { shv::scriptRegister(m, f); }
static inline void scriptUnregister(HMODULE m) { shv::scriptUnregister(m); }
static inline void nativeInit(UINT64 h) { shv::nativeInit(h); }
static inline void nativePush64(UINT64 v) { shv::nativePush64(v); }
static inline PUINT64 nativeCall() { return shv::nativeCall(); }
static inline int worldGetAllVehicles(int *a, int n) { return shv::worldGetAllVehicles(a, n); }
static inline int worldGetAllPeds(int *a, int n) { return shv::worldGetAllPeds(a, n); }

static void WAIT(DWORD time) { scriptWait(time); }
static void TERMINATE() { WAIT(MAXDWORD); }
