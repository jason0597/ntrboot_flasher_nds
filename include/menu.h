#include "device.h"

void print_boot_msg(void);
void WaitPress(uint32_t KEY);
void menu_lvl1(flashcart_core::Flashcart* cart, bool isDevMode);
void menu_lvl2(flashcart_core::Flashcart* cart, bool isDevMode);
bool d0k3_buttoncombo(int cur_c, int cur_r);
void d0k3_buttoncombo_print_chars(int collumn, int row, uint16_t color, char character);
