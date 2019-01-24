// Part of Neogeo_MiSTer
// (C) 2019 Sean 'furrtek' Gonsalves

#include <string.h>
#include "graphics.h"
#include "loader.h"
#include "../../user_io.h"
#include "../../osd.h"

int neogeo_file_tx(const char* romset, const char* name, unsigned char neo_file_type, unsigned char index, unsigned long offset, unsigned long bytes2send)
{
	fileTYPE f = {};
	static uint8_t buf[4096];
	static char name_buf[256];
	int last_osd_line = OsdGetSize() - 1;

	strcpy(name_buf, "/media/fat/NeoGeo/");
	if (strlen(romset)) {
		strcat(name_buf, romset);
		strcat(name_buf, "/");
	}
	strcat(name_buf, name);

	if (!FileOpen(&f, name_buf, 0)) return 0;
	
	//unsigned long bytes2send = f.size;

	FileSeek(&f, offset, SEEK_SET);

	// transmit the entire file using one transfer
	OsdWrite(last_osd_line, name, 0, 0);
	printf("Loading file %s (start %lu, size %lu, type %u) with index 0x%02X\n", name, offset, bytes2send, neo_file_type, index);

	// set index byte
	user_io_set_index(index);

	// prepare transmission of new file
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0xff);
	DisableFpga();
	
	if (neo_file_type == NEO_FILE_FIX)
	{
		EnableFpga();
		spi8(UIO_FILE_TX_DAT);
		fix_convert(&f);
		DisableFpga();
	}
	else if (neo_file_type == NEO_FILE_SPR)
	{
		EnableFpga();
		spi8(UIO_FILE_TX_DAT);
		spr_convert(&f);
		DisableFpga();
	} else {
		while (bytes2send)
		{
			printf(".");

			uint16_t chunk = (bytes2send > sizeof(buf)) ? sizeof(buf) : bytes2send;

			FileReadAdv(&f, buf, chunk);

			EnableFpga();
			spi8(UIO_FILE_TX_DAT);
			spi_write(buf, chunk, 1);
			DisableFpga();

			bytes2send -= chunk;
		}
		printf("\n");
	}

	FileClose(&f);

	// signal end of transmission
	EnableFpga();
	spi8(UIO_FILE_TX);
	spi8(0x00);
	DisableFpga();
	printf("\n");

	return 1;
}

