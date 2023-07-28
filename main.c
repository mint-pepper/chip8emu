#include <SDL.h>
// #include <curses.h>
#include <curses.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MEMORY_SIZE 4096
#define START_POINT 0x200
#define DISPLAY_X 0
#define DISPLAY_Y 0
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define VALUE_X DISPLAY_WIDTH + 1
#define VALUE_Y 0

typedef struct {
  uint8_t V[16];
  uint16_t I;
  uint8_t DT;
  uint8_t ST;
  uint16_t PC;
  uint8_t SP;
  uint16_t stack[16];
  int pixel[32][64];
  uint8_t *memory;
  uint16_t old_PC;
  char inst[12];
  char next_inst[12];
  int key;
} Emulator;

const int PIXEL_WIDTH = 10;
const int SCREEN_WIDTH = DISPLAY_WIDTH * PIXEL_WIDTH;
const int SCREEN_HEIGHT = DISPLAY_HEIGHT * PIXEL_WIDTH;
// const char CHIP8_KEYS[] = {
//     SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_q, SDLK_w, SDLK_e, SDLK_r,
//     SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_z, SDLK_x, SDLK_c, SDLK_v,
// };
const char CHIP8_KEYS[] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_V, SDL_SCANCODE_F,
};
const int DELAY_TIME = 3;
const int DT_DEC_TIME = 5;

SDL_Window *gWindow = NULL;

SDL_Surface *gScreenSurface = NULL;

SDL_Surface *gHelloWorld = NULL;
// void function(int i) {
//   clear();
//   mvprintw(12, 30, "%d", i);
//   refresh();
// }

bool screenInit() {
  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    success = false;
  } else {
    gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                               SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
      success = false;
    } else {
      // Get window surface
      gScreenSurface = SDL_GetWindowSurface(gWindow);
    }
  }
  return success;
}

void screenClose() {
  SDL_FreeSurface(gHelloWorld);
  gHelloWorld = NULL;

  SDL_DestroyWindow(gWindow);
  gWindow = NULL;

  SDL_Quit();
}

void display_screen(Emulator *emu) {
  //  bkgd(COLOR_PAIR(1));
  attrset(COLOR_PAIR(2));
  SDL_FillRect(gScreenSurface, NULL,
               SDL_MapRGB(gScreenSurface->format, 145, 100, 30));
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    for (int j = 0; j < DISPLAY_WIDTH; j++) {
      if (emu->pixel[i][j]) {
        //       mvprintw(DISPLAY_Y + i, DISPLAY_X + j, "%c", ' ');
        SDL_Rect rect;
        rect.x = j * PIXEL_WIDTH;
        rect.y = i * PIXEL_WIDTH;
        rect.w = PIXEL_WIDTH;
        rect.h = PIXEL_WIDTH;
        SDL_FillRect(gScreenSurface, &rect,
                     SDL_MapRGB(gScreenSurface->format, 245, 200, 70));
      }
    }
  }
  SDL_UpdateWindowSurface(gWindow);
  //  bkgd(COLOR_PAIR(2));
  attrset(COLOR_PAIR(1));
}

