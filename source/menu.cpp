#include <nds.h>
#include "ui.h"
#include "font.h"
#include "menu.h"
#include "nds_platform.h"
#include "../flashcart_core/device.h"
#include <ncgcpp/ntrcard.h>
#include <ctime>
#include <nds/arm9/dldi.h>

using namespace flashcart_core;
using namespace ncgc;

void print_boot_msg(void) 
{
    ClearScreen(TOP_SCREEN, COLOR_RED);
    DrawString(TOP_SCREEN, 0, 0, COLOR_WHITE, "\n WARNING: READ THIS BEFORE CONTINUING\n ------------------------------------\n\n If you don't know what you're doing: STOP\n Open your browser to https://3ds.guide/\n and follow the steps provided there.\n\n This software writes directly to your\n flashcart. It's possible you may BRICK \n your flashcart.\n\n ALWAYS KEEP A BACKUP\n\n <A> CONTINUE  <B> POWEROFF");

    while (true) 
    { 
        scanKeys();
        if (keysDown() & KEY_B) 
        {
            exit(0);
        }
        if (keysDown() & KEY_A) 
        {
            break;
        }
    }
}

void sd_mount_fail(void) 
{
    DrawString(TOP_SCREEN, 10*FONT_WIDTH, 10*FONT_HEIGHT, COLOR_WHITE, "SD mounting failed!\nPress <B> to exit");
    while (true) { scanKeys(); if (keysDown() & KEY_B) { exit(0); } }
}

void cart_failed(void) 
{
    DrawString(TOP_SCREEN, 10*FONT_WIDTH, 10*FONT_HEIGHT, COLOR_WHITE, "Cart failed!\nPress <B> to exit");
    while (true) { scanKeys(); if (keysDown() & KEY_B) { exit(0); } }
}

void menu_lvl1(Flashcart* cart, bool isDevMode) 
{
    u32 menu_sel = 0;
    NTRCard card(nullptr); 
    DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16*FONT_WIDTH)) / 2)); 
    DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREENWIDTH - (14*FONT_WIDTH)) / 2));
    DrawStringF(BOTTOM_SCREEN, FONT_WIDTH , FONT_HEIGHT*2, COLOR_WHITE, "%s\n%s", flashcart_list->at(0)->getAuthor(), flashcart_list->at(0)->getDescription()); 

    while (true) //This will be our MAIN loop
    { 
        bool reprintFlag = false;
        for (u32 i = 0; i < flashcart_list->size(); i++) 
        {
            cart = flashcart_list->at(i);
            DrawString(TOP_SCREEN, FONT_WIDTH, ((i+2) * FONT_HEIGHT), (i == menu_sel) ? COLOR_RED : COLOR_WHITE, cart->getName());
        }
        scanKeys();
        if (keysDown() & KEY_DOWN && menu_sel < (flashcart_list->size() - 1)) 
        {
            menu_sel++;
            reprintFlag = true;
        }
        if (keysDown() & KEY_UP   && menu_sel > 0) 
        {
            menu_sel--;
            reprintFlag = true;
        }
        if (keysDown() & KEY_A) 
        {
            cart = flashcart_list->at(menu_sel); //Set the cart equal to whatever we had selected from before
            card.state(NTRState::Key2); 

            if (!cart->initialize(&card)) //If cart initialization fails, do all this and then break to main menu
            { 
                DrawString(TOP_SCREEN, FONT_WIDTH, 7*FONT_HEIGHT, COLOR_RED, "Flashcart setup failed!\nPress <B>");
                while (true) { scanKeys(); if (keysDown() & KEY_B) { DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16*FONT_WIDTH)) / 2)); break; } }
            }
            else 
            {
                menu_lvl2(cart, isDevMode); //There is a while loop over at menu_lvl2(), the statements underneath won't get executed immediately
                DrawHeader(TOP_SCREEN, "Select flashcart", ((SCREENWIDTH - (16*FONT_WIDTH)) / 2)); 
                reprintFlag = true;
            }
        }
        
        if (reprintFlag) 
        {
            cart = flashcart_list->at(menu_sel);
            DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREENWIDTH - (14*FONT_WIDTH)) / 2)); 
            DrawStringF(BOTTOM_SCREEN, FONT_WIDTH , FONT_HEIGHT*2, COLOR_WHITE, "%s\n%s", cart->getAuthor(), cart->getDescription());
        }
    }
}

