#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>

#define TEXT_MAX   16384
#define LINE_MAX   1536
#define MAX_FILES  40
#define NAME_LEN   56
#define WIDTH      126
#define TEXT_TOP   9
#define TEXT_H     55

#define SAVE_DIR EXT_PATH("apps_data/bt_reader")

//CORE_BEGIN
typedef struct { uint16_t cp; uint8_t c[5]; } Gl;
typedef struct { uint8_t h, gap; uint8_t rows[3]; } Acc;
typedef struct { uint16_t cp; uint8_t acc; uint8_t script; } Tok;
typedef struct { uint8_t script; uint8_t group; } TS;
typedef struct { const char* name; uint16_t cp; } Cmd;

static const uint8_t FONT_ASCII[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5E,0x00,0x00}, {0x00,0x0E,0x00,0x0E,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x04,0x2A,0x7F,0x2A,0x10}, {0x00,0x16,0x08,0x34,0x00}, {0x36,0x49,0x36,0x40,0x00}, {0x00,0x00,0x0E,0x00,0x00},
    {0x00,0x3C,0x42,0x00,0x00}, {0x00,0x42,0x3C,0x00,0x00}, {0x54,0x38,0x38,0x54,0x00}, {0x10,0x10,0x7C,0x10,0x10},
    {0x00,0x80,0x60,0x20,0x00}, {0x10,0x10,0x10,0x10,0x00}, {0x00,0x40,0xE0,0x40,0x00}, {0x60,0x10,0x08,0x06,0x00},
    {0x00,0x3C,0x42,0x3C,0x00}, {0x00,0x44,0x7E,0x40,0x00}, {0x64,0x52,0x52,0x4C,0x00}, {0x22,0x4A,0x4E,0x32,0x00},
    {0x18,0x14,0x7E,0x10,0x00}, {0x2E,0x4A,0x4A,0x32,0x00}, {0x3C,0x4A,0x4A,0x30,0x00}, {0x02,0x62,0x1A,0x06,0x00},
    {0x34,0x4A,0x4A,0x34,0x00}, {0x0C,0x52,0x52,0x3C,0x00}, {0x00,0x6C,0x6C,0x00,0x00}, {0x00,0x80,0x6C,0x2C,0x00},
    {0x00,0x18,0x24,0x42,0x00}, {0x28,0x28,0x28,0x28,0x00}, {0x00,0x42,0x24,0x18,0x00}, {0x00,0x04,0x52,0x0C,0x00},
    {0x3C,0x42,0x99,0xA5,0x1E}, {0x7C,0x12,0x12,0x7C,0x00}, {0x7E,0x4A,0x4A,0x34,0x00}, {0x3C,0x42,0x42,0x24,0x00},
    {0x7E,0x42,0x42,0x3C,0x00}, {0x7E,0x4A,0x4A,0x42,0x00}, {0x7E,0x0A,0x0A,0x02,0x00}, {0x3C,0x42,0x52,0x34,0x00},
    {0x7E,0x08,0x08,0x7E,0x00}, {0x00,0x42,0x7E,0x42,0x00}, {0x20,0x42,0x3E,0x02,0x00}, {0x7E,0x08,0x34,0x42,0x00},
    {0x7E,0x40,0x40,0x40,0x00}, {0x7E,0x0C,0x0C,0x7E,0x00}, {0x7E,0x0C,0x38,0x7E,0x00}, {0x3C,0x42,0x42,0x3C,0x00},
    {0x7E,0x12,0x12,0x0C,0x00}, {0x3C,0x52,0x62,0xBC,0x00}, {0x7E,0x12,0x12,0x6C,0x00}, {0x24,0x4A,0x52,0x24,0x00},
    {0x00,0x02,0x7E,0x02,0x00}, {0x3E,0x40,0x40,0x3E,0x00}, {0x1E,0x60,0x60,0x1E,0x00}, {0x7E,0x30,0x30,0x7E,0x00},
    {0x66,0x18,0x18,0x66,0x00}, {0x06,0x08,0x70,0x08,0x06}, {0x62,0x52,0x4A,0x46,0x00}, {0x00,0x7E,0x42,0x42,0x00},
    {0x06,0x08,0x10,0x60,0x00}, {0x00,0x42,0x42,0x7E,0x00}, {0x00,0x04,0x02,0x04,0x00}, {0x80,0x80,0x80,0x80,0x00},
    {0x00,0x02,0x04,0x00,0x00}, {0x30,0x48,0x48,0x78,0x00}, {0x7E,0x48,0x48,0x30,0x00}, {0x00,0x30,0x48,0x48,0x00},
    {0x30,0x48,0x48,0x7E,0x00}, {0x30,0x68,0x58,0x10,0x00}, {0x10,0x7C,0x12,0x04,0x00}, {0x10,0xA8,0xA8,0x70,0x00},
    {0x7E,0x08,0x08,0x70,0x00}, {0x00,0x48,0x7A,0x40,0x00}, {0x00,0x40,0x80,0x7A,0x00}, {0x7E,0x10,0x10,0x68,0x00},
    {0x00,0x42,0x7E,0x40,0x00}, {0x78,0x08,0x70,0x08,0x70}, {0x78,0x08,0x08,0x70,0x00}, {0x30,0x48,0x48,0x30,0x00},
    {0xF8,0x28,0x28,0x10,0x00}, {0x10,0x28,0x28,0xF8,0x00}, {0x78,0x10,0x08,0x10,0x00}, {0x00,0x50,0x58,0x28,0x00},
    {0x08,0x3E,0x48,0x20,0x00}, {0x38,0x40,0x40,0x78,0x00}, {0x00,0x38,0x40,0x38,0x00}, {0x38,0x40,0x30,0x40,0x38},
    {0x48,0x30,0x30,0x48,0x00}, {0x58,0xA0,0xA0,0x78,0x00}, {0x48,0x68,0x58,0x48,0x00}, {0x08,0x2A,0x55,0x41,0x00},
    {0x00,0x00,0x7E,0x00,0x00}, {0x41,0x55,0x2A,0x08,0x00}, {0x04,0x02,0x04,0x02,0x00},
};