void get_inst(Emulator *emu, uint16_t pc, char *inst) {
  int o1 = (emu->memory[pc] >> 4) & 0xF;
  int o2 = emu->memory[pc] & 0xF;
  int o3 = (emu->memory[pc + 1] >> 4) & 0xF;
  int o4 = emu->memory[pc + 1] & 0xF;

  switch (o1) {
  case 0:
    if (o2 == 0 && o3 == 0xE && o4 == 0) {
      sprintf(inst, "CLS");
    } else if (o2 == 0 && o3 == 0xE && o4 == 0xE) {
      sprintf(inst, "RET");
    } else {
      sprintf(inst, "SYS %03X", (o2 << 8) | (o3 << 4) | o4);
    }
    break;
  case 1:
    sprintf(inst, "JP %03X", (o2 << 8) | (o3 << 4) | o4);
    break;

  case 2:
    sprintf(inst, "CALL %03X", (o2 << 8) | (o3 << 4) | o4);
    break;

  case 3:
    sprintf(inst, "SE V%X %02X", o2, (o3 << 4) | o4);
    break;

  case 4:
    sprintf(inst, "SNE V%X %02X", o2, (o3 << 4) | o4);
    break;

  case 5:
    sprintf(inst, "SE V%X V%X", o2, o3);
    break;

  case 6:
    sprintf(inst, "LD V%X %02X", o2, (o3 << 4) | o4);
    break;

  case 7:
    sprintf(inst, "ADD V%X %02X", o2, (o3 << 4) | o4);
    break;

  case 8:
    switch (o4) {
    case 0:
      sprintf(inst, "LD V%X V%X", o2, o3);
      break;
    case 1:
      sprintf(inst, "OR V%X V%X", o2, o3);
      break;
    case 2:
      sprintf(inst, "AND V%X V%X", o2, o3);
      break;
    case 3:
      sprintf(inst, "XOR V%X V%X", o2, o3);
      break;
    case 4:
      sprintf(inst, "ADD V%X V%X", o2, o3);
      break;
    case 5:
      sprintf(inst, "SUB V%X V%X", o2, o3);
      break;
    case 6:
      sprintf(inst, "SHR V%X V%X", o2, o3);
      break;
    case 7:
      sprintf(inst, "SUBN V%X V%X", o2, o3);
      break;
    case 0xE:
      sprintf(inst, "SHL V%X V%X", o2, o3);
      break;
    default:
      sprintf(inst, "nothing");
      break;
    }
    break;
  case 9:
    sprintf(inst, "SNE V%X V%X", o2, o3);
    break;
  case 0xA:
    sprintf(inst, "LD I %3X", (o2 << 8) | (o3 << 4) | o4);
    break;
  case 0xB:
    sprintf(inst, "JP V0 %03X", (o2 << 8) | (o3 << 4) | o4);
    break;
  case 0xC:
    sprintf(inst, "RND V%X %03X", o2, (o3 << 4) | o4);
    break;
  case 0xD:
    sprintf(inst, "SNE V%X V%X", o2, o3);
    break;
  case 0xE:
    if (o3 == 9 && o4 == 0xE) {
      sprintf(inst, "SKP V%X", o2);
    } else if (o3 == 0xA && o4 == 1) {
      sprintf(inst, "SKNP V%X", o2);
    } else {
      sprintf(inst, "nothing");
    }
    break;
  case 0xF:
    if (o3 == 0 && o4 == 7) {
      sprintf(inst, "LD V%X DT", o2);
    } else if (o3 == 0 && o4 == 0xA) {
      sprintf(inst, "LD V%X K", o2);
    } else if (o3 == 1 && o4 == 5) {
      sprintf(inst, "LD DT V%X", o2);
    } else if (o3 == 1 && o4 == 8) {
      sprintf(inst, "LD ST V%X", o2);
    } else if (o3 == 1 && o4 == 0xE) {
      sprintf(inst, "ADD I V%X", o2);
    } else if (o3 == 2 && o4 == 9) {
      sprintf(inst, "LD F V%X", o2);
    } else if (o3 == 3 && o4 == 3) {
      sprintf(inst, "LD B V%X", o2);
    } else if (o3 == 5 && o4 == 5) {
      sprintf(inst, "LD I V%X", o2);
    } else if (o3 == 6 && o4 == 5) {
      sprintf(inst, "LD V%X I", o2);
    } else {
      sprintf(inst, "nothing");
    }
    break;
  }
}

void sys_addr(Emulator *emu, int addr) {
  sprintf(emu->inst, "SYS %03X", addr);
  emu->PC = addr;
}

void cls(Emulator *emu) {
  sprintf(emu->inst, "CLS");
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    for (int j = 0; j < DISPLAY_WIDTH; j++) {
      emu->pixel[i][j] = 0;
    }
  }
  emu->PC += 2;

  display_screen(emu);
}

void ret(Emulator *emu) {
  sprintf(emu->inst, "RET");
  emu->SP--;
  emu->PC = emu->stack[emu->SP] + 2;
}

void jp_addr(Emulator *emu, int addr) {
  sprintf(emu->inst, "JP %3X", addr);
  emu->PC = addr;
}

void call_addr(Emulator *emu, int addr) {
  sprintf(emu->inst, "CALL %3X", addr);
  emu->stack[emu->SP] = emu->PC;
  emu->SP++;
  emu->PC = addr;
}

void se_Vx_byte(Emulator *emu, int x, int byte) {
  sprintf(emu->inst, "SE V%X %2X", x, byte);
  if (emu->V[x] == byte) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void sne_Vx_byte(Emulator *emu, int x, int byte) {
  sprintf(emu->inst, "SNE V%X %02X", x, byte);
  if (emu->V[x] != byte) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void se_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "SE V%X V%X", x, y);
  if (emu->V[x] == emu->V[y]) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void ld_Vx_byte(Emulator *emu, int x, int byte) {
  sprintf(emu->inst, "LD V%X %02X", x, byte);
  emu->V[x] = byte;
  emu->PC += 2;
}

void add_Vx_byte(Emulator *emu, int x, int byte) {
  sprintf(emu->inst, "ADD V%X %02X", x, byte);
  emu->V[x] += byte;
  emu->PC += 2;
}

void ld_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "LD V%X V%X", x, y);
  emu->V[x] = emu->V[y];
  emu->PC += 2;
}