void menu_lvl2(Flashcart* cart, bool isDevMode)
{
    DrawHeader(TOP_SCREEN, cart->getName(), ((SCREENWIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2)); 
    int menu_sel = 0;
    bool breakCondition = false;

    while (!breakCondition) //We are using (!breakCondition) and not (true) because we need to be able to break out of the loop inside case statements (where `break;` breaks out of the case, not the loop)
    { 
        scanKeys();
        DrawString(TOP_SCREEN, FONT_WIDTH, (2 * FONT_HEIGHT), (menu_sel == 0) ? COLOR_RED : COLOR_WHITE, "Inject FIRM");	//0
        DrawString(TOP_SCREEN, FONT_WIDTH, (3 * FONT_HEIGHT), (menu_sel == 1) ? COLOR_RED : COLOR_WHITE, "Dump flash");		//1
        if (keysDown() & KEY_DOWN && menu_sel < 1) 
        {
            menu_sel++;
        }
        if (keysDown() & KEY_UP   && menu_sel > 0) 
        {
            menu_sel--;
        }
        if (keysDown() & KEY_B) 
        {
            breakCondition = true;
        }
        int ntrboot_return = 0;
        if (keysDown() & KEY_A) 
        {
            switch (menu_sel) {
                case 0:
                    DrawString(TOP_SCREEN, FONT_WIDTH, (8*FONT_HEIGHT), COLOR_WHITE, " About to inject FIRM!\n Enter button combination to proceed:");
                    if (d0k3_buttoncombo(10, 12)) 
                    {
						ntrboot_return = InjectFIRM(cart, isDevMode);
                        if (ntrboot_return == 1) {
                            DrawString(TOP_SCREEN, (FONT_WIDTH), (15*FONT_HEIGHT), COLOR_WHITE, " Failed to open FIRM file!\n Press <B> to return to main menu");
                            while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }
                        }
                        if (ntrboot_return == 2) {
                            DrawString(TOP_SCREEN, (FONT_WIDTH), (15*FONT_HEIGHT), COLOR_WHITE, " Failed to inject FIRM to flashcart!\n Press <B> to return to main menu");
                            while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }
                        }
                    }
                    else 
                    {
                        DrawString(TOP_SCREEN, (2*FONT_WIDTH), (14*FONT_HEIGHT), COLOR_WHITE, "Stopped by user...\nPress <B> to return to main menu");
                        while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }    
                    }
                    DrawString(TOP_SCREEN, (2*FONT_WIDTH), (18*FONT_HEIGHT), COLOR_WHITE, "FIRM flash sucessful!\nPress <B> to return to main menu");
                    while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }  
                    breakCondition = true;
                    break;
                
                case 1:
                    DrawString(TOP_SCREEN, FONT_WIDTH, (8*FONT_HEIGHT), COLOR_WHITE, " About to dump flash!\n Enter button combination to proceed:");
                    if (d0k3_buttoncombo(10, 12)) 
                    {
						ntrboot_return = DumpFlash(cart);
                        if (ntrboot_return == 1) {
                            DrawString(TOP_SCREEN, (FONT_WIDTH), (15*FONT_HEIGHT), COLOR_WHITE, " Failed to open (or create) backup.bin file!\n Press <B> to return to main menu");
                            while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }
                        }
                        if (ntrboot_return == 2) {
                            DrawString(TOP_SCREEN, (FONT_WIDTH), (15*FONT_HEIGHT), COLOR_WHITE, " Failed to dump flash!\n Press <B> to return to main menu");
                            while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }
                        }
                    }
                    else 
                    {
                        DrawString(TOP_SCREEN, (2*FONT_WIDTH), (14*FONT_HEIGHT), COLOR_WHITE, "Stopped by user...\nPress <B> to return to main menu");
                        while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }
                    }
                    DrawString(TOP_SCREEN, (2*FONT_WIDTH), (18*FONT_HEIGHT), COLOR_WHITE, "Flash dump sucessful!\nPress <B> to return to main menu");
                    while (true) { scanKeys(); if (keysDown() & KEY_B) { break; } }  
                    breakCondition = true;
                    break;
            }
        }
    }
}