static const uint8_t FONT_CYR[66][5] = {
    {0x7C,0x12,0x12,0x7C,0x00}, {0x7E,0x4A,0x4A,0x30,0x00}, {0x7E,0x4A,0x4A,0x34,0x00}, {0x7E,0x02,0x02,0x06,0x00},
    {0xFC,0x42,0x7E,0xC0,0x00}, {0x7E,0x4A,0x4A,0x42,0x00}, {0x76,0x08,0x7E,0x08,0x76}, {0x24,0x42,0x4A,0x34,0x00},
    {0x7E,0x10,0x08,0x7E,0x00}, {0x7E,0x11,0x09,0x7E,0x00}, {0x7E,0x08,0x14,0x62,0x00}, {0x40,0x3C,0x02,0x7E,0x00},
    {0x7E,0x0C,0x0C,0x7E,0x00}, {0x7E,0x08,0x08,0x7E,0x00}, {0x3C,0x42,0x42,0x3C,0x00}, {0x7E,0x02,0x02,0x7E,0x00},
    {0x7E,0x12,0x12,0x0C,0x00}, {0x3C,0x42,0x42,0x24,0x00}, {0x00,0x02,0x7E,0x02,0x00}, {0x4E,0x50,0x50,0x3E,0x00},
    {0x18,0x24,0x7E,0x24,0x18}, {0x00,0x66,0x18,0x66,0x00}, {0x7E,0x40,0x7E,0xC0,0x00}, {0x00,0x0E,0x10,0x7E,0x00},
    {0x7E,0x40,0x7E,0x40,0x7E}, {0x7E,0x40,0x7E,0x40,0xFE}, {0x02,0x02,0x7E,0x48,0x30}, {0x7E,0x48,0x30,0x00,0x7E},
    {0x00,0x7E,0x48,0x30,0x00}, {0x42,0x4A,0x4A,0x3C,0x00}, {0x7E,0x08,0x3C,0x42,0x3C}, {0x6C,0x12,0x12,0x7E,0x00},
    {0x30,0x48,0x48,0x78,0x00}, {0x3C,0x4A,0x4A,0x31,0x00}, {0x78,0x58,0x58,0x20,0x00}, {0x00,0x78,0x08,0x08,0x00},
    {0xE0,0x58,0x48,0xF8,0x00}, {0x30,0x68,0x58,0x10,0x00}, {0x68,0x10,0x78,0x10,0x68}, {0x00,0x48,0x58,0x38,0x00},
    {0x78,0x20,0x10,0x78,0x00}, {0x78,0x22,0x12,0x78,0x00}, {0x78,0x10,0x30,0x48,0x00}, {0x40,0x30,0x08,0x78,0x00},
    {0x78,0x10,0x60,0x10,0x78}, {0x78,0x10,0x10,0x78,0x00}, {0x30,0x48,0x48,0x30,0x00}, {0x78,0x08,0x08,0x78,0x00},
    {0xF8,0x48,0x48,0x30,0x00}, {0x00,0x30,0x48,0x48,0x00}, {0x00,0x08,0x78,0x08,0x00}, {0x18,0xA0,0x40,0x38,0x00},
    {0x00,0x78,0xCE,0x78,0x00}, {0x48,0x30,0x30,0x48,0x00}, {0x78,0x40,0x40,0xF8,0x00}, {0x18,0x20,0x20,0x78,0x00},
    {0x78,0x40,0x78,0x40,0x78}, {0x78,0x40,0x78,0x40,0xF8}, {0x08,0x78,0x50,0x20,0x00}, {0x78,0x50,0x20,0x78,0x00},
    {0x78,0x50,0x50,0x20,0x00}, {0x00,0x48,0x58,0x30,0x00}, {0x78,0x30,0x48,0x30,0x00}, {0x00,0x50,0x28,0x78,0x00},
    {0x7E,0x4A,0x4A,0x42,0x00}, {0x32,0x68,0x5A,0x10,0x00},
};

