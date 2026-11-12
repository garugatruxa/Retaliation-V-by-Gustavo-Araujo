// v8: menu simples do mod na tecla Insert.
// Setas para cima/baixo escolhem a linha; esquerda/direita mudam o valor. A escolha fica salva no .ini.
#include "common.h"

const char *kModeName[4] = {"", "Junto com a polícia", "Substitui a polícia", "Ciclo (polícia > Retaliation)"};

static bool s_open = false;
static int s_row = 0; // 0 = mod ativado, 1 = modo

static bool Pressed(int vk) {
    static bool down[256];
    bool d = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool hit = d && !down[vk & 0xFF];
    down[vk & 0xFF] = d;
    return hit;
}

static void Rect(float x, float y, float w, float h, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y + h * 0.5f, w, h, r, g, b, a);
}

static void Text(const char *s, float x, float y, float scale, bool center = false) {
    UI::SET_TEXT_FONT(4);
    UI::SET_TEXT_SCALE(0.0f, scale);
    UI::SET_TEXT_COLOUR(255, 255, 255, 240);
    UI::SET_TEXT_DROPSHADOW(1, 0, 0, 0, 200);
    if (center) UI::SET_TEXT_CENTRE(TRUE);
    UI::_SET_TEXT_ENTRY((char *)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char *)s);
    UI::_DRAW_TEXT(x, y);
}

static void Draw() {
    const float x = 0.015f, y = 0.30f, w = 0.24f, head = 0.038f, rowH = 0.034f;
    int R = g_cfg.hudR, G = g_cfg.hudG, B = g_cfg.hudB;
    // cabecalho vermelho, no mesmo tom da barra
    Rect(x, y, w, head, R, G, B, 235);
    Text("RETALIATION V", x + 0.008f, y + 0.004f, 0.48f);
    // corpo escuro
    Rect(x, y + head, w, rowH * 2 + 0.008f, 10, 10, 10, 190);

    const char *labels[2] = {"Mod", "Modo"};
    char values[2][64];
    snprintf(values[0], sizeof values[0], "< %s >", g_cfg.enabled ? "Ativado" : "Desativado");
    snprintf(values[1], sizeof values[1], "< %s >", kModeName[g_cfg.mode]);
    for (int i = 0; i < 2; ++i) {
        float ry = y + head + 0.004f + rowH * i;
        if (i == s_row) {
            Rect(x, ry, w, rowH, R, G, B, 110);      // linha selecionada
            Rect(x, ry, 0.0025f, rowH, R, G, B, 255); // marcador na esquerda
        }
        Text(labels[i], x + 0.008f, ry + 0.005f, 0.34f);
        Text(values[i], x + w * 0.62f, ry + 0.005f, 0.34f, true);
    }
    Text("Setas: escolher e mudar   Insert: fechar", x + 0.004f, y + head + rowH * 2 + 0.012f, 0.25f);
}

void MenuTick() {
    if (Pressed(VK_INSERT)) s_open = !s_open;
    bool up = Pressed(VK_UP), dn = Pressed(VK_DOWN), lf = Pressed(VK_LEFT), rt = Pressed(VK_RIGHT);
    if (!s_open) return;

    // enquanto o menu esta aberto, as setas nao mexem no celular nem no jogo
    static const int kBlock[] = {27, 172, 173, 174, 175, 187, 188, 189, 190};
    for (int c : kBlock) CONTROLS::DISABLE_CONTROL_ACTION(0, c, TRUE);

    if (up || dn) s_row = 1 - s_row;
    if (lf || rt) {
        if (s_row == 0) {
            g_cfg.enabled = !g_cfg.enabled;
            ConfigSetInt("General", "Enabled", g_cfg.enabled ? 1 : 0);
            Log("Menu: mod %s.", g_cfg.enabled ? "ativado" : "desativado");
        } else {
            int m = g_cfg.mode + (rt ? 1 : -1);
            if (m < 1) m = 3;
            if (m > 3) m = 1;
            g_cfg.mode = m;
            ConfigSetInt("General", "Mode", m);
            Log("Menu: modo %d (%s).", m, kModeName[m]);
        }
    }
    Draw();
}
