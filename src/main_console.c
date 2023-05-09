#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#endif

const char* CHARMAP[] = {
    " ", "☺", "☻", "♥", "♦", "♣", "♠", "•", "◘", "○", "◙", "♂", "♀", "♪", "♫", "☼",  "►",  "◄", "↕",
    "‼", "¶", "§", "▬", "↨", "↑", "↓", "→", "←", "∟", "↔", "▲", "▼", " ", "!", "\"", "#",  "$", "%",
    "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0", "1", "2", "3", "4", "5",  "6",  "7", "8",
    "9", ":", ";", "<", "=", ">", "?", "@", "A", "B", "C", "D", "E", "F", "G", "H",  "I",  "J", "K",
    "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[",  "\\", "]", "^",
    "_", "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",  "o",  "p", "q",
    "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "⌂", "Ç", "ü",  "é",  "â", "ä",
    "à", "å", "ç", "ê", "ë", "è", "ï", "î", "ì", "Ä", "Å", "É", "æ", "Æ", "ô", "ö",  "ò",  "û", "ù",
    "ÿ", "Ö", "Ü", "¢", "£", "¥", "₧", "ƒ", "á", "í", "ó", "ú", "ñ", "Ñ", "ª", "º",  "¿",  "⌐", "¬",
    "½", "¼", "¡", "«", "»", "░", "▒", "▓", "│", "┤", "╡", "╢", "╖", "╕", "╣", "║",  "╗",  "╝", "╜",
    "╛", "┐", "└", "┴", "┬", "├", "─", "┼", "╞", "╟", "╚", "╔", "╩", "╦", "╠", "═",  "╬",  "╧", "╨",
    "╤", "╥", "╙", "╘", "╒", "╓", "╫", "╪", "┘", "┌", "█", "▄", "▌", "▐", "▀", "α",  "ß",  "Γ", "π",
    "Σ", "σ", "µ", "τ", "Φ", "Θ", "Ω", "δ", "∞", "φ", "ε", "∩", "≡", "±", "≥", "≤",  "⌠",  "⌡", "÷",
    "≈", "°", "∙", "·", "√", "ⁿ", "²", "■", " ",
};

// Black, blue, green, cyan, red, magenta, brown, gray
const int FG_COLORMAP[] = {30, 34, 32, 36, 31, 35, 33, 37, 90, 94, 92, 96, 91, 95, 93, 97};
const char BG_COLORMAP[] = {40, 44, 42, 46, 41, 45, 43, 47};

void print_ansi_char(uint8_t ch, uint8_t attrs) {
  int fg = attrs & 0b1111;
  int bg = (attrs & 0b1110000) >> 4;
  bool blink = attrs & 0b1000000;

  // \e[FG;BGm
  char escape_string[20] = "";
  snprintf(escape_string, 20, "\033[%d;%dm\033[%dm", FG_COLORMAP[fg], BG_COLORMAP[bg],
           blink ? 6 : 25);
  printf("%s%s", escape_string, CHARMAP[ch]);
}

void ansi_reset() { printf("\033[0m"); }

int main(int argc, char** argv) {
#ifdef _WIN32
  setlocale(LC_ALL, ".UTF8");
  SetConsoleOutputCP(65001);
#else
  setlocale(LC_ALL, "");
#endif

  FILE* infile = argc >= 2 ? fopen(argv[1], "r") : fopen("eviternity.bin", "r");
  if (!infile) {
    perror("Could not open file");
    return 1;
  }

  for (int i = 0; i < 256; ++i) {
    printf("%s", CHARMAP[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }

  int colnum = 0;
  int ch, attrs;

  for (;;) {
    if ((ch = fgetc(infile)) == EOF) break;
    if ((attrs = fgetc(infile)) == EOF) break;

    if (ferror(infile)) {
      ansi_reset();
      perror("Error reading file.");
      fclose(infile);
      return 1;
    }

    print_ansi_char((uint8_t)ch, (uint8_t)attrs);
    if (++colnum == 80) {
      ansi_reset();
      printf("\n");
      colnum = 0;
    }
  }

  ansi_reset();
  fclose(infile);
  return 0;
}