static const Gl EXTRA[] = {
    {0x00B0,{0x00,0x04,0x0A,0x04,0x00}}, {0x00B1,{0x00,0x48,0x5C,0x48,0x00}},
    {0x00B5,{0xF8,0x00,0x40,0x78,0x00}}, {0x00B7,{0x00,0x00,0x10,0x00,0x00}},
    {0x00D7,{0x44,0x28,0x10,0x28,0x44}}, {0x00F7,{0x10,0x10,0x54,0x10,0x10}},
    {0x0391,{0x7C,0x12,0x12,0x7C,0x00}}, {0x0392,{0x7E,0x4A,0x4A,0x34,0x00}},
    {0x0393,{0x7E,0x02,0x02,0x06,0x00}}, {0x0394,{0x70,0x4C,0x42,0x4C,0x70}},
    {0x0395,{0x7E,0x4A,0x4A,0x42,0x00}}, {0x0396,{0x62,0x52,0x4A,0x46,0x00}},
    {0x0397,{0x7E,0x08,0x08,0x7E,0x00}}, {0x0398,{0x3C,0x52,0x52,0x52,0x3C}},
    {0x0399,{0x00,0x42,0x7E,0x42,0x00}}, {0x039A,{0x7E,0x08,0x34,0x42,0x00}},
    {0x039B,{0x70,0x0C,0x02,0x0C,0x70}}, {0x039C,{0x7E,0x0C,0x0C,0x7E,0x00}},
    {0x039D,{0x7E,0x0C,0x38,0x7E,0x00}}, {0x039E,{0x42,0x4A,0x4A,0x4A,0x42}},
    {0x039F,{0x3C,0x42,0x42,0x3C,0x00}}, {0x03A0,{0x7E,0x02,0x02,0x7E,0x00}},
    {0x03A1,{0x7E,0x12,0x12,0x0C,0x00}}, {0x03A3,{0x42,0x66,0x5A,0x42,0x42}},
    {0x03A4,{0x00,0x02,0x7E,0x02,0x00}}, {0x03A5,{0x06,0x08,0x70,0x08,0x06}},
    {0x03A6,{0x18,0x24,0x7E,0x24,0x18}}, {0x03A7,{0x66,0x18,0x18,0x66,0x00}},
    {0x03A8,{0x0E,0x10,0x7E,0x10,0x0E}}, {0x03A9,{0x5C,0x62,0x02,0x62,0x5C}},
    {0x03B1,{0x30,0x48,0x48,0x30,0x48}}, {0x03B2,{0xFC,0x12,0x12,0x6C,0x00}},
    {0x03B3,{0x18,0x20,0xE0,0x18,0x00}}, {0x03B4,{0x30,0x4C,0x4A,0x4A,0x30}},
    {0x03B5,{0x10,0x68,0x68,0x48,0x00}}, {0x03B6,{0x32,0x4A,0x46,0xC2,0x00}},
    {0x03B7,{0x78,0x10,0x08,0x08,0xF0}}, {0x03B8,{0x3C,0x4A,0x4A,0x3C,0x00}},
    {0x03B9,{0x00,0x38,0x40,0x40,0x00}}, {0x03BA,{0x78,0x10,0x30,0x48,0x00}},
    {0x03BB,{0x62,0x1C,0x10,0x20,0x40}}, {0x03BC,{0xF8,0x00,0x40,0x78,0x00}},
    {0x03BD,{0x00,0x38,0x40,0x38,0x00}}, {0x03BE,{0x34,0x4A,0x4A,0xC2,0x00}},
    {0x03BF,{0x30,0x48,0x48,0x30,0x00}}, {0x03C0,{0x08,0x78,0x08,0x38,0x48}},
    {0x03C1,{0xF0,0x48,0x48,0x30,0x00}}, {0x03C2,{0x30,0x48,0x48,0x38,0x08}},
    {0x03C3,{0x30,0x48,0x48,0x38,0x08}}, {0x03C4,{0x08,0x08,0x38,0x48,0x48}},
    {0x03C5,{0x38,0x40,0x40,0x78,0x00}}, {0x03C6,{0x30,0x48,0xFE,0x48,0x30}},
    {0x03C7,{0x88,0x50,0x20,0x50,0x88}}, {0x03C8,{0x38,0x40,0xFE,0x40,0x38}},
    {0x03C9,{0x38,0x40,0x30,0x40,0x38}}, {0x03D1,{0x3C,0x4A,0x4A,0x3C,0x00}},
    {0x03D5,{0x30,0x48,0xFE,0x48,0x30}}, {0x03F5,{0x10,0x68,0x68,0x48,0x00}},
    {0x210F,{0x7E,0x04,0x04,0x78,0x00}}, {0x2126,{0x5C,0x62,0x02,0x62,0x5C}},
    {0x2190,{0x10,0x38,0x54,0x10,0x10}}, {0x2191,{0x08,0x04,0x7E,0x04,0x08}},
    {0x2192,{0x10,0x10,0x54,0x38,0x10}}, {0x2193,{0x10,0x20,0x7E,0x20,0x10}},
    {0x2202,{0x30,0x48,0x4A,0x3E,0x00}}, {0x2206,{0x70,0x4C,0x42,0x4C,0x70}},
    {0x2207,{0x06,0x1A,0x22,0x1A,0x06}}, {0x2208,{0x38,0x54,0x54,0x54,0x00}},
    {0x2211,{0x42,0x66,0x5A,0x42,0x42}}, {0x2218,{0x00,0x04,0x0A,0x04,0x00}},
    {0x221A,{0x20,0x40,0x3E,0x02,0x02}}, {0x221D,{0x30,0x48,0x48,0x30,0x48}},
    {0x221E,{0x10,0x28,0x10,0x28,0x10}}, {0x2225,{0x00,0x7E,0x00,0x7E,0x00}},
    {0x222B,{0x80,0x80,0x7C,0x02,0x02}}, {0x2248,{0x48,0x24,0x24,0x48,0x24}},
    {0x2260,{0x28,0x68,0x38,0x2C,0x28}}, {0x2264,{0x00,0x48,0x54,0x62,0x00}},
    {0x2265,{0x00,0x62,0x54,0x48,0x00}}, {0x22A5,{0x40,0x40,0x7C,0x40,0x40}},
    {0x22C5,{0x00,0x00,0x10,0x00,0x00}},
};
#define EXTRA_N (sizeof(EXTRA) / sizeof(EXTRA[0]))

static const uint8_t MINI[41][3] = {
    {0x1F,0x11,0x1F}, {0x12,0x1F,0x10}, {0x1D,0x15,0x17}, {0x15,0x15,0x1F}, {0x07,0x04,0x1F}, {0x17,0x15,0x1D},
    {0x1F,0x15,0x1D}, {0x01,0x1D,0x03}, {0x1F,0x15,0x1F}, {0x17,0x15,0x1F}, {0x04,0x0E,0x04}, {0x04,0x04,0x04},
    {0x0A,0x0A,0x0A}, {0x0E,0x11,0x00}, {0x00,0x11,0x0E}, {0x1E,0x05,0x1E}, {0x1F,0x15,0x0A}, {0x0E,0x11,0x11},
    {0x1F,0x11,0x0E}, {0x1F,0x15,0x11}, {0x1F,0x05,0x01}, {0x0E,0x11,0x1D}, {0x1F,0x04,0x1F}, {0x11,0x1F,0x11},
    {0x08,0x10,0x0F}, {0x1F,0x04,0x1B}, {0x1F,0x10,0x10}, {0x1F,0x06,0x1F}, {0x1F,0x01,0x1E}, {0x0E,0x11,0x0E},
    {0x1F,0x05,0x02}, {0x0E,0x19,0x1E}, {0x1F,0x05,0x1A}, {0x12,0x15,0x09}, {0x01,0x1F,0x01}, {0x1F,0x10,0x1F},
    {0x0F,0x10,0x0F}, {0x1F,0x0C,0x1F}, {0x1B,0x04,0x1B}, {0x03,0x1C,0x03}, {0x19,0x15,0x13},
};

static const Acc ACCS[7] = {
    {0,0,{0,0,0}},
    {3,0,{0x08,0x1F,0x08}},
    {1,1,{0x04,0x00,0x00}},
    {1,1,{0x0A,0x00,0x00}},
    {2,0,{0x04,0x0A,0x00}},
    {1,0,{0x1F,0x00,0x00}},
    {2,0,{0x06,0x18,0x00}}
};

