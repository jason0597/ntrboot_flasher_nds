#pragma once

#include <nds.h>
#define vBufferM ((u16 *)0x06000000)
#define vBufferS ((u16 *)0x06200000)

#define TOP_SCREEN vBufferM
#define BOTTOM_SCREEN vBufferS

#define BG_15BITCOLOR    (1<<7)
#define BG_CBB1          (1<<2)

#define SCREENWIDTH  256
#define SCREENHEIGHT 192

#define RGB(x, y, z) RGB8(x, y, z)

#define COLOR_BLACK         RGB(0x00, 0x00, 0x00)
#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_RED           RGB(0xFF, 0x00, 0x00)
#define COLOR_GREEN         RGB(0x00, 0xFF, 0x00)
#define COLOR_BLUE          RGB(0x00, 0x00, 0xFF)
#define COLOR_CYAN          RGB(0x00, 0xFF, 0xFF)
#define COLOR_MAGENTA       RGB(0xFF, 0x00, 0xFF)
#define COLOR_YELLOW        RGB(0xFF, 0xFF, 0x00)
#define COLOR_GREY          RGB(0x77, 0x77, 0x77)
#define COLOR_TRANSPARENT   RGB(0xFF, 0x00, 0xEF) // otherwise known as 'super fuchsia'

#define COLOR_GREYBLUE      RGB(0xA0, 0xA0, 0xFF)
#define COLOR_GREYGREEN     RGB(0xA0, 0xFF, 0xA0)
#define COLOR_GREYRED       RGB(0xFF, 0xA0, 0xA0)
#define COLOR_GREYCYAN      RGB(0xA0, 0xFF, 0xFF)
#define COLOR_TINTEDRED     RGB(0xFF, 0x60, 0x60)
#define COLOR_LIGHTGREY     RGB(0xA0, 0xA0, 0xA0)

#define COLOR_ASK           COLOR_GREYGREEN
#define COLOR_SELECT        COLOR_LIGHTGREY
#define COLOR_ACCENT        COLOR_GREEN

#define STD_COLOR_BG   COLOR_BLACK
#define STD_COLOR_FONT COLOR_WHITE

void InitializeScreens(void);
void setPixel(u16 *screen, int r, int c, u16 color);
void ClearScreen(u16 *screen, u16 color);
void DrawRectangle(u16 *screen, int x, int y, int width, int height, u16 color);

void DrawCharacter(u16 *screen, int character, int x, int y, u16 color);
void DrawString(u16 *screen, int x, int y, u16 color, const char *str);
void DrawStringF(u16 *screen, int x, int y, u16 color, const char *format, ...);

void DrawHex(u16 *screen, unsigned int hex, int x, int y, u16 color);
void DrawHexWithName(u16 *screen, unsigned int hex, int x, int y, u16 color);

u32 GetDrawStringHeight(const char *str);
u32 GetDrawStringWidth(const char *str);

void ShowString(u16 *screen, const char* format, ...);

void DrawHeader(u16* screen, const char *str, int offset);
void ShowProgress(u16 *screen, uint32_t current, uint32_t total, const char* status);
