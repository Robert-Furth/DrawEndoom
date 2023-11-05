#include "rendering.h"

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

#include "endoom.h"
#include "resources/font.h"

static const int CHAR_WIDTH = 9;
static const int CHAR_HEIGHT = 16;
static const int TEX_FONT_WIDTH = 32;  // Width *in chars* of the font texture

static const SDL_Color COLORMAP[] = {
    {0, 0, 0, 255},        // black
    {0, 0, 170, 255},      // blue
    {0, 170, 0, 255},      // green
    {0, 170, 170, 255},    // cyan
    {170, 0, 0, 255},      // red
    {170, 0, 170, 255},    // magenta
    {170, 85, 0, 255},     // brown
    {170, 170, 170, 255},  // light gray

    {85, 85, 85, 255},     // dark gray
    {85, 85, 255, 255},    // light blue
    {85, 255, 85, 255},    // light green
    {85, 255, 255, 255},   // light cyan
    {255, 85, 85, 255},    // light red
    {255, 85, 255, 255},   // light magenta
    {255, 255, 85, 255},   // yellow
    {255, 255, 255, 255},  // white
};

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture* g_tex_font = NULL;
static SDL_Texture* g_tex_canvas = NULL;

void teardown_renderer(void) {
  if (g_tex_canvas) SDL_DestroyTexture(g_tex_canvas);
  if (g_tex_font) SDL_DestroyTexture(g_tex_font);
  if (g_renderer) SDL_DestroyRenderer(g_renderer);
  if (g_window) SDL_DestroyWindow(g_window);
  SDL_Quit();
}

static void draw_frame(uint8_t* endoom, bool blink_flag) {
  // Render to the canvas
  SDL_SetRenderTarget(g_renderer, g_tex_canvas);

  for (int i = 0; i < ROW_COUNT * COL_COUNT; ++i) {
    int texture_index = endoom[i * 2];
    uint8_t attr_byte = endoom[i * 2 + 1];

    uint8_t fg_color_index = attr_byte & 0b1111;
    uint8_t bg_color_index = (attr_byte & 0b1110000) >> 4;
    bool char_blinks = attr_byte & 0b10000000;

    SDL_Rect source = {
        .x = (texture_index % TEX_FONT_WIDTH) * CHAR_WIDTH,
        .y = (texture_index / TEX_FONT_WIDTH) * CHAR_HEIGHT,
        .w = CHAR_WIDTH,
        .h = CHAR_HEIGHT,
    };
    SDL_Rect dest = {
        .x = (i % COL_COUNT) * CHAR_WIDTH,
        .y = (i / COL_COUNT) * CHAR_HEIGHT,
        .w = CHAR_WIDTH,
        .h = CHAR_HEIGHT,
    };

    SDL_Color fg = COLORMAP[fg_color_index];
    SDL_Color bg = COLORMAP[bg_color_index];

    // Draw the background color
    SDL_SetRenderDrawColor(g_renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(g_renderer, &dest);

    // Draw the foreground if the char isn't blinking off
    if (!char_blinks || !blink_flag) {
      SDL_SetTextureColorMod(g_tex_font, fg.r, fg.g, fg.b);
      SDL_RenderCopy(g_renderer, g_tex_font, &source, &dest);
    }
  }

  // Copy the canvas to the screen (possibly squashing/stretching it)
  SDL_SetRenderTarget(g_renderer, NULL);
  SDL_RenderCopy(g_renderer, g_tex_canvas, NULL, NULL);
  SDL_RenderPresent(g_renderer);
}

typedef struct {
  uint8_t* endoom;
  bool blink_on;
} CallbackData;

static Uint32 blink_chars(Uint32 interval, void* data) {
  CallbackData* cbdata = (CallbackData*)data;
  draw_frame(cbdata->endoom, cbdata->blink_on);
  cbdata->blink_on = !cbdata->blink_on;
  return interval;
}

void show_endoom(uint8_t* endoom) {
  draw_frame(endoom, false);
  SDL_RaiseWindow(g_window);

  CallbackData cbdata = {.endoom = endoom, .blink_on = true};
  SDL_TimerID timer = SDL_AddTimer(400, blink_chars, (void*)&cbdata);

  SDL_Event evt;
  while (SDL_WaitEvent(&evt)) {
    if (evt.type == SDL_QUIT || evt.type == SDL_KEYUP || evt.type == SDL_MOUSEBUTTONUP) break;
  }

  SDL_RemoveTimer(timer);
}

static SDL_Texture* load_font(SDL_Renderer* renderer, const void* data, int size) {
  SDL_Surface* surf = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, size), 1);
  if (!surf) return NULL;

  // Set black to transparent
  SDL_SetColorKey(surf, SDL_TRUE, SDL_MapRGB(surf->format, 0, 0, 0));

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);
  return tex;
}

bool setup_renderer(const char* window_title, int window_w, int window_h) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) return false;
  SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");

  g_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              window_w, window_h, SDL_WINDOW_ALLOW_HIGHDPI);
  if (!g_window) goto error;  // don't tell dijkstra

  g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_TARGETTEXTURE);
  if (!g_renderer) goto error;

  g_tex_canvas = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                   COL_COUNT * CHAR_WIDTH, ROW_COUNT * CHAR_HEIGHT);
  if (!g_tex_canvas) goto error;

  g_tex_font = load_font(g_renderer, font_bmp_DATA, font_bmp_SIZE);
  if (!g_tex_font) goto error;
  SDL_SetTextureBlendMode(g_tex_font, SDL_BLENDMODE_BLEND);

  return true;

error:
  teardown_renderer();
  return false;
}