static const Cmd CMDS[] = {
    {"alpha", 0x03B1},
    {"beta", 0x03B2},
    {"gamma", 0x03B3},
    {"delta", 0x03B4},
    {"epsilon", 0x03B5},
    {"varepsilon", 0x03B5},
    {"zeta", 0x03B6},
    {"eta", 0x03B7},
    {"theta", 0x03B8},
    {"vartheta", 0x03B8},
    {"iota", 0x03B9},
    {"kappa", 0x03BA},
    {"lambda", 0x03BB},
    {"mu", 0x03BC},
    {"micro", 0x03BC},
    {"nu", 0x03BD},
    {"xi", 0x03BE},
    {"pi", 0x03C0},
    {"rho", 0x03C1},
    {"sigma", 0x03C3},
    {"tau", 0x03C4},
    {"upsilon", 0x03C5},
    {"phi", 0x03C6},
    {"varphi", 0x03C6},
    {"chi", 0x03C7},
    {"psi", 0x03C8},
    {"omega", 0x03C9},
    {"Gamma", 0x0393},
    {"Delta", 0x0394},
    {"Theta", 0x0398},
    {"Lambda", 0x039B},
    {"Xi", 0x039E},
    {"Pi", 0x03A0},
    {"Sigma", 0x03A3},
    {"Phi", 0x03A6},
    {"Psi", 0x03A8},
    {"Omega", 0x03A9},
    {"ohm", 0x03A9},
    {"pm", 0x00B1},
    {"times", 0x00D7},
    {"div", 0x00F7},
    {"cdot", 0x00B7},
    {"deg", 0x00B0},
    {"circ", 0x00B0},
    {"degree", 0x00B0},
    {"sqrt", 0x221A},
    {"infty", 0x221E},
    {"approx", 0x2248},
    {"neq", 0x2260},
    {"ne", 0x2260},
    {"leq", 0x2264},
    {"le", 0x2264},
    {"geq", 0x2265},
    {"ge", 0x2265},
    {"to", 0x2192},
    {"rightarrow", 0x2192},
    {"leftarrow", 0x2190},
    {"uparrow", 0x2191},
    {"downarrow", 0x2193},
    {"partial", 0x2202},
    {"nabla", 0x2207},
    {"int", 0x222B},
    {"sum", 0x2211},
    {"hbar", 0x210F},
    {"propto", 0x221D},
    {"in", 0x2208},
    {"perp", 0x22A5},
    {"parallel", 0x2225},
    {"Rightarrow", 0x2192},
    {"Leftarrow", 0x2190}
};
static const char* const ACC_NAMES[6] = {"vec", "dot", "ddot", "hat", "bar", "tilde"};

static uint32_t next_cp(const char* t, size_t len, size_t* i) {
    uint8_t c = (uint8_t)t[*i];
    (*i)++;
    if(c < 0x80) return c;
    if((c & 0xE0) == 0xC0 && *i < len) {
        uint8_t c2 = (uint8_t)t[*i];
        if((c2 & 0xC0) == 0x80) {
            (*i)++;
            return ((uint32_t)(c & 0x1F) << 6) | (c2 & 0x3F);
        }
    } else if((c & 0xF0) == 0xE0 && *i + 1 < len) {
        uint8_t c2 = (uint8_t)t[*i];
        uint8_t c3 = (uint8_t)t[*i + 1];
        if((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
            *i += 2;
            return ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) | (c3 & 0x3F);
        }
    } else if((c & 0xF8) == 0xF0) {
        size_t k = 0;
        while(k < 3 && *i < len && ((uint8_t)t[*i] & 0xC0) == 0x80) {
            (*i)++;
            k++;
        }
    }
    return '?';
}

static const uint8_t* glyph_for(uint32_t cp) {
    if(cp == '\t' || cp == 0xA0) cp = ' ';
    if(cp == 0x2013 || cp == 0x2014 || cp == 0x2212) cp = '-';
    if(cp == 0x2018 || cp == 0x2019) cp = '\'';
    if(cp == 0x201C || cp == 0x201D || cp == 0xAB || cp == 0xBB) cp = '"';
    if(cp >= 0x20 && cp <= 0x7E) return FONT_ASCII[cp - 0x20];
    if(cp >= 0x410 && cp <= 0x44F) return FONT_CYR[cp - 0x410];
    if(cp == 0x401) return FONT_CYR[64];
    if(cp == 0x451) return FONT_CYR[65];
    size_t lo = 0, hi = EXTRA_N;
    while(lo < hi) {
        size_t mid = (lo + hi) / 2;
        if(EXTRA[mid].cp == cp) return EXTRA[mid].c;
        if(EXTRA[mid].cp < cp) lo = mid + 1;
        else hi = mid;
    }
    return FONT_ASCII['?' - 0x20];
}

static int mini_idx(uint32_t cp) {
    if(cp >= '0' && cp <= '9') return (int)(cp - '0');
    if(cp == '+') return 10;
    if(cp == '-' || cp == 0x2212) return 11;
    if(cp == '=') return 12;
    if(cp == '(') return 13;
    if(cp == ')') return 14;
    if(cp >= 'a' && cp <= 'z') return 15 + (int)(cp - 'a');
    if(cp >= 'A' && cp <= 'Z') return 15 + (int)(cp - 'A');
    return -1;
}

static int top_row(const uint8_t* g) {
    uint8_t o = g[0] | g[1] | g[2] | g[3] | g[4];
    if(!o) return 3;
    int t = 0;
    while(!(o & (1 << t))) t++;
    return t;
}

