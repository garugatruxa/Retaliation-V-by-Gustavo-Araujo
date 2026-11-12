// Barra de retaliacao no estilo da barra do especial: fundo escuro da mesma cor, preenchimento
// forte, animacao suave e brilho no trecho que acabou de encher.
#include "common.h"
#include <cmath>
#include <cstdio>

static float s_shown = 0.f;      // valor exibido (0..1), anima ate o alvo
static float s_last = 0.f;       // alvo do frame anterior
static float s_gainFrom = 0.f, s_gainTo = 0.f;
static int s_gainAt = -100000;
static float s_lossFrom = 0.f;
static int s_lossAt = -100000;
static int s_lastMs = 0;

static const int kGainMs = 900, kLossMs = 700;

void Notify(const char *text) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY((char *)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char *)text);
    UI::_DRAW_NOTIFICATION(FALSE, FALSE);
}

static int Clamp255(float v) { return v < 0 ? 0 : v > 255 ? 255 : (int)v; }

// Retangulo por borda esquerda (DRAW_RECT usa o centro).
static void RectL(float x, float y, float w, float h, int r, int g, int b, int a) {
    if (w <= 0.f) return;
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y, w, h, r, g, b, a);
}

static void DrawSmallText(const char *s, float x, float y) {
    UI::SET_TEXT_FONT(4);
    UI::SET_TEXT_SCALE(0.0f, 0.28f);
    UI::SET_TEXT_COLOUR(230, 230, 230, 220);
    UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY((char *)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char *)s);
    UI::_DRAW_TEXT(x, y);
}

void HudTick() {
    State &s = g_st;
    int dt = s_lastMs ? s.now - s_lastMs : 16;
    s_lastMs = s.now;
    if (dt < 0 || dt > 500) dt = 16;

    float target = g_cfg.maxPoints > 0 ? (float)s.Pts() / g_cfg.maxPoints : 0.f;
    if (target > s_last + 0.0001f) { s_gainFrom = s_shown; s_gainTo = target; s_gainAt = s.now; }
    if (target < s_last - 0.0001f) { s_lossFrom = s_shown; s_lossAt = s.now; }
    s_last = target;
    // anima: sobe rapido, desce um pouco mais devagar
    float speed = (target > s_shown ? 2.2f : 1.2f) * dt / 1000.f;
    if (std::fabs(target - s_shown) <= speed) s_shown = target;
    else s_shown += target > s_shown ? speed : -speed;

    if (!g_cfg.hudEnabled) return;
    if (s.Pts() <= 0 && s_shown <= 0.f && s.now >= s.cooldownUntil) return;
    if (UI::IS_HUD_HIDDEN() || UI::IS_RADAR_HIDDEN() || UI::IS_PAUSE_MENU_ACTIVE() ||
        CUTSCENE::IS_CUTSCENE_PLAYING() || CAM::IS_SCREEN_FADED_OUT())
        return;

    // posicao do minimapa (formula padrao da ancora do radar)
    float safe = invoke<float>(0xBAF107B6BB2C97F0); // GET_SAFE_ZONE_SIZE
    float aspect = GRAPHICS::_GET_SCREEN_ASPECT_RATIO(FALSE);
    if (aspect < 1.0f) aspect = 16.f / 9.f;
    float mapW = 1.0f / (4.0f * aspect);
    float margin = 0.5f * std::fabs(safe - 1.0f);
    float mapLeft = margin;
    float mapBottom = 1.0f - margin;

    float w = mapW * (g_cfg.hudEnd - g_cfg.hudStart);
    float h = g_cfg.hudHeight;
    float x = mapLeft + mapW * g_cfg.hudStart + g_cfg.hudOffsetX;
    float y = mapBottom + g_cfg.hudOffsetY;

    int R = g_cfg.hudR, G = g_cfg.hudG, B = g_cfg.hudB;

    // fundo: mesma cor bem escura, como a barra do especial vazia
    RectL(x, y, w, h, Clamp255(R * 0.30f), Clamp255(G * 0.30f), Clamp255(B * 0.30f), 170);

    // trecho perdido: fica claro e some
    float lt = (float)(s.now - s_lossAt) / kLossMs;
    if (lt >= 0.f && lt < 1.f && s_lossFrom > s_shown) {
        float a = 1.f - lt;
        RectL(x + w * s_shown, y, w * (s_lossFrom - s_shown), h, Clamp255(R + (255 - R) * 0.5f),
              Clamp255(G + (255 - G) * 0.5f), Clamp255(B + (255 - B) * 0.5f), Clamp255(200 * a));
    }

    // preenchimento; pisca quando o calor maximo esta carregando
    int alpha = 235;
    if (s.maxHeatSince >= 0) alpha = Clamp255(150 + 105 * (0.5f + 0.5f * std::sin(s.now * 0.012f)));
    RectL(x, y, w * s_shown, h, R, G, B, alpha);

    // brilho do ganho: o trecho novo fica quase branco e volta para a cor, com um halo em volta
    float gt = (float)(s.now - s_gainAt) / kGainMs;
    if (gt >= 0.f && gt < 1.f) {
        float k = 1.f - gt;
        k = k * k;
        float from = s_gainFrom, to = s_shown < s_gainTo ? s_shown : s_gainTo;
        if (to > from) {
            RectL(x + w * from, y, w * (to - from), h, Clamp255(R + (255 - R) * 0.85f * k),
                  Clamp255(G + (255 - G) * 0.85f * k), Clamp255(B + (255 - B) * 0.85f * k), 255);
        }
        // halo suave na barra inteira
        RectL(x - 0.0015f, y, w + 0.003f, h * 2.6f, Clamp255(R + 40), Clamp255(G + 20), Clamp255(B + 20),
              Clamp255(70 * k));
        // ponta brilhante na frente do preenchimento
        RectL(x + w * s_shown - 0.002f, y, 0.002f, h * 1.6f, 255, 235, 235, Clamp255(230 * k));
    }

    // marcas dos niveis
    for (int start : g_cfg.levelStart) {
        float f = (float)start / g_cfg.maxPoints;
        if (f <= 0.f || f >= 1.f) continue;
        RectL(x + w * f - 0.0004f, y, 0.0008f, h, 0, 0, 0, 120);
    }

    if (g_cfg.hudShowLevel) {
        char buf[16];
        if (s.now < s.cooldownUntil) snprintf(buf, sizeof buf, "--");
        else snprintf(buf, sizeof buf, "%d", LevelOf(s.Pts()));
        DrawSmallText(buf, x + w + 0.004f, y - 0.0115f);
    }
}