void or_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "LD V%X V%X", x, y);
  emu->V[x] |= emu->V[y];
  emu->PC += 2;
}

void and_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "AND V%X V%X", x, y);
  emu->V[x] &= emu->V[y];
  emu->PC += 2;
}

void xor_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "XOR V%X V%X", x, y);
  emu->V[x] ^= emu->V[y];
  emu->PC += 2;
}

void add_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "ADD V%X V%X", x, y);
  int vx = emu->V[x] + emu->V[y];
  emu->V[x] = vx & 0xFF;
  if (vx > 0xFF) {
    emu->V[0xF] = 1;
  } else {
    emu->V[0xF] = 0;
  }
  emu->PC += 2;
}

void sub_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "SUB V%X V%X", x, y);
  uint8_t vx = emu->V[x];
  uint8_t vy = emu->V[y];
  emu->V[x] = vx - vy;
  if (vx > vy) {
    emu->V[0xF] = 1;
  } else {
    emu->V[0xF] = 0;
  }
  emu->PC += 2;
}

void shr_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "SHR V%X V%X", x, y);
  uint8_t vx = emu->V[x];
  emu->V[x] >>= 1;
  if (vx & 1) {
    emu->V[0xF] = 1;
  } else {
    emu->V[0xF] = 0;
  }
  emu->PC += 2;
}

void subn_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "ADD V%X V%X", x, y);
  uint8_t vx = emu->V[x];
  uint8_t vy = emu->V[y];
  emu->V[x] = vy - vx;
  if (vy > vx) {
    emu->V[0xF] = 1;
  } else {
    emu->V[0xF] = 0;
  }
  emu->PC += 2;
}

void shl_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "ADD V%X V%X", x, y);
  uint8_t vx = emu->V[x];
  emu->V[x] <<= 1;
  if ((vx >> 7) & 1) {
    emu->V[0xF] = 1;
  } else {
    emu->V[0xF] = 0;
  }
  emu->PC += 2;
}

void sne_Vx_Vy(Emulator *emu, int x, int y) {
  sprintf(emu->inst, "SNE V%X V%X", x, y);
  if (emu->V[x] != emu->V[y]) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void ld_I_addr(Emulator *emu, int addr) {
  sprintf(emu->inst, "LD I %3X", addr);
  emu->I = addr;
  emu->PC += 2;
}

void jp_V0_addr(Emulator *emu, int addr) {
  sprintf(emu->inst, "JP V0 %3X", addr);
  emu->PC = emu->V[0] + addr;
}

void rnd_Vx_byte(Emulator *emu, int x, int byte) {
  sprintf(emu->inst, "RND V%X %02X", x, byte);
  emu->V[x] = (int)(rand() * 256.0 / (1.0 + RAND_MAX)) & byte;
  emu->PC += 2;
}

void drw_Vx_Vy_nibble(Emulator *emu, int x, int y, int n) {
  sprintf(emu->inst, "DRW V%X V%X %X", x, y, n);

  int vx = emu->V[x];
  int vy = emu->V[y];
  emu->V[0xF] = 0;
  for (int i = 0; i < n; i++) {
    uint8_t sp = emu->memory[emu->I + i];
    int dx = 1;
    int vx2 = vx;

    for (int j = 0; j < 8; j++) {
      if (emu->pixel[vy][vx2] & !(emu->pixel[vy][vx2] ^ (sp >> (7 - j) & 1))) {
        emu->V[0xF] = 1;
      }
      emu->pixel[vy][vx2] ^= (sp >> (7 - j)) & 1;
      if (vx2 >= DISPLAY_WIDTH - 1) {
        dx = -1;
      }
      vx2 += dx;
    }
    if (vy < DISPLAY_HEIGHT) {
      vy++;
    } else {
      break;
    }
  }
  emu->PC += 2;

  display_screen(emu);
}

void skp_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "SKP V%X", x);
  if (emu->key == emu->V[x]) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void sknp_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "SKNP V%X", x);
  if (emu->key != emu->V[x]) {
    emu->PC += 2;
  }
  emu->PC += 2;
}

void ld_Vx_DT(Emulator *emu, int x) {
  sprintf(emu->inst, "LS V%X DT", x);
  emu->V[x] = emu->DT;
  emu->PC += 2;
}

