#include "nds_platform.h"
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <fat.h>
#include "device.h"
#include "binaries.h"
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
			if (priority < global_loglevel) { return 0; }

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
			//I use a bunch of if statements here because the array that has strings over at ntrboot_flasher's `platform.cpp` is not available here
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

bool file_exists(const char* filename) {
    if (FILE* file = fopen(filename, "r")) {
        fclose(file);
        return true;
    } else {
		return false;
	}
}

static char* calculate_backup_path(const char *cart_name) {
    int path_len = snprintf(NULL, 0, "fat:/ntrboot/%s-backup.bin", cart_name) + 1;
    char *path = (char *)malloc(path_len);
    snprintf(path, path_len, "fat:/ntrboot/%s-backup.bin", cart_name);
    return path;
}

return_codes_t InjectFIRM(flashcart_core::Flashcart* cart, bool isDevMode)
{
	if (!fatInitDefault()) { return FAT_MOUNT_FAILED; } //Fat mount failed
	
	char* backup_path = calculate_backup_path(cart->getShortName());

	if (!file_exists(backup_path)) {
		fatUnmount("fat:/");
		free(backup_path);
		return NO_BACKUP_FOUND;
	} free(backup_path);

	u32 filesize; //Used later on

	FILE *FileIn = fopen("fat:/ntrboot/flashcart_payload.firm", "rb");
	if (!FileIn) { 
		fatUnmount("fat:/");
		return FILE_OPEN_FAILED; 
	}
	fseek(FileIn, 0, SEEK_END);
	filesize = ftell(FileIn); 
	fseek(FileIn, 0, SEEK_SET); 
	u8 *FIRM = new u8[filesize];

	if (fread(FIRM, 1, filesize, FileIn) != filesize) {
		delete[] FIRM;
		fclose(FileIn);
		fatUnmount("fat:/");
		return FILE_IO_FAILED; //File reading failed
	}
	fclose(FileIn);
	fatUnmount("fat:/"); //We must unmount *before* calling any flashcart_core functions

	if (!cart->injectNtrBoot((isDevMode) ? blowfish_dev_bin : blowfish_retail_bin, FIRM, filesize)) {
		delete[] FIRM;
		return INJECT_OR_DUMP_FAILED; //FIRM injection failed
	}
	
	delete[] FIRM;
	return ALL_OK;
}

return_codes_t DumpFlash(flashcart_core::Flashcart* cart)
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size
	const u32 chunkSize = 0x80000; // chunk out in half megabyte chunks out to avoid ram limitations

	if (!fatInitDefault()) { return FAT_MOUNT_FAILED; }

	mkdir("fat:/ntrboot", 0700); //If the directory exists, this line isn't going to crash the program or anything like that

	fatUnmount("fat:/");

	char* backup_path = calculate_backup_path(cart->getShortName());

	u8 *Flashrom = new u8[chunkSize]; //Allocate a new array to store the flashrom we are about to retrieve from the flashcart

	for (u32 chunkOffset = 0; chunkOffset < Flash_size; chunkOffset += chunkSize) {
		DrawRectangle(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT * 2, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
		DrawStringF(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT * 2, COLOR_WHITE, "Reading at 0x%x", chunkOffset);

		if (!cart->readFlash(chunkOffset, chunkSize, Flashrom)) {
			delete[] Flashrom;
			free(backup_path);
			return INJECT_OR_DUMP_FAILED; //Flash reading failed
		}

		if (!fatInitDefault())
		{
			delete[] Flashrom;
			free(backup_path);
			return FAT_MOUNT_FAILED; //Fat init failed
		}

		FILE *FileOut = fopen(backup_path, chunkOffset == 0 ? "wb" : "ab");
		if (!FileOut) {
			delete[] Flashrom;
			fclose(FileOut);
			fatUnmount("fat:/");
			free(backup_path);
			return FILE_OPEN_FAILED; //File opening failed
		}

		if (fwrite(Flashrom, 1, chunkSize, FileOut) != chunkSize) {
			delete[] Flashrom;
			fclose(FileOut);
			fatUnmount("fat:/");
			free(backup_path);
			return FILE_IO_FAILED; //File writing failed
		}

		fclose(FileOut);
		fatUnmount("fat:/");
	}

	//Draw a black rectangle over the old "Reading at..." message to clear it away
	DrawRectangle(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - 2 * FONT_HEIGHT, 20 * FONT_WIDTH, FONT_HEIGHT, COLOR_BLACK);

	free(backup_path);
	delete[] Flashrom;
	return ALL_OK;
}
