#include <nds.h>
#include "device.h"

using namespace flashcart_core;
using namespace ncgc;

void print_boot_msg(void);
void WaitPress(u32 KEY);
void menu_lvl1(Flashcart* cart, bool isDevMode);
void menu_lvl2(Flashcart* cart, bool isDevMode);
bool d0k3_buttoncombo(int cur_c, int cur_r);
void d0k3_buttoncombo_print_chars(int collumn, int row, u16 color, char character);
