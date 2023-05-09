#include "endoom.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

uint8_t* load_from_fp(FILE* fp) {
  uint8_t* buf = calloc(ROW_COUNT * COL_COUNT * 2, sizeof(uint8_t));
  fread(buf, sizeof(uint8_t), ROW_COUNT * COL_COUNT * 2, fp);

  if (feof(fp)) {
    eprintf("File too small to be ENDOOM.\n");
    free(buf);
    return NULL;
  } else if (ferror(fp)) {
    perror("Error reading file");
    free(buf);
    return NULL;
  }

  return buf;
}

uint8_t* load_endoom_from_ansifile(const char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    eprintf("Could not open file '%s': %s.\n", filename, strerror(errno));
    return NULL;
  }

  uint8_t* endoom = load_from_fp(fp);
  fclose(fp);
  return endoom;
}

uint32_t ptr_to_u32(uint8_t* ptr) {
  return ptr[0] | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

uint8_t* load_endoom_from_wad(const char* wad_path) {
  uint8_t* endoom = NULL;
  FILE* fp = fopen(wad_path, "rb");

  if (!fp) {
    eprintf("Could not open file '%s': %s.\n", wad_path, strerror(errno));
    return NULL;
  }

  uint8_t buf[16];
  fread(buf, sizeof(uint8_t), 12, fp);
  if (ferror(fp)) {
    perror("Error reading WAD file");
    goto cleanup;
  }

  if (feof(fp) || (strncmp(buf, "PWAD", 4) && strncmp(buf, "IWAD", 4))) {
    eprintf("%s is not a WAD file.\n", wad_path);
    goto cleanup;
  }

  uint32_t toc_offset = ptr_to_u32(buf + 8);
  if (fseek(fp, toc_offset, SEEK_SET)) {
    perror("Error reading WAD file");
    goto cleanup;
  }

  for (;;) {
    fread(buf, sizeof(uint8_t), 16, fp);
    if (feof(fp)) {
      eprintf("WAD does not contain ENDOOM.\n");
      break;
    } else if (ferror(fp)) {
      perror("Error reading WAD file");
      break;
    }

    if (strncmp(buf + 8, "ENDOOM", 8) == 0) {
      uint32_t offset = ptr_to_u32(buf);

      if (fseek(fp, offset, SEEK_SET)) {
        perror("Error reading WAD file");
      } else {
        endoom = load_from_fp(fp);
      }

      break;
    }
  }

cleanup:
  fclose(fp);
  return endoom;
}
