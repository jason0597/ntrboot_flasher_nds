#include <nds.h>
#include "ui.h"
#include "font.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void InitializeScreens(void) {
	REG_DISPCNT = MODE_3_2D | DISPLAY_BG3_ACTIVE;
    REG_DISPCNT_SUB = MODE_3_2D | DISPLAY_BG3_ACTIVE;

    // Set bg3 on both displays large enough to fill the screen, use 15 bit
    // RGB color values, and look for bitmap data at character base block 1.
    REG_BG3CNT =  BgSize_R_256x256| BG_15BITCOLOR | BG_CBB1;
    REG_BG3CNT_SUB = BgSize_R_256x256 | BG_15BITCOLOR | BG_CBB1;

    // Don't scale bg3 (set its affine transformation matrix to [[1,0],[0,1]])
    REG_BG3PA = 1 << 8;
    REG_BG3PD = 1 << 8;

    REG_BG3PA_SUB = 1 << 8;
    REG_BG3PD_SUB= 1 << 8;	
}

void setPixel(u16 *screen, int row, int col, u16 color) {
    screen[row * SCREENWIDTH + col] = color | BIT(15);
}

void ClearScreen(u16 *screen, u16 color) {
	int width = SCREENHEIGHT * SCREENWIDTH;
	for(int i = 0; i < width; i++) {
		screen[i] = color | BIT(15);
	}
}

void DrawRectangle(u16 *screen, int x, int y, int width, int height, u16 color) {
    for (int c = x; c < x+width; c++) {
        for (int r = y; r < y+height; r++) {
            setPixel(screen, r, c, color);
        }
    }
}

void DrawCharacter(u16 *screen, int character, int x, int y, u16 color) {
    for(int yy = 0; yy < FONT_HEIGHT; yy++) {
        uint8_t charPos = font[character * FONT_HEIGHT + yy];
        for (int xx = (8 - FONT_WIDTH); xx <= 7; xx++) {
            if ((charPos >> xx) & 1) {
                setPixel(screen, y + yy, x + FONT_WIDTH - xx, color);
            }
        }
    }
}

void DrawString(u16* screen, int x, int y, u16 color, const char *str) 
{
    const int startx = x;
    const size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) 
    {
        if (str[i] == '\n') 
        {
            x = startx;
            y += FONT_HEIGHT;
            continue;
        }
        if ((y + FONT_HEIGHT) > SCREENHEIGHT) 
        {
            break;
        }

        if ((x + FONT_WIDTH) > SCREENWIDTH) 
        {
            x = startx;
            y += FONT_HEIGHT;
        }
        DrawCharacter(screen, str[i], x, y, color);
        x += FONT_WIDTH;
    }
}

void DrawHeader(u16* screen, const char *str, int offset) 
{
    ClearScreen(screen, COLOR_BLACK);
    DrawRectangle(screen, 0, 0, SCREENWIDTH, FONT_HEIGHT, COLOR_BLUE);
    DrawString(screen, offset, 0, COLOR_WHITE, str);
}

void DrawStringF(u16 *screen, int x, int y, u16 color, const char *format, ...) 
{
    char str[256];
    char *p = str;
    va_list va;

    va_start(va, format);
    int w = vsnprintf(str, sizeof(str), format, va);
    if (w < 0) {
        // printf failed
        return;
    } 
    else if ((unsigned int)w > sizeof(str) - 1) {
        // cry now
        // allocate a buffer big enough
        char *m = (char *) malloc(w + 1);
        if (m) {
            vsnprintf(m, w + 1, format, va);
            p = m;
        } // if malloc fails, we just write the truncated string i guess
    }
    va_end(va);

    DrawString(screen, x, y, color, p);
    if (p != str && p) {
        free(p);
    }
}

//------------------------------------------------------------------------------------------
//All functions below this line are unused functions
//------------------------------------------------------------------------------------------

void DrawHex(u16 *screen, unsigned int hex, int x, int y, u16 color) {
    for(int i=0; i<8; i++) {
        int character = '-';
        int nibble = (hex >> ((7-i)*4))&0xF;
        if(nibble > 9) character = 'A' + nibble-10;
        else character = '0' + nibble;

        DrawCharacter(screen, character, x+(i*8), y, color);
    }
}

void DrawHexWithName(u16 *screen, const char *str, unsigned int hex, int x, int y, u16 color) {
    DrawString(screen, x, y, color, str);
    DrawHex(screen, hex,x + strlen(str) * FONT_WIDTH, y, color);
}

uint32_t GetDrawStringHeight(const char* str) {
    uint32_t height = FONT_HEIGHT;
    for (char* lf = strchr(str, '\n'); (lf != NULL); lf = strchr(lf + 1, '\n')) {
        height += 10;
    }
    return height;
}

uint32_t GetDrawStringWidth(const char* str) {
    uint32_t width = 0;
    char* old_lf = (char*) str;
    char* str_end = (char*) str + strnlen(str, 256);
    for (char* lf = strchr(str, '\n'); lf != NULL; lf = strchr(lf + 1, '\n')) {
        if ((uint32_t) (lf - old_lf) > width) width = lf - old_lf;
        old_lf = lf;
    }
    if ((uint32_t) (str_end - old_lf) > width) {
        width = str_end - old_lf;
    }
    width *= FONT_WIDTH;
    return width;
}

void ShowString(u16 *screen, const char *format, ...) {
    uint32_t str_width, str_height;
    uint32_t x, y;

    char str[256] = { 0 };
    va_list va;
    va_start(va, format);
    vsnprintf(str, 256, format, va);
    va_end(va);

    str_width = GetDrawStringWidth(str);
    str_height = GetDrawStringHeight(str);
    x = (str_width >= SCREENWIDTH) ? 0 : (SCREENWIDTH - str_width) / 2;
    y = (str_height >= SCREENHEIGHT) ? 0 : (SCREENHEIGHT - str_height) / 2;

    ClearScreen(screen, STD_COLOR_BG);
    DrawString(screen, x, y, STD_COLOR_FONT, str);
}
