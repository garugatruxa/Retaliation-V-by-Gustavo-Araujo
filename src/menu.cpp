// v8/v9.2: menu do mod na tecla Insert.
// Setas para cima/baixo escolhem a linha; esquerda/direita mudam o valor. A escolha fica salva no .ini.
// v9.2: segue o idioma do jogo (portugues, espanhol; o resto em ingles), titulo na fonte do GTA e janela compacta.
#include "common.h"

// ---------- idioma ----------
// ids do jogo: 0 ingles, 1 frances, 2 alemao, 3 italiano, 4 espanhol, 5 portugues, 6 polones, 7 russo,
// 8 coreano, 9 chines trad., 10 japones, 11 espanhol (Mexico), 12 chines simpl.
int UiLang() {
    int id = UNK::_GET_UI_LANGUAGE_ID();
    if (id == 5) return 0;             // portugues
    if (id == 4 || id == 11) return 2; // espanhol
    return 1;                          // ingles para os demais
}
const char *Tr(const char *pt, const char *en, const char *es) {
    int l = UiLang();
    return l == 0 ? pt : l == 2 ? es : en;
}

const char *kModeName[4] = {"", "Junto com a polícia", "Substitui a polícia", "Ciclo (polícia > Retaliation)"};
static const char *ModeName(int m) {
    switch (m) {
    case 1: return Tr("Junto com a polícia", "Alongside the police", "Junto a la policía");
    case 2: return Tr("Substitui a polícia", "Replaces the police", "Reemplaza a la policía");
    default: return Tr("Ciclo: polícia > Retaliation", "Cycle: police > Retaliation", "Ciclo: policía > Retaliation");
    }
}

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

// align: 0 esquerda, 1 centro, 2 direita (ate wrapR)
static void Text(const char *s, float x, float y, float scale, int font = 4, int align = 0, float wrapR = 0.f) {
    UI::SET_TEXT_FONT(font);
    UI::SET_TEXT_SCALE(0.0f, scale);
    UI::SET_TEXT_COLOUR(255, 255, 255, 245);
    UI::SET_TEXT_DROPSHADOW(1, 0, 0, 0, 220);
    if (align == 1) UI::SET_TEXT_CENTRE(TRUE);
    if (align == 2) { UI::SET_TEXT_RIGHT_JUSTIFY(TRUE); UI::SET_TEXT_WRAP(0.f, wrapR); }
    UI::_SET_TEXT_ENTRY((char *)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char *)s);
    UI::_DRAW_TEXT(x, y);
}

static void Draw() {
    // perto do topo, compacto, letras maiores que na v8
    const float x = 0.012f, y = 0.022f, w = 0.235f, head = 0.046f, rowH = 0.036f, pad = 0.008f;
    int R = g_cfg.hudR, G = g_cfg.hudG, B = g_cfg.hudB;

    // cabecalho vermelho (tom da barra) com o titulo na fonte do GTA (Pricedown)
    Rect(x, y, w, head, R, G, B, 240);
    Text("Retaliation V", x + pad, y - 0.002f, 0.62f, 7);
    // corpo escuro
    float bodyY = y + head;
    Rect(x, bodyY, w, rowH * 2 + 0.006f, 8, 8, 8, 200);

    const char *labels[2] = {Tr("Mod", "Mod", "Mod"), Tr("Modo", "Mode", "Modo")};
    char values[2][80];
    snprintf(values[0], sizeof values[0], "< %s >",
             g_cfg.enabled ? Tr("Ativado", "On", "Activado") : Tr("Desativado", "Off", "Desactivado"));
    snprintf(values[1], sizeof values[1], "< %s >", ModeName(g_cfg.mode));
    for (int i = 0; i < 2; ++i) {
        float ry = bodyY + 0.003f + rowH * i;
        if (i == s_row) {
            Rect(x, ry, w, rowH, R, G, B, 120);      // linha selecionada
            Rect(x, ry, 0.003f, rowH, R, G, B, 255); // marcador na esquerda
        }
        Text(labels[i], x + pad + 0.002f, ry + 0.005f, 0.40f, 4);
        Text(values[i], 0.f, ry + 0.005f, 0.40f, 4, 2, x + w - pad); // valor alinhado a direita
    }
    // rodape fininho com a dica
    float fy = bodyY + rowH * 2 + 0.006f;
    Rect(x, fy, w, 0.024f, 8, 8, 8, 170);
    Rect(x, fy, w, 0.0015f, R, G, B, 200);
    Text(Tr("Setas: escolher e mudar  |  Insert: fechar", "Arrows: select and change  |  Insert: close",
            "Flechas: elegir y cambiar  |  Insert: cerrar"),
         x + pad, fy + 0.002f, 0.28f, 4);
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