int neogeo_romset_tx(char* name) {
	char romset[8 + 1];
	//unsigned int i;

	memset(romset, 0, sizeof(romset));

	// Get romset name from path (current directory)
	char *p = strrchr(name, '/');
	if (!p) return 0;
	*p = 0;
	p = strrchr(name, '/');
	if (!p) return 0;
	strncpy(romset, p + 1, strlen(p + 1));
	
	user_io_8bit_set_status(1, 1);	// Maintain reset

	/*unsigned int entry_count = sizeof romset_defs / sizeof romset_defs[0];
	for (i = 0; i < entry_count; i++) {
		if (!strcmp(romset_defs[i].name, romset))
			break;
	}
	if (i == entry_count)
		return 0;

	printf("Loading romset %s...\n", romset);
	
	unsigned int file_count = romset_defs[i].file_count;
	for (unsigned int j = 0; j < file_count ; j++) {
		OsdWrite(last_osd_line, romset_defs[i].file_list[j].name, 0, 0);
		neogeo_file_tx(&romset_defs[i].file_list[j]);
	}*/
	
	neogeo_file_tx("", "neo-epo.sp1", NEO_FILE_RAW, 0, 0, 0x20000);
	
	if (!strcmp(romset, "eightman")) {
		neogeo_file_tx(romset, "025-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000);
		neogeo_file_tx(romset, "025-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000);
		neogeo_file_tx(romset, "025-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x100000);		neogeo_file_tx(romset, "025-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x100000);
		neogeo_file_tx(romset, "025-c3.c3", NEO_FILE_SPR, 32 + (2 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "025-c4.c4", NEO_FILE_SPR, 32 + (2 << 1) + 0, 0, 0x80000);
	} else if (!strcmp(romset, "lbowling")) {
		neogeo_file_tx(romset, "019-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000);
		neogeo_file_tx(romset, "019-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000);
		neogeo_file_tx(romset, "019-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "019-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x80000);
	} else if (!strcmp(romset, "lresort")) {
		neogeo_file_tx(romset, "024-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000);
		neogeo_file_tx(romset, "024-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000);
		neogeo_file_tx(romset, "024-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x100000);		neogeo_file_tx(romset, "024-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x100000);
		neogeo_file_tx(romset, "024-c3.c3", NEO_FILE_SPR, 32 + (2 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "024-c4.c4", NEO_FILE_SPR, 32 + (2 << 1) + 0, 0, 0x80000);
	} else if (!strcmp(romset, "mslug")) {
		neogeo_file_tx(romset, "201-p1.p1", NEO_FILE_RAW, 1, 0x100000, 0x100000 );
		neogeo_file_tx(romset, "201-p1.p1", NEO_FILE_RAW, 2, 0, 0x100000 );
		neogeo_file_tx(romset, "201-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "201-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x400000 );		neogeo_file_tx(romset, "201-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x400000 );
		neogeo_file_tx(romset, "201-c3.c3", NEO_FILE_SPR, 32 + (8 << 1) + 1, 0, 0x400000 );		neogeo_file_tx(romset, "201-c4.c4", NEO_FILE_SPR, 32 + (8 << 1) + 0, 0, 0x400000 );
	} else if (!strcmp(romset, "nam1975")) {
		neogeo_file_tx(romset, "001-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000);
		neogeo_file_tx(romset, "001-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000);
		neogeo_file_tx(romset, "001-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "001-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x80000);
		neogeo_file_tx(romset, "001-c3.c3", NEO_FILE_SPR, 32 + (1 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "001-c4.c4", NEO_FILE_SPR, 32 + (1 << 1) + 0, 0, 0x80000);
		neogeo_file_tx(romset, "001-c5.c5", NEO_FILE_SPR, 32 + (2 << 1) + 1, 0, 0x80000);		neogeo_file_tx(romset, "001-c6.c6", NEO_FILE_SPR, 32 + (2 << 1) + 0, 0, 0x80000);
	} else if (!strcmp(romset, "ncombat")) {
		neogeo_file_tx(romset, "009-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000 );
		neogeo_file_tx(romset, "009-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "009-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x80000 );		neogeo_file_tx(romset, "009-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x80000 );
		neogeo_file_tx(romset, "009-c3.c3", NEO_FILE_SPR, 32 + (1 << 1) + 1, 0, 0x80000 );		neogeo_file_tx(romset, "009-c4.c4", NEO_FILE_SPR, 32 + (1 << 1) + 0, 0, 0x80000 );
		neogeo_file_tx(romset, "009-c5.c5", NEO_FILE_SPR, 32 + (2 << 1) + 1, 0, 0x80000 );		neogeo_file_tx(romset, "009-c6.c6", NEO_FILE_SPR, 32 + (2 << 1) + 0, 0, 0x80000 );
	} else if (!strcmp(romset, "neobombe")) {
		neogeo_file_tx(romset, "093-p1.p1", NEO_FILE_RAW, 1, 0, 0x100000 );
		neogeo_file_tx(romset, "093-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "093-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x400000 );		neogeo_file_tx(romset, "093-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x400000 );
		neogeo_file_tx(romset, "093-c3.c3", NEO_FILE_SPR, 32 + (8 << 1) + 1, 0, 0x80000 );		neogeo_file_tx(romset, "093-c4.c4", NEO_FILE_SPR, 32 + (8 << 1) + 0, 0, 0x80000 );
	} else if (!strcmp(romset, "panicbom")) {
		neogeo_file_tx(romset, "073-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000 );
		neogeo_file_tx(romset, "073-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "073-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x100000 );		neogeo_file_tx(romset, "073-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x100000 );
	} else if (!strcmp(romset, "pbobblen")) {
		neogeo_file_tx(romset, "d96-07.ep1", NEO_FILE_RAW, 1, 0, 0x80000 );
		neogeo_file_tx(romset, "d96-04.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "d96-02.c5", NEO_FILE_SPR, 32 + (4 << 1) + 1, 0, 0x80000 );		neogeo_file_tx(romset, "d96-03.c6", NEO_FILE_SPR, 32 + (4 << 1) + 0, 0, 0x80000 );
	} else if (!strcmp(romset, "puzzledp")) {
		neogeo_file_tx(romset, "202-p1.p1", NEO_FILE_RAW, 1, 0, 0x80000 );
		neogeo_file_tx(romset, "202-s1.s1", NEO_FILE_FIX, 3, 0, 0x20000 );
		neogeo_file_tx(romset, "202-c1.c1", NEO_FILE_SPR, 32 + (0 << 1) + 1, 0, 0x100000 );		neogeo_file_tx(romset, "202-c2.c2", NEO_FILE_SPR, 32 + (0 << 1) + 0, 0, 0x100000 );
	}

	user_io_8bit_set_status(0, 1);	// Release reset

	return 1;
}