//Will print out a gm9/sb9si like "input button combo to continue" prompt, also does the button checking for it
//Requires #include <ctime> for the rand() seed
bool d0k3_buttoncombo(int cur_c, int cur_r) 
{
    cur_c *= FONT_WIDTH; cur_r *= FONT_HEIGHT;
    srand(time(NULL));
    int num_rancombo[4] = { (rand() % 4), (rand() % 4), (rand() % 4), (rand() % 4) };
    char print_rancombo[4] = { ' ', ' ', ' ', ' ' };
    for (int i = 0; i < 4; i++) {
        if (num_rancombo[i] == 0) { print_rancombo[i] = '\x1B'; } //Left
        if (num_rancombo[i] == 1) { print_rancombo[i] = '\x18'; } //Up
        if (num_rancombo[i] == 2) { print_rancombo[i] = '\x1A'; } //Right
        if (num_rancombo[i] == 3) { print_rancombo[i] = '\x19'; } //Down
    } 
    u32 check_rancombo[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; i++) {
        if (num_rancombo[i] == 0) { check_rancombo[i] = KEY_LEFT; }
        if (num_rancombo[i] == 1) { check_rancombo[i] = KEY_UP; }
        if (num_rancombo[i] == 2) { check_rancombo[i] = KEY_RIGHT; }
        if (num_rancombo[i] == 3) { check_rancombo[i] = KEY_DOWN; }
    }
    int depth = 0; //How far we have gotten into the button combination, like, how many buttons have been pressed so far, how *deep* we are

    while (true) {
        int temp_c = cur_c;
        u16 cur_color = COLOR_WHITE;
        switch (depth) {
            case 0:
                for (int i = 0; i < 4; i++) {
                    d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
                    temp_c += 4*FONT_WIDTH;
                }
                break;
            
            case 1:
                cur_color = COLOR_GREEN;
                for (int i = 0; i < 4; i++) {
                    d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
                    temp_c += 4*FONT_WIDTH;
                    if (i == 0) { cur_color = COLOR_WHITE; }
                }
                break;

            case 2:
                cur_color = COLOR_GREEN;
                for (int i = 0; i < 4; i++) {
                    d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
                    temp_c += 4*FONT_WIDTH; //3 for our printout ('<', 'arrow', '>'), and one for the space that follows it
                    if (i == 1) { cur_color = COLOR_WHITE; }
                }
                break;

            case 3:
                cur_color = COLOR_GREEN;
                for (int i = 0; i < 4; i++) {
                    d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
                    temp_c += 4*FONT_WIDTH;
                    if (i == 2) { cur_color = COLOR_WHITE; }
                }
                break;
        }
        
        scanKeys();
        if (keysDown()) {
            if (keysDown() & check_rancombo[depth]) {
                depth++;
            }   
            else if (keysDown() & KEY_B) {
                return false;
            }
            else {
                depth = 0;
            }
        }
        
        if (depth == 4) {
            int temp_c = cur_c;
            for (int i = 0; i < 4; i++) {
                d0k3_buttoncombo_print_chars(temp_c, cur_r, COLOR_GREEN, print_rancombo[i]);
                temp_c += 4*FONT_WIDTH;
            }
            return true;
        }
    }
}

void d0k3_buttoncombo_print_chars(int collumn, int row, u16 color, char character) 
{
    DrawCharacter(TOP_SCREEN, '<', collumn, row, color);
    collumn += FONT_WIDTH; 
    DrawCharacter(TOP_SCREEN, character, collumn, row, color);
    collumn += FONT_WIDTH;
    DrawCharacter(TOP_SCREEN, '>', collumn, row, color);
}