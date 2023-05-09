#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
// #include <SDL_main.h>

#include "endoom.h"
#include "resources/font.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

const int CHAR_WIDTH = 9;
const int CHAR_HEIGHT = 16;

const SDL_Color COLORMAP[] = {
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
static SDL_Texture* g_font_texture = NULL;
static SDL_Texture* g_offscreen_texture = NULL;
static SDL_TimerID g_blink_timer = 0;
static bool g_blink = false;

void teardown_and_exit(int exit_code) {
  if (g_blink_timer) SDL_RemoveTimer(g_blink_timer);
  if (g_font_texture) SDL_DestroyTexture(g_font_texture);
  if (g_offscreen_texture) SDL_DestroyTexture(g_offscreen_texture);
  if (g_renderer) SDL_DestroyRenderer(g_renderer);
  if (g_window) SDL_DestroyWindow(g_window);
  SDL_Quit();
  exit(exit_code);
}

void init_check(bool test, const char* stage) {
  if (!test) {
    eprintf("Could not %s: %s. Exiting.\n", stage, SDL_GetError());
    teardown_and_exit(1);
  }
}

SDL_Texture* load_font(SDL_Renderer* renderer) {
  SDL_Surface* font_surface = SDL_LoadBMP_RW(SDL_RWFromConstMem(font_bmp_DATA, font_bmp_SIZE), 1);
  if (!font_surface) {
    return NULL;
  }

  SDL_SetColorKey(font_surface, SDL_TRUE, SDL_MapRGBA(font_surface->format, 0, 0, 0, 255));
  SDL_Texture* font_tex = SDL_CreateTextureFromSurface(renderer, font_surface);
  SDL_FreeSurface(font_surface);
  return font_tex;
}

// Helper function: index into the font texture
SDL_Rect texrect(int idx) {
  assert(idx >= 0 && idx < 256);
  SDL_Rect result = {.x = (idx % 32) * CHAR_WIDTH,
                     .y = (idx / 32) * CHAR_HEIGHT,
                     .w = CHAR_WIDTH,
                     .h = CHAR_HEIGHT};
  return result;
}

int fg_color_index(int attrs) { return attrs & 0b1111; }
int bg_color_index(int attrs) { return (attrs & 0b1110000) >> 4; }
bool should_blink(int attrs) { return (attrs & 0b10000000) >> 7; }

Uint32 blink_callback(Uint32 interval, void* data) {
  g_blink = !g_blink;

  SDL_Event user_event;
  user_event.type = SDL_USEREVENT;
  user_event.user.code = g_blink;
  SDL_PushEvent(&user_event);
  return interval;
}

int main(int argc, char* argv[]) {
  if (argc < 2) return 1;

  uint8_t* endoom = load_endoom_from_wad(argv[1]);
  if (!endoom) {
    eprintf("Could not load '%s'. Exiting.\n", argv[1]);
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    eprintf("Could not init SDL: %s\n", SDL_GetError());
    return 1;
  }

  g_window = SDL_CreateWindow("ENDOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              CHAR_WIDTH * COL_COUNT * 2, CHAR_HEIGHT * ROW_COUNT * 2,
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
  init_check(g_window, "create window");

  g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
  init_check(g_renderer, "create renderer");

  g_offscreen_texture =
      SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                        CHAR_WIDTH * COL_COUNT, CHAR_HEIGHT * ROW_COUNT);
  init_check(g_offscreen_texture, "create texture");

  g_font_texture = load_font(g_renderer);
  init_check(g_font_texture, "load font");

  g_blink_timer = SDL_AddTimer(300, blink_callback, NULL);
  if (!g_blink_timer) {
    eprintf("Could not initialize timer: %s\n", SDL_GetError());
    eprintf("Blinking will be disabled.\n");
  }

  // SDL_SetTextureScaleMode(font, SDL_ScaleModeNearest);
  SDL_SetTextureBlendMode(g_font_texture, SDL_BLENDMODE_BLEND);
  SDL_ShowWindow(g_window);

  SDL_Event event;
  while (SDL_WaitEvent(&event)) {
    if (event.type == SDL_QUIT) break;
    SDL_SetRenderTarget(g_renderer, g_offscreen_texture);

    SDL_Rect src_rect;
    SDL_Rect dest_rect = {.x = 0, .y = 0, .w = CHAR_WIDTH, .h = CHAR_HEIGHT};
    bool blink_local = g_blink;
    for (int i = 0; i < ROW_COUNT * COL_COUNT; ++i) {
      uint8_t tex_index = endoom[i * 2];
      uint8_t attrs = endoom[i * 2 + 1];

      src_rect = texrect(tex_index);
      dest_rect.x = (i % COL_COUNT) * CHAR_WIDTH;
      dest_rect.y = (i / COL_COUNT) * CHAR_HEIGHT;

      SDL_Color fg = COLORMAP[fg_color_index(attrs)];
      SDL_Color bg = COLORMAP[bg_color_index(attrs)];

      SDL_SetRenderDrawColor(g_renderer, bg.r, bg.g, bg.b, bg.a);
      SDL_SetTextureColorMod(g_font_texture, fg.r, fg.g, fg.b);
      SDL_RenderFillRect(g_renderer, &dest_rect);
      if (!blink_local || !should_blink(attrs)) {
        SDL_RenderCopy(g_renderer, g_font_texture, &src_rect, &dest_rect);
      }
    }

    SDL_SetRenderTarget(g_renderer, NULL);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_offscreen_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
  }

  teardown_and_exit(0);

  return 0;  // make the linter happy
}
