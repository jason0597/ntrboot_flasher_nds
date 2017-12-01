#include <nds.h>
#include "nds_platform.h"
#include <nds/arm9/dldi.h>
#include <ncgcpp/ntrcard.h>
#include "device.h"
#include "blowfish_keys.h"
#include <fstream>

namespace {
	const std::uint32_t dummy_blowfish_key[0x412] = {0};
}

namespace flashcart_core {
	namespace platform {
		void showProgress(std::uint32_t current, std::uint32_t total, const char *status) {
			static_cast<void>(current); static_cast<void>(total); static_cast<void>(status);
		}

		int logMessage(log_priority priority, const char *fmt, ...) {
			static_cast<void>(priority); static_cast<void>(fmt);
			return -1;
		}

		auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048] 
		{
			return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
			//For now, this is hardcoded to retail B9S blowfish key. TODO: Make it so that it returns the correct key depending on isDevMode
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
	std::streampos filesize = FileIn.tellg();
	char *File = new char[filesize];
	FileIn.seekg(0);
	FileIn.read(File, filesize);

	//Convert the char array into a u8 array, because that's what injectNtrBoot likes
	u8 *FIRM = new u8[filesize];
	for (int i = 0; i <= filesize; i++) {
		FIRM[i] = File[i];
	}

	if (!cart->injectNtrBoot((isDevMode) ? blowfish_dev_bin : blowfish_retail_bin, FIRM, filesize)) {
		FileIn.close();
		//Same return concept as before, but in this case, a 2 means that flashing failed
		return 2;
	}
	FileIn.close();
	return 0;
}

int DumpFlash(flashcart_core::Flashcart* cart, bool isDevMode) 
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

	char *Array_to_write = new char[Flash_size];
	for (u32 i = 0; i <= Flash_size; i++) {
		Array_to_write[i] = Flashrom[i];
	}

	FileOut.write(Array_to_write, Flash_size);
	FileOut.close();

	return 0;
}