void ld_Vx_K(Emulator *emu, int x) {
  sprintf(emu->inst, "LD V%X K", x);
  if (emu->key >= 0) {
    emu->V[x] = emu->key;
    emu->PC += 2;
  }
}

void ld_DT_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "LD DT V%X", x);
  emu->DT = emu->V[x];
  emu->PC += 2;
}

void ld_ST_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "LD ST V%X", x);
  emu->ST = emu->V[x];
  emu->PC += 2;
}

void add_I_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "ADD I V%X", x);
  emu->I += emu->V[x];
  emu->PC += 2;
}

void ld_F_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "LD F V%X", x);
  emu->I = 5 * (emu->V[x] & 0xF);
  emu->PC += 2;
}

void ld_B_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "LD B V%X", x);
  int n = emu->V[x];
  for (int i = 0; i < 3; i++) {
    emu->memory[emu->I + 2 - i] = n % 10;
    n /= 10;
  }
  emu->PC += 2;
}

void ld_I_Vx(Emulator *emu, int x) {
  sprintf(emu->inst, "LD [I] V%X", x);
  for (int i = 0; i < x + 1; i++) {
    emu->memory[emu->I + i] = emu->V[i];
  }
  emu->PC += 2;
}

void ld_Vx_I(Emulator *emu, int x) {
  sprintf(emu->inst, "LD V%X [I]", x);
  for (int i = 0; i < x + 1; i++) {
    emu->V[i] = emu->memory[emu->I + i];
  }
  emu->PC += 2;
}

void instruction(Emulator *emu) {
  emu->old_PC = emu->PC;
  int o1 = (emu->memory[emu->PC] >> 4) & 0xF;
  int o2 = emu->memory[emu->PC] & 0xF;
  int o3 = (emu->memory[emu->PC + 1] >> 4) & 0xF;
  int o4 = emu->memory[emu->PC + 1] & 0xF;

  switch (o1) {
  case 0:
    if (o2 == 0 && o3 == 0xE && o4 == 0) {
      cls(emu);
    } else if (o2 == 0 && o3 == 0xE && o4 == 0xE) {
      ret(emu);
    } else {
      sys_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    }
    break;
  case 1:
    jp_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    break;

  case 2:
    call_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    break;

  case 3:
    se_Vx_byte(emu, o2, (o3 << 4) | o4);
    break;

  case 4:
    sne_Vx_byte(emu, o2, (o3 << 4) | o4);
    break;

  case 5:
    se_Vx_Vy(emu, o2, o3);
    break;

  case 6:
    ld_Vx_byte(emu, o2, (o3 << 4) | o4);
    break;

  case 7:
    add_Vx_byte(emu, o2, (o3 << 4) | o4);
    break;

  case 8:
    switch (o4) {
    case 0:
      ld_Vx_Vy(emu, o2, o3);
      break;
    case 1:
      or_Vx_Vy(emu, o2, o3);
      break;
    case 2:
      and_Vx_Vy(emu, o2, o3);
      break;
    case 3:
      xor_Vx_Vy(emu, o2, o3);
      break;
    case 4:
      add_Vx_Vy(emu, o2, o3);
      break;
    case 5:
      sub_Vx_Vy(emu, o2, o3);
      break;
    case 6:
      shr_Vx_Vy(emu, o2, o3);
      break;
    case 7:
      subn_Vx_Vy(emu, o2, o3);
      break;
    case 0xE:
      shl_Vx_Vy(emu, o2, o3);
      break;
    default:
      sprintf(emu->inst, "nothing");
      break;
    }
    break;
  case 9:
    sne_Vx_Vy(emu, o2, o3);
    break;
  case 0xA:
    ld_I_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    break;
  case 0xB:
    ld_I_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    break;
  case 0xC:
    sne_Vx_byte(emu, o2, (o3 << 4) | o4);
    break;
    ld_I_addr(emu, (o2 << 8) | (o3 << 4) | o4);
    break;
  case 0xD:
    drw_Vx_Vy_nibble(emu, o2, o3, o4);
    break;
  case 0xE:
    if (o3 == 9 && o4 == 0xE) {
      skp_Vx(emu, o2);
    } else if (o3 == 0xA && o4 == 1) {
      sknp_Vx(emu, o2);
    } else {
      sprintf(emu->inst, "nothing");
    }
    break;
  case 0xF:
    if (o3 == 0 && o4 == 7) {
      ld_Vx_DT(emu, o2);
    } else if (o3 == 0 && o4 == 0xA) {
      ld_Vx_K(emu, o2);
    } else if (o3 == 1 && o4 == 5) {
      ld_DT_Vx(emu, o2);
    } else if (o3 == 1 && o4 == 8) {
      ld_ST_Vx(emu, o2);
    } else if (o3 == 1 && o4 == 0xE) {
      add_I_Vx(emu, o2);
    } else if (o3 == 2 && o4 == 9) {
      ld_F_Vx(emu, o2);
    } else if (o3 == 3 && o4 == 3) {
      ld_B_Vx(emu, o2);
    } else if (o3 == 5 && o4 == 5) {
      ld_I_Vx(emu, o2);
    } else if (o3 == 6 && o4 == 5) {
      ld_Vx_I(emu, o2);
    } else {
      sprintf(emu->inst, "nothing");
    }
    break;
  }
}

