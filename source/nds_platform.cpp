#include "nds_platform.h"
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <fat.h>
#include "device.h"
#include "blowfish_keys.h"
#define FONT_WIDTH  6
#define FONT_HEIGHT 10
#include "ui.h"
#include <cstdio>
#include <cstdarg>
#include <sys/stat.h>

int progressCount = 0;

namespace flashcart_core {
	namespace platform {
		void showProgress(std::uint32_t current, std::uint32_t total, const char *status) {
            if (progressCount < 1000) {
                progressCount++;
                return;
            } 
			else {
                progressCount = 0;
            }
            ShowProgress(BOTTOM_SCREEN, current, total, status);
		}

		int logMessage(log_priority priority, const char *fmt, ...) 
		{
			if (priority < global_loglevel) return 0;

			static bool first_open = true;
			if (!fatInitDefault()) { return -1; }

			mkdir("fat:/ntrboot", 0700); //If the directory exists, this line isn't going to crash the program or anything like that

			// Overwrite if this is our first time opening the file.
			FILE *logfile = fopen("fat:/ntrboot/ntrboot.log", first_open ? "w" : "a");
			if (!logfile) { return -1; }
			first_open = false;

			va_list args;
			va_start(args, fmt);

			const char *priority_str;
			if (priority == 0) { priority_str = "DEBUG"; }
			if (priority == 1) { priority_str = "INFO"; }
			if (priority == 2) { priority_str = "NOTICE"; }
			if (priority == 3) { priority_str = "WARN"; }
			if (priority == 4) { priority_str = "ERROR"; }
			if (priority >= 5) { priority_str = "?!#$"; }

			char string_to_write[100]; //just do 100, should be enough for any kind of log message we get...
			sprintf(string_to_write, "[%s]: %s\n", priority_str, fmt);

			int result = vfprintf(logfile, string_to_write, args);
			fclose(logfile);
			fatUnmount("fat:/");
			va_end(args);

			return result;
		}

		auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048] 
		{
            switch (key) {
                default:
                case BlowfishKey::NTR:
                    return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_ntr_bin)); 
                case BlowfishKey::B9Retail:
                    return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
                case BlowfishKey::B9Dev:
                    return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_dev_bin));
            }
		}
	}
}

int InjectFIRM(flashcart_core::Flashcart* cart, bool isDevMode) 
{
	if (!fatInitDefault()) 
	{
        return 1; //Fat mount failed
    }

	//Open the file
	FILE *FileIn = fopen((isDevMode) ? "fat:/ntrboot/boot9strap_ntr_dev.firm" : "fat:/ntrboot/boot9strap_ntr.firm", "rb");
    if (!FileIn) 
	{
		fclose(FileIn);
        return 2; //File opening failed
    }

	fseek(FileIn, 0, SEEK_END); //Go to the end of the file
	u32 filesize = ftell(FileIn); //Now that we are at the end of the file, we can use the end as the number of bytes
	u8 *FIRM = new u8[filesize]; //Make our new array to store the FIRM from the SD card
	fseek(FileIn, 0, SEEK_SET); //Go to the start of the file because we are going to read it from start to finish in the next line
	
	if (fread(FIRM, 1, filesize, FileIn) != filesize) {
		delete[] FIRM;
		fclose(FileIn);
		return 3; //File reading failed
	}

	fatUnmount("fat:/"); //We must unmount *before* calling any flashcart_core functions

	if (!cart->injectNtrBoot((isDevMode) ? blowfish_dev_bin : blowfish_retail_bin, FIRM, filesize)) {
		delete[] FIRM;
		fclose(FileIn);
		return 4; //FIRM injection failed
	}

	delete[] FIRM;
	fclose(FileIn);

	return 0;
}

int DumpFlash(flashcart_core::Flashcart* cart) 
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size
	u8 *Flashrom = new u8[Flash_size]; //Allocate a new array to store the flashrom we are about to retrieve from the flashcart

	if (!cart->readFlash(0, Flash_size, Flashrom)) {
		delete[] Flashrom;
		return 4; //Flash reading failed
	}

	if (!fatInitDefault()) 
	{
		delete[] Flashrom;
        return 1; //Fat init failed
    }

	mkdir("fat:/ntrboot", 0700); //If the directory exists, this line isn't going to crash the program or anything like that

	FILE *FileOut = fopen("fat:/ntrboot/backup.bin", "wb");
	if (!FileOut) {
		delete[] Flashrom;
		fclose(FileOut);
		fatUnmount("fat:/");
		return 2; //File opening failed
	}

	if (fwrite (Flashrom, 1, Flash_size, FileOut) != Flash_size) {
		delete[] Flashrom;
		fclose(FileOut);
		fatUnmount("fat:/");
		return 3; //File writing failed
	}
	
	delete[] Flashrom;
	fclose(FileOut);
	fatUnmount("fat:/");

	return 0;
}
