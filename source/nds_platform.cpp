#include <nds.h>
#include <fstream>
#include "nds_platform.h"
#include <nds/arm9/dldi.h>
#include <ncgcpp/ntrcard.h>
#include "device.h"
#include "blowfish_keys.h"
#include "font.h"
#include "ui.h"

namespace {
	const std::uint32_t dummy_blowfish_key[0x412] = {0};
}

int progressCount = 0;

namespace flashcart_core {
	namespace platform {
		void showProgress(std::uint32_t current, std::uint32_t total, const char *status) {
            if (progressCount < 1000) {
                progressCount++;
                return;
            } else {
                progressCount = 0;
            }
            swiWaitForVBlank();
            ShowProgress(BOTTOM_SCREEN, current, total, status);
            //ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
            //DrawString(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT*4, COLOR_WHITE, status);
            //DrawRectangle(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT*6, SCREENWIDTH-FONT_HEIGHT * ((float)current/(float)total), FONT_HEIGHT, COLOR_GREEN);
		}

		int logMessage(log_priority priority, const char *fmt, ...) {
			static_cast<void>(priority); static_cast<void>(fmt);
			return -1;
		}

		auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048] 
		{
            switch (key) {
                default:
                case BlowfishKey::NTR:
                    // return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_ntr_bin)); we don't have this?
                case BlowfishKey::B9Retail:
                    return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
                case BlowfishKey::B9Dev:
                    return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_dev_bin));
            }
		}
	}
}

std::uint8_t buffer[0x4000];

int InjectFIRM(flashcart_core::Flashcart* cart, bool isDevMode) 
{
	//Open the file
	std::ifstream FileIn((isDevMode) ? "fat:/ntrboot/boot9strap_ntr_dev.firm" : "fat:/ntrboot/boot9strap_ntr.firm", std::ios::in | std::ios::binary | std::ios::ate);
	if (!FileIn.is_open()) {
		//Return a 1 to menu_lvl2() if we get a failed to open file. We can then use this 1 to determine the error type
		return 1;
	}
	u32 filesize = FileIn.tellg();
	char *FIRM = new char[filesize];
	FileIn.seekg(0);
	FileIn.read(FIRM, filesize);

	if (!cart->injectNtrBoot((isDevMode) ? blowfish_dev_bin : blowfish_retail_bin, reinterpret_cast<u8*>(FIRM), filesize)) {
		FileIn.close();
		//Same return concept as before, but in this case, a 2 means that flashing failed
		return 2;
	}

	delete[] FIRM;
	FileIn.close();

	return 0;
}

int DumpFlash(flashcart_core::Flashcart* cart) 
{
	u32 Flash_size = cart->getMaxLength();
	u8 *Flashrom = new u8[Flash_size];
	if (!cart->readFlash(0, Flash_size, Flashrom)) {
		return 2;
	}
	std::ofstream FileOut("fat:/ntrboot/backup.bin", std::ios::out | std::ios::binary | std::ios::trunc);
	if (!FileOut.is_open()) {
		return 1;
	}

	FileOut.write(reinterpret_cast<const char *>(Flashrom), Flash_size); //fucking write() and its const char* requirements
	delete[] Flashrom;
	FileOut.close();

	return 0;
}
