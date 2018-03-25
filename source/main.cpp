#include <nds.h>
#include "device.h"
#include "ui.h"
#include "menu.h"

using namespace flashcart_core;
using namespace ncgc;

int main(void)
{
	sysSetBusOwners(true, true); //Give ARM9 access to the cart
	InitializeScreens(); 

	bool isDevMode = false;																					//Hold START/SELECT/X on boot to enable dev flashing
	scanKeys();																				                //This is so that noobs don't do a dev flash by accident
	if (keysHeld() & KEY_START && keysHeld() & KEY_SELECT && keysHeld() & KEY_X) { isDevMode = true; }		//And it's not hard too get a dev flash if you wanted to

	Flashcart *cart = nullptr; //We define our main cart variable right here, and we will pass it along from function to function until the very end

	print_boot_msg();

	menu_lvl1(cart, isDevMode); //menu_lvl1() will later call menu_lvl2()

	return 0;
}