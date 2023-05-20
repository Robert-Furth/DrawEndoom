#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_main.h>
#include <Windows.h>

#include "endoom.h"
#include "rendering.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

void reverse_list(char** list, size_t size) {
  for (size_t i = 0; i < size / 2; ++i) {
    char* tmp = list[i];
    list[i] = list[size - i - 1];
    list[size - i - 1] = tmp;
  }
}

char** get_wads_by_priority(int argc, char* argv[]) {
  bool next_is_pwad = false;
  bool next_is_iwad = false;

  // Wad list will have strictly fewer entries than argv
  char** wad_list = calloc(argc, sizeof(char*));
  size_t wad_list_size = 0;
  char* iwad = NULL;

  for (int i = 1; i < argc; ++i) {
    if (next_is_iwad) {
      iwad = argv[i];
      next_is_iwad = false;
      continue;
    }

    if (next_is_pwad) {
      wad_list[wad_list_size++] = argv[i];
      next_is_pwad = false;
      continue;
    }

    if (strcmp(argv[i], "-file") == 0) {
      next_is_pwad = true;
    } else if (strcmp(argv[i], "-iwad") == 0) {
      next_is_iwad = true;
    }
  }

  reverse_list(wad_list, wad_list_size);
  if (iwad) wad_list[wad_list_size++] = iwad;
  return wad_list;
}

void start_source_port(char* argv0) {
  char* cmdline = GetCommandLine();
  char first_char = cmdline[0];

  cmdline += strlen(argv0) + 1;         // length of program name, plus following space
  if (first_char == '"') cmdline += 2;  // include double quotes

  SHELLEXECUTEINFO exec_info = {0};
  exec_info.cbSize = sizeof(SHELLEXECUTEINFO);
  exec_info.lpFile = "dsda-doom.exe";
  exec_info.lpParameters = cmdline;
  exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
  exec_info.nShow = SW_NORMAL;

  ShellExecuteEx(&exec_info);
  WaitForSingleObject(exec_info.hProcess, INFINITE);
}

int main(int argc, char** argv) {
  if (argc < 2) return 1;

  uint8_t* endoom;

  char** wad_list = get_wads_by_priority(argc, argv);
  for (size_t i = 0; wad_list[i] != NULL; ++i) {
    endoom = load_endoom_from_wad(wad_list[i]);
    if (endoom) break;
  }
  free(wad_list);

  start_source_port(argv[0]);

  if (endoom && setup_renderer("ENDOOM", 720, 400)) {
    show_endoom(endoom);
    teardown_renderer();
  }
  free(endoom);
  return 0;
}