static Emulator *create_emu(int size, int pc) {
  Emulator *emu = malloc(sizeof(Emulator));
  emu->memory = malloc(size);

  memset(emu->V, 0, sizeof(emu->V));
  memset(emu->pixel, 0, sizeof(emu->pixel));

  int preset[] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80, // F
  };
  for (int i = 0; i < sizeof(preset) / sizeof(int); i++) {
    emu->memory[i] = preset[i];
  }

  emu->PC = pc;
  emu->key = -1;

  return emu;
}

void display_emu(Emulator *emu) {
  clear();
  mvprintw(VALUE_Y, VALUE_X, "         PC:0x%X\n", emu->old_PC);
  mvprintw(VALUE_Y + 1, VALUE_X, "    opecode: %02X%02X",
           emu->memory[emu->old_PC], emu->memory[emu->old_PC + 1]);
  mvprintw(VALUE_Y + 2, VALUE_X, "instruction: %s", emu->inst);

  get_inst(emu, emu->PC, emu->next_inst);
  mvprintw(VALUE_Y + 0, VALUE_X + 30, "    next PC:0x%X\n", emu->PC);
  mvprintw(VALUE_Y + 1, VALUE_X + 30, "    opecode: %02X%02X",
           emu->memory[emu->PC], emu->memory[emu->PC + 1]);
  mvprintw(VALUE_Y + 2, VALUE_X + 30, "instruction: %s", emu->next_inst);

  mvprintw(VALUE_Y + 3, VALUE_X, "SP:0x%X\n", emu->SP);
  mvprintw(VALUE_Y + 3, VALUE_X + 10, " I:0x%X\n", emu->I);

  for (int i = 0; i < 16; i++) {
    mvprintw(VALUE_Y + 4 + i, VALUE_X, "V%X: %02X\n", i, emu->V[i]);
  }
  for (int i = 0; i < 16; i++) {
    mvprintw(VALUE_Y + 4 + i, VALUE_X + 10, "stack[%X]: %04X\n", i,
             emu->stack[i]);
  }
  mvprintw(VALUE_Y + 21, VALUE_X, "key: %X", emu->key);
  mvprintw(VALUE_Y + 21, VALUE_X + 20, "DT: %X", emu->DT);
}

int main(int argc, char *argv[]) {
  initscr();
  noecho();
  curs_set(0);
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_WHITE);
  if (!screenInit()) {
    printf("Failed to initialize\n");
    return -1;
  }

  FILE *binary;
  Emulator *emu;

  if (argc != 2) {
    printf("input filename\n");
    return 1;
  }

  emu = create_emu(MEMORY_SIZE, START_POINT);

  binary = fopen(argv[1], "rb");
  if (binary == NULL) {
    printf("cannot open file %s\n", argv[1]);
    return 1;
  }

  struct stat st;
  stat(argv[1], &st);

  fread(emu->memory + START_POINT, 1, st.st_size, binary);

  //  for (int i = 0; i < st.st_size / 2; i++) {
  //    printf("%04X\n", emu->memory[i]);
  //  }
  int i = 0;
  SDL_Event e;
  int delayTimes = 0;
  bool quit = false;
  while (quit == false) {
    emu->key = -1;
//    emu->pixel[0][0] = 1;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < 16; i++) {
      if (state[CHIP8_KEYS[i]]) {
        emu->key = i;
      }
    }

    instruction(emu);

    display_emu(emu);
//    display_screen(emu);
    refresh();
    SDL_Delay(DELAY_TIME);
    if (delayTimes == DT_DEC_TIME) {
      if (emu->DT != 0) {
        emu->DT--;
      }
      delayTimes = 0;
    } else {
      delayTimes++;
    }
    //         usleep(30000);
    // getch();
  }

  endwin();
  screenClose();

  return 0;
}
