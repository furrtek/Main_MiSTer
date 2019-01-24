#include "../../file_io.h"

#define NEO_FILE_RAW 1
#define NEO_FILE_FIX 2
#define NEO_FILE_SPR 3

/*struct romset_file {
	const char* name;
	unsigned char neo_file_type;
	unsigned char index;
	unsigned long start;
	unsigned long size;
};

struct romset_def {
	const char* name;
	unsigned char file_count;
	romset_file file_list[];
};*/

int neogeo_romset_tx(char* name);