static bool is_alpha(uint8_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int cmd_lookup(const char* s, size_t n) {
    for(size_t k = 0; k < sizeof(CMDS) / sizeof(CMDS[0]); k++) {
        if(strlen(CMDS[k].name) == n && memcmp(CMDS[k].name, s, n) == 0) return CMDS[k].cp;
    }
    return 0;
}

static int acc_lookup(const char* s, size_t n) {
    for(int k = 0; k < 6; k++) {
        if(strlen(ACC_NAMES[k]) == n && memcmp(ACC_NAMES[k], s, n) == 0) return k + 1;
    }
    return 0;
}

static int acc_from_mark(uint32_t m) {
    if(m == 0x20D7 || m == 0x20D1) return 1;
    if(m == 0x0307) return 2;
    if(m == 0x0308) return 3;
    if(m == 0x0302) return 4;
    if(m == 0x0304 || m == 0x0305) return 5;
    if(m == 0x0303) return 6;
    return 0;
}

static bool script_map(uint32_t cp, uint16_t* base, uint8_t* sc) {
    *sc = 1;
    if(cp == 0xB9) { *base = '1'; return true; }
    if(cp == 0xB2) { *base = '2'; return true; }
    if(cp == 0xB3) { *base = '3'; return true; }
    if(cp == 0x2070) { *base = '0'; return true; }
    if(cp >= 0x2074 && cp <= 0x2079) { *base = (uint16_t)('0' + (cp - 0x2070)); return true; }
    if(cp == 0x207A) { *base = '+'; return true; }
    if(cp == 0x207B) { *base = '-'; return true; }
    if(cp == 0x207C) { *base = '='; return true; }
    if(cp == 0x207D) { *base = '('; return true; }
    if(cp == 0x207E) { *base = ')'; return true; }
    if(cp == 0x207F) { *base = 'n'; return true; }
    *sc = 2;
    if(cp >= 0x2080 && cp <= 0x2089) { *base = (uint16_t)('0' + (cp - 0x2080)); return true; }
    if(cp == 0x208A) { *base = '+'; return true; }
    if(cp == 0x208B) { *base = '-'; return true; }
    if(cp == 0x208C) { *base = '='; return true; }
    if(cp == 0x208D) { *base = '('; return true; }
    if(cp == 0x208E) { *base = ')'; return true; }
    static const char SUBL[] = "a e o x . h k l m n p s t";
    if(cp >= 0x2090 && cp <= 0x209C && SUBL[(cp - 0x2090) * 2] != '.') {
        *base = (uint16_t)SUBL[(cp - 0x2090) * 2];
        return true;
    }
    return false;
}

static uint16_t read_base(const char* t, size_t len, size_t* k) {
    if(t[*k] == '\\') {
        size_t j = *k + 1;
        while(j < len && is_alpha((uint8_t)t[j]) && j - *k - 1 < 12) j++;
        int sym = (j > *k + 1) ? cmd_lookup(t + *k + 1, j - *k - 1) : 0;
        if(sym) {
            *k = j;
            return (uint16_t)sym;
        }
        (*k)++;
        return '\\';
    }
    return (uint16_t)next_cp(t, len, k);
}

static size_t next_tok(const char* t, size_t len, size_t i, TS* st, Tok* o) {
    o->cp = 0;
    o->acc = 0;
    o->script = st->script;
    while(i < len) {
        uint8_t c = (uint8_t)t[i];
        if(st->group && c == '}') {
            st->group = 0;
            st->script = 0;
            i++;
            continue;
        }
        o->script = st->script;
        if(c == '\\') {
            size_t j = i + 1;
            while(j < len && is_alpha((uint8_t)t[j]) && j - i - 1 < 12) j++;
            size_t nl = j - i - 1;
            if(nl == 0) {
                if(j < len && (uint8_t)t[j] < 0x80 && t[j] != '\n') {
                    o->cp = (uint8_t)t[j];
                    return j + 1;
                }
                o->cp = '\\';
                return i + 1;
            }
            int a = acc_lookup(t + i + 1, nl);
            if(a) {
                size_t k = j;
                while(k < len && t[k] == ' ') k++;
                bool br = (k < len && t[k] == '{');
                if(br) k++;
                if(k >= len || t[k] == '\n') {
                    o->cp = '\\';
                    return i + 1;
                }
                o->cp = read_base(t, len, &k);
                o->acc = (uint8_t)a;
                if(br && k < len && t[k] == '}') k++;
                return k;
            }
            int sym = cmd_lookup(t + i + 1, nl);
            if(sym) {
                o->cp = (uint16_t)sym;
                size_t k = j;
                if(k < len && t[k] == ' ') k++;
                return k;
            }
            o->cp = '\\';
            return i + 1;
        }
        if((c == '^' || c == '_') && !st->group && st->script == 0) {
            uint8_t mode = (c == '^') ? 1 : 2;
            size_t k = i + 1;
            if(k < len && t[k] == '{') {
                st->group = 1;
                st->script = mode;
                i = k + 1;
                continue;
            }
            if(k < len && t[k] != ' ' && t[k] != '\n') {
                o->script = mode;
                o->cp = read_base(t, len, &k);
                return k;
            }
            o->cp = c;
            return i + 1;
        }
        size_t k = i;
        uint32_t cp = next_cp(t, len, &k);
        uint16_t base;
        uint8_t sc;
        if(st->script == 0 && script_map(cp, &base, &sc)) {
            o->cp = base;
            o->script = sc;
            return k;
        }
        o->cp = (uint16_t)cp;
        if(k < len) {
            size_t k2 = k;
            uint32_t m = next_cp(t, len, &k2);
            int a = acc_from_mark(m);
            if(a) {
                o->acc = (uint8_t)a;
                k = k2;
            }
        }
        return k;
    }
    return i;
}

static int tok_script(const Tok* tk) {
    return (tk->script && mini_idx(tk->cp) >= 0) ? tk->script : 0;
}

static int tok_adv(const Tok* tk) {
    return tok_script(tk) ? 4 : 6;
}

static uint8_t tok_need(const Tok* tk) {
    if(!tk->acc || tok_script(tk)) return 0;
    const Acc* a = &ACCS[tk->acc];
    int need = a->h + a->gap - top_row(glyph_for(tk->cp));
    if(need < 0) need = 0;
    if(need > 3) need = 3;
    return (uint8_t)need;
}

static void draw_tok(Canvas* cv, int x, int y, const Tok* tk) {
    int sc = tok_script(tk);
    if(sc) {
        int mi = mini_idx(tk->cp);
        int yo = (sc == 1) ? 0 : 3;
        for(int cx = 0; cx < 3; cx++) {
            for(int ry = 0; ry < 5; ry++) {
                if(MINI[mi][cx] & (1 << ry)) canvas_draw_dot(cv, x + cx, y + yo + ry);
            }
        }
        return;
    }
    const uint8_t* g = glyph_for(tk->cp);
    for(int cx = 0; cx < 5; cx++) {
        for(int cy = 0; cy < 8; cy++) {
            if(g[cx] & (1 << cy)) canvas_draw_dot(cv, x + cx, y + cy);
        }
    }
    if(tk->acc) {
        const Acc* a = &ACCS[tk->acc];
        int yt = y + top_row(g) - a->gap - a->h;
        for(int r = 0; r < a->h; r++) {
            for(int cx = 0; cx < 5; cx++) {
                if(a->rows[r] & (1 << cx)) canvas_draw_dot(cv, x + cx, yt + r);
            }
        }
    }
}

static size_t layout_lines(
    const char* text,
    size_t len,
    uint16_t* ls,
    uint8_t* ex,
    uint8_t* sc,
    size_t max_lines,
    int width) {
    size_t n = 1;
    ls[0] = 0;
    ex[0] = 0;
    sc[0] = 0;
    int x = 0;
    uint8_t cur = 0;
    TS st = {0, 0};
    size_t i = 0;
    while(i < len && n < max_lines - 1) {
        TS before = st;
        size_t start = i;
        Tok tk;
        i = next_tok(text, len, i, &st, &tk);
        if(tk.cp == 0) break;
        if(tk.cp == '\n') {
            ex[n - 1] = cur;
            cur = 0;
            x = 0;
            st.script = 0;
            st.group = 0;
            ls[n] = (uint16_t)i;
            ex[n] = 0;
            sc[n] = 0;
            n++;
            continue;
        }
        int adv = tok_adv(&tk);
        if(x + adv > width && x > 0) {
            ex[n - 1] = cur;
            cur = 0;
            x = 0;
            ls[n] = (uint16_t)start;
            ex[n] = 0;
            sc[n] = (uint8_t)(before.script | (before.group << 2));
            n++;
        }
        x += adv;
        uint8_t nd = tok_need(&tk);
        if(nd > cur) cur = nd;
    }
    ex[n - 1] = cur;
    return n;
}

static void draw_line(Canvas* cv, const char* text, size_t len, size_t p, size_t stop, uint8_t scp, int y) {
    TS st = {(uint8_t)(scp & 3), (uint8_t)((scp >> 2) & 1)};
    int x = 0;
    while(p < len && p < stop) {
        Tok tk;
        p = next_tok(text, len, p, &st, &tk);
        if(tk.cp == 0 || tk.cp == '\n') break;
        draw_tok(cv, x, y, &tk);
        x += tok_adv(&tk);
    }
}

static size_t calc_max_scroll(size_t lines, const uint8_t* ex, int avail) {
    int used = 0;
    size_t k = lines;
    while(k > 0) {
        int h = 8 + ex[k - 1];
        if(used + h > avail) break;
        used += h;
        k--;
    }
    return k;
}

static bool name_is(const char* nm, size_t nl, const char* s) {
    return nl == strlen(s) && memcmp(nm, s, nl) == 0;
}

static size_t grp_end(const char* t, size_t len, size_t k) {
    int d = 0;
    for(size_t j = k; j < len; j++) {
        if(t[j] == '{') d++;
        else if(t[j] == '}') {
            d--;
            if(d == 0) return j;
        }
    }
    return len;
}

static bool grp_needs_paren(const char* t, size_t a, size_t b) {
    for(size_t j = a; j < b; j++) {
        char c = t[j];
        if(c == '+' || c == '-' || c == '=' || c == '*' || c == '/' || c == ',' || c == ' ' ||
           c == '<' || c == '>')
            return true;
        if(c == '\\' && j + 4 < b && memcmp(t + j + 1, "frac", 4) == 0) return true;
    }
    return false;
}

typedef struct {
    uint8_t kind; // 0 plain, 1 numerator, 2 denominator, 3 dropped braces, 4 sqrt
    uint8_t paren;
} BrE;
#define BR_MAX 24

static size_t preprocess(char* t, size_t len) {
    BrE st[BR_MAX];
    int sp = 0;
    int over = 0;
    size_t i = 0, w = 0;
    while(i < len) {
        char c = t[i];
        if(c == '$') {
            i++;
            continue;
        }
        if(c == '}') {
            if(over > 0) {
                over--;
                t[w++] = c;
                i++;
                continue;
            }
            if(sp > 0) {
                BrE e = st[--sp];
                i++;
                if(e.kind == 0) {
                    t[w++] = '}';
                } else if(e.kind == 1) {
                    if(e.paren) t[w++] = ')';
                    t[w++] = '/';
                    size_t k = i;
                    while(k < len && t[k] == ' ') k++;
                    if(k < len && t[k] == '{') {
                        size_t ge = grp_end(t, len, k);
                        bool p = grp_needs_paren(t, k + 1, ge);
                        if(p) t[w++] = '(';
                        if(sp < BR_MAX) {
                            st[sp].kind = 2;
                            st[sp].paren = p;
                            sp++;
                        } else {
                            over++;
                        }
                        i = k + 1;
                    }
                } else if(e.kind == 2 || e.kind == 4) {
                    if(e.paren) t[w++] = ')';
                }
                continue;
            }
            t[w++] = c;
            i++;
            continue;
        }
        if(c == '{') {
            if(sp < BR_MAX) {
                st[sp].kind = 0;
                st[sp].paren = 0;
                sp++;
            } else {
                over++;
            }
            t[w++] = c;
            i++;
            continue;
        }
        if(c == '\\') {
            char n1 = (i + 1 < len) ? t[i + 1] : 0;
            if(n1 == '(' || n1 == ')' || n1 == '[' || n1 == ']') {
                i += 2;
                if(n1 == ')' || n1 == ']') {
                    while(w > 0 && t[w - 1] == ' ') w--;
                    if(n1 == ']' && w > 0 && t[w - 1] == '\n') w--;
                } else {
                    while(i < len && t[i] == ' ') i++;
                    if(n1 == '[' && i < len && t[i] == '\n') i++;
                }
                continue;
            }
            if(n1 == ',' || n1 == ';' || n1 == ':' || n1 == ' ') {
                t[w++] = ' ';
                i += 2;
                continue;
            }
            if(n1 == '!') {
                i += 2;
                continue;
            }
            if(n1 == '{' || n1 == '}' || n1 == '$') {
                t[w++] = c;
                t[w++] = n1;
                i += 2;
                continue;
            }
            size_t j = i + 1;
            while(j < len && is_alpha((uint8_t)t[j]) && j - i - 1 < 12) j++;
            size_t nl = j - i - 1;
            if(nl > 0) {
                const char* nm = t + i + 1;
                if(name_is(nm, nl, "left") || name_is(nm, nl, "right")) {
                    i = j;
                    if(i < len && t[i] == '.') i++;
                    continue;
                }
                if(name_is(nm, nl, "quad") || name_is(nm, nl, "qquad")) {
                    t[w++] = ' ';
                    t[w++] = ' ';
                    i = j;
                    continue;
                }
                if(name_is(nm, nl, "dots") || name_is(nm, nl, "ldots") || name_is(nm, nl, "cdots")) {
                    t[w++] = '.';
                    t[w++] = '.';
                    t[w++] = '.';
                    i = j;
                    continue;
                }
                if(name_is(nm, nl, "text") || name_is(nm, nl, "mathrm") ||
                   name_is(nm, nl, "mathbf") || name_is(nm, nl, "mathit") ||
                   name_is(nm, nl, "textbf") || name_is(nm, nl, "textit") ||
                   name_is(nm, nl, "boldsymbol") || name_is(nm, nl, "operatorname")) {
                    size_t k = j;
                    while(k < len && t[k] == ' ') k++;
                    if(k < len && t[k] == '{' && sp < BR_MAX) {
                        st[sp].kind = 3;
                        st[sp].paren = 0;
                        sp++;
                        i = k + 1;
                    } else {
                        i = j;
                    }
                    continue;
                }
                if(name_is(nm, nl, "frac") || name_is(nm, nl, "dfrac") || name_is(nm, nl, "tfrac")) {
                    size_t k = j;
                    while(k < len && t[k] == ' ') k++;
                    if(k < len && t[k] == '{' && sp < BR_MAX - 1) {
                        size_t ge = grp_end(t, len, k);
                        bool p = grp_needs_paren(t, k + 1, ge);
                        st[sp].kind = 1;
                        st[sp].paren = p;
                        sp++;
                        if(p) t[w++] = '(';
                        i = k + 1;
                        continue;
                    }
                }
                if(name_is(nm, nl, "sqrt")) {
                    if(j < len && t[j] == '{' && sp < BR_MAX) {
                        size_t ge = grp_end(t, len, j);
                        bool p = grp_needs_paren(t, j + 1, ge);
                        st[sp].kind = 4;
                        st[sp].paren = p;
                        sp++;
                        t[w++] = '\\';
                        t[w++] = 's';
                        t[w++] = 'q';
                        t[w++] = 'r';
                        t[w++] = 't';
                        t[w++] = p ? '(' : ' ';
                        i = j + 1;
                        continue;
                    }
                }
            }
        }
        t[w++] = c;
        i++;
    }
    t[w] = 0;
    return w;
}
//CORE_END

typedef enum {
    ScreenList,
    ScreenReader,
} Screen;

typedef struct {
    FuriMutex* mutex;
    FuriMessageQueue* queue;
    ViewPort* vp;
    Storage* storage;
    Screen screen;
    char names[MAX_FILES][NAME_LEN];
    int count;
    int sel;
    int list_top;
    char text[TEXT_MAX];
    size_t len;
    uint16_t line_start[LINE_MAX];
    uint8_t line_extra[LINE_MAX];
    uint8_t line_script[LINE_MAX];
    size_t lines;
    size_t max_scroll;
    int scroll;
    bool confirm;
    const char* status;
} App;

static void scan_files(App* a) {
    a->count = 0;
    storage_simply_mkdir(a->storage, EXT_PATH("apps_data"));
    storage_simply_mkdir(a->storage, SAVE_DIR);
    File* d = storage_file_alloc(a->storage);
    if(storage_dir_open(d, SAVE_DIR)) {
        FileInfo fi;
        char name[NAME_LEN];
        while(a->count < MAX_FILES && storage_dir_read(d, &fi, name, sizeof(name))) {
            if(file_info_is_dir(&fi)) continue;
            size_t n = strlen(name);
            if(n < 5) continue;
            const char* e = name + n - 4;
            if(e[0] == '.' && (e[1] | 32) == 't' && (e[2] | 32) == 'x' && (e[3] | 32) == 't') {
                strcpy(a->names[a->count++], name);
            }
        }
        storage_dir_close(d);
    }
    storage_file_free(d);
    for(int i = 0; i < a->count; i++) {
        for(int j = i + 1; j < a->count; j++) {
            if(strcmp(a->names[i], a->names[j]) > 0) {
                char tmp[NAME_LEN];
                strcpy(tmp, a->names[i]);
                strcpy(a->names[i], a->names[j]);
                strcpy(a->names[j], tmp);
            }
        }
    }
    if(a->sel >= a->count) a->sel = a->count ? a->count - 1 : 0;
}

static void relayout(App* a) {
    a->lines = layout_lines(
        a->text, a->len, a->line_start, a->line_extra, a->line_script, LINE_MAX, WIDTH);
    a->max_scroll = calc_max_scroll(a->lines, a->line_extra, TEXT_H);
    if(a->scroll > (int)a->max_scroll) a->scroll = (int)a->max_scroll;
}

static void load_file(App* a, int idx) {
    a->len = 0;
    a->text[0] = 0;
    a->scroll = 0;
    if(idx >= 0 && idx < a->count) {
        char path[NAME_LEN + 40];
        snprintf(path, sizeof(path), "%s/%s", SAVE_DIR, a->names[idx]);
        File* f = storage_file_alloc(a->storage);
        if(storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
            size_t n = storage_file_read(f, a->text, TEXT_MAX - 1);
            storage_file_close(f);
            size_t w = 0;
            for(size_t i = 0; i < n; i++) {
                if(a->text[i] != '\r') a->text[w++] = a->text[i];
            }
            a->text[w] = 0;
            a->len = preprocess(a->text, w);
        }
        storage_file_free(f);
    }
    relayout(a);
}

static void delete_file(App* a, int idx) {
    if(idx < 0 || idx >= a->count) return;
    char path[NAME_LEN + 40];
    snprintf(path, sizeof(path), "%s/%s", SAVE_DIR, a->names[idx]);
    storage_simply_remove(a->storage, path);
    scan_files(a);
    a->screen = ScreenList;
    a->status = "Deleted";
}

static void draw_plain(Canvas* cv, int x, int y, const char* s, int maxc, bool strip_txt) {
    size_t len = strlen(s);
    if(strip_txt && len > 4) len -= 4;
    size_t i = 0;
    int n = 0;
    while(i < len && n < maxc) {
        uint32_t cp = next_cp(s, len, &i);
        const uint8_t* g = glyph_for(cp);
        for(int cx = 0; cx < 5; cx++) {
            for(int cy = 0; cy < 8; cy++) {
                if(g[cx] & (1 << cy)) canvas_draw_dot(cv, x + n * 6 + cx, y + cy);
            }
        }
        n++;
    }
}

static void draw_header(Canvas* cv, const char* s, bool strip_txt) {
    canvas_set_color(cv, ColorBlack);
    canvas_draw_box(cv, 0, 0, 128, 8);
    canvas_set_color(cv, ColorWhite);
    draw_plain(cv, 1, 0, s, 21, strip_txt);
    canvas_set_color(cv, ColorBlack);
}

static void draw_list(Canvas* cv, App* a) {
    char hdr[40];
    snprintf(hdr, sizeof(hdr), "Files: %d", a->count);
    draw_header(cv, hdr, false);
    if(a->count == 0) {
        canvas_set_font(cv, FontSecondary);
        canvas_draw_str(cv, 2, 22, "No .txt files in:");
        canvas_draw_str(cv, 2, 32, "ext/apps_data/bt_reader");
        canvas_draw_str(cv, 2, 46, "Add files, then reopen");
        if(a->status) canvas_draw_str(cv, 2, 60, a->status);
        return;
    }
    for(int r = 0; r < 7; r++) {
        int idx = a->list_top + r;
        if(idx >= a->count) break;
        int y = 8 + r * 8;
        if(idx == a->sel) {
            canvas_draw_box(cv, 0, y, 126, 8);
            canvas_set_color(cv, ColorWhite);
        }
        draw_plain(cv, 1, y, a->names[idx], 20, true);
        canvas_set_color(cv, ColorBlack);
    }
    if(a->count > 7) {
        int thumb = 56 * 7 / a->count;
        if(thumb < 3) thumb = 3;
        int y = 8 + (56 - thumb) * a->sel / (a->count - 1);
        canvas_draw_box(cv, 126, y, 2, thumb);
    }
}

static void draw_reader(Canvas* cv, App* a) {
    char hdr[NAME_LEN + 16];
    snprintf(hdr, sizeof(hdr), "%d/%d %s", a->sel + 1, a->count, a->names[a->sel]);
    draw_header(cv, hdr, true);
    if(a->len == 0) {
        canvas_set_font(cv, FontSecondary);
        canvas_draw_str(cv, 2, 28, "(empty file)");
        return;
    }
    int y = TEXT_TOP;
    int li = a->scroll;
    int shown = 0;
    while(li < (int)a->lines) {
        int h = 8 + a->line_extra[li];
        if(y + h > 64) break;
        size_t next = (li + 1 < (int)a->lines) ? a->line_start[li + 1] : a->len;
        draw_line(cv, a->text, a->len, a->line_start[li], next, a->line_script[li], y + a->line_extra[li]);
        y += h;
        li++;
        shown++;
    }
    if(a->max_scroll > 0) {
        int thumb = TEXT_H * shown / (int)a->lines;
        if(thumb < 3) thumb = 3;
        int ty = TEXT_TOP + (TEXT_H - thumb) * a->scroll / (int)a->max_scroll;
        canvas_draw_box(cv, 126, ty, 2, thumb);
    }
}

static void draw_cb(Canvas* cv, void* ctx) {
    App* a = ctx;
    furi_mutex_acquire(a->mutex, FuriWaitForever);
    canvas_clear(cv);
    canvas_set_color(cv, ColorBlack);
    if(a->screen == ScreenList) draw_list(cv, a);
    else draw_reader(cv, a);
    if(a->confirm) {
        canvas_set_color(cv, ColorWhite);
        canvas_draw_box(cv, 8, 16, 112, 32);
        canvas_set_color(cv, ColorBlack);
        canvas_draw_frame(cv, 8, 16, 112, 32);
        canvas_set_font(cv, FontSecondary);
        canvas_draw_str(cv, 16, 29, "Delete this file?");
        canvas_draw_str(cv, 16, 42, "Hold OK: yes, other: no");
    }
    furi_mutex_release(a->mutex);
}

static void input_cb(InputEvent* e, void* ctx) {
    furi_message_queue_put((FuriMessageQueue*)ctx, e, 0);
}

static void open_reader(App* a) {
    if(a->count == 0) return;
    a->screen = ScreenReader;
    load_file(a, a->sel);
}

int32_t bt_reader_app(void* p) {
    UNUSED(p);
    App* a = malloc(sizeof(App));
    memset(a, 0, sizeof(App));
    a->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    a->queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    a->storage = furi_record_open(RECORD_STORAGE);
    a->lines = 1;
    scan_files(a);

    a->vp = view_port_alloc();
    view_port_draw_callback_set(a->vp, draw_cb, a);
    view_port_input_callback_set(a->vp, input_cb, a->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, a->vp, GuiLayerFullscreen);

    InputEvent ev;
    bool run = true;
    while(run) {
        if(furi_message_queue_get(a->queue, &ev, FuriWaitForever) != FuriStatusOk) continue;
        furi_mutex_acquire(a->mutex, FuriWaitForever);
        bool is_short = (ev.type == InputTypeShort || ev.type == InputTypeRepeat);
        bool is_long = (ev.type == InputTypeLong);
        if(a->confirm) {
            if(is_long && ev.key == InputKeyOk) delete_file(a, a->sel);
            if(is_short || is_long) a->confirm = false;
        } else if(a->screen == ScreenList) {
            if(is_short) {
                if(ev.key == InputKeyUp && a->sel > 0) a->sel--;
                if(ev.key == InputKeyDown && a->sel < a->count - 1) a->sel++;
                if(a->sel < a->list_top) a->list_top = a->sel;
                if(a->sel > a->list_top + 6) a->list_top = a->sel - 6;
                if(ev.key == InputKeyOk && ev.type == InputTypeShort) open_reader(a);
                if(ev.key == InputKeyBack) run = false;
            } else if(is_long && ev.key == InputKeyOk && a->count > 0) {
                a->confirm = true;
            }
        } else {
            if(is_short) {
                if(ev.key == InputKeyUp && a->scroll > 0) a->scroll--;
                if(ev.key == InputKeyDown && a->scroll < (int)a->max_scroll) a->scroll++;
                if(ev.key == InputKeyLeft && a->sel > 0) {
                    a->sel--;
                    load_file(a, a->sel);
                }
                if(ev.key == InputKeyRight && a->sel < a->count - 1) {
                    a->sel++;
                    load_file(a, a->sel);
                }
                if(ev.type == InputTypeShort && (ev.key == InputKeyOk || ev.key == InputKeyBack)) {
                    scan_files(a);
                    a->screen = ScreenList;
                    if(a->sel < a->list_top) a->list_top = a->sel;
                    if(a->sel > a->list_top + 6) a->list_top = a->sel - 6;
                }
            } else if(is_long && ev.key == InputKeyOk) {
                a->confirm = true;
            }
        }
        furi_mutex_release(a->mutex);
        view_port_update(a->vp);
    }

    gui_remove_view_port(gui, a->vp);
    furi_record_close(RECORD_GUI);
    view_port_free(a->vp);
    furi_record_close(RECORD_STORAGE);
    furi_message_queue_free(a->queue);
    furi_mutex_free(a->mutex);
    free(a);
    return 0;
}
